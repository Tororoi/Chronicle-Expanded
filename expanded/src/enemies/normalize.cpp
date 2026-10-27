#include "dungeon.h"
#include "expanded/enemies/normalize.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <filesystem>
#include <optional>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/enemies/boss_stats.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/enemies/stronger.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kStats = 6;
constexpr int kBeyond = kEnemyRegions; // past Demon Shaft's last band, for Stronger enemies

// Look-alikes (one rig and its reskins), weakest to strongest: none outstrips the next on a floor.
const std::vector<std::vector<Monster>> kLookAlikes = {
    {Monster::CAVE_BAT, Monster::EVIL_BAT, Monster::CAVE_BAT_ENHANCED, Monster::EVIL_BAT_ENHANCED},
    {Monster::CRESCENT_BARON, Monster::CRESCENT_BARON_ENHANCED},
    {Monster::SKELETON_SOLDIER, Monster::MASTER_JACKET, Monster::MASTER_JACKET_ENHANCED, Monster::HORN_HEAD, Monster::SILVER_GEAR, Monster::GACIOUS_ENHANCED},
    {Monster::DASHER, Monster::RASH_DASHER, Monster::RASH_DASHER_ENHANCED},
    {Monster::HORNET, Monster::PHANTOM},
    {Monster::HALLOWEEN, Monster::HALLOWEEN_ENHANCED},
    {Monster::CANNIBAL_PLANT, Monster::CURSED_ROSE, Monster::DARK_FLOWER, Monster::CURSED_ROSE_ENHANCED},
    {Monster::EARTH_DIGGER, Monster::MOON_DIGGER},
    {Monster::SUNDAY, Monster::MONDAY, Monster::TUESDAY, Monster::WEDNESDAY, Monster::THURSDAY, Monster::FRIDAY, Monster::SATURDAY, Monster::HELL_POCKLE},
    {Monster::WITCH_ILLZA, Monster::WITCH_HELLZA, Monster::WITCH_HELLZA_ENHANCED},
    {Monster::GYON, Monster::SPACE_GYON, Monster::GYON_ENHANCED, Monster::SPACE_GYON_ENHANCED},
    {Monster::PIRATES_CHARIOT, Monster::PIRATES_CHARIOT_ENHANCED},
    {Monster::AUNTIE_MEDU, Monster::AUNTIE_MEDU_ENHANCED},
    {Monster::CAPTAIN, Monster::CAPTAIN_ENHANCED},
    {Monster::CORCEA, Monster::CORCEA_ENHANCED},
    {Monster::GOLEM, Monster::SIL, Monster::GOL, Monster::STEEL_GIANT, Monster::TITAN, Monster::BLIZZARD, Monster::SIL_ENHANCED, Monster::STEEL_GIANT_ENHANCED, Monster::GOL_ENHANCED, Monster::TITAN_ENHANCED},
    {Monster::DUNE, Monster::VULCAN, Monster::VULCAN_ENHANCED},
    {Monster::ARTHUR, Monster::ARTHUR_ENHANCED},
    {Monster::GHOST, Monster::LICH, Monster::LICH_ENHANCED},
    {Monster::ALEXANDER, Monster::ALEXANDER_ENHANCED},
    {Monster::HEART, Monster::CLUB, Monster::DIAMOND, Monster::SPADE, Monster::JOKER, Monster::DIAMOND_ENHANCED, Monster::CLUB_ENHANCED, Monster::SPADE_ENHANCED, Monster::HEART_ENHANCED, Monster::JOKER_ENHANCED},
    {Monster::BOMBER_HEAD, Monster::BOMBER_HEAD_ENHANCED},
    {Monster::MUMMY, Monster::MUMMY_ENHANCED},
    {Monster::STATUE, Monster::LIVING_ARMOR, Monster::LIVING_ARMOR_ENHANCED},
    {Monster::WEREWOLF, Monster::WHITE_FANG, Monster::WHITE_FANG_ENHANCED},
    {Monster::SAM, Monster::MR_BLARE, Monster::BILLY},
    {Monster::CRABBY_HERMIT, Monster::CRABBY_HERMIT_ENHANCED},
    {Monster::DRAGON, Monster::BLUE_DRAGON, Monster::BLACK_DRAGON},
    {Monster::GEMRON_FIRE, Monster::GEMRON_ICE, Monster::GEMRON_THUNDER, Monster::GEMRON_WIND, Monster::GEMRON_HOLY, Monster::BOMB_GEMRON},
    {Monster::MASK_OF_PRAJNA, Monster::MASK_OF_PRAJNA_ENHANCED},
    {Monster::ROCKANOFF, Monster::ROCKANOFF_ENHANCED},
    {Monster::YAMMICH, Monster::YAMMICH_ENHANCED},
    {Monster::STATUE_DOG, Monster::STATUE_DOG_ENHANCED},
};

// The Mimics' and King Mimics' stats past the last of their lines.
constexpr EnemyStats kMimicBeyond = {.hp = 12000, .defense = 40, .hardness = 15, .melee = 140};
constexpr EnemyStats kKingMimicBeyond = {.hp = 30000, .defense = 40, .hardness = 60, .melee = 180};

int Native(int model_no, EnemyStat stat) {
    EnemyStats native = NativeStats(model_no);
    switch (stat) {
        case EnemyStat::Hp:
            return native.hp;
        case EnemyStat::Abs:
            return native.abs;
        case EnemyStat::Defense:
            return native.defense;
        case EnemyStat::Hardness:
            return native.hardness;
        case EnemyStat::Melee:
            return native.melee;
        case EnemyStat::Shot:
            return native.shot;
    }
    return 0;
}

bool IsAttack(EnemyStat stat) {
    return stat == EnemyStat::Melee || stat == EnemyStat::Shot;
}

// A regular monster or a mimic; bosses and their parts are never lock-on targets.
bool IsFloorEnemy(int model_no) {
    const MONSTOR_MODEL *species = Species(model_no);
    return species != nullptr && species->kind != MONSTER_KIND_NO_LOCK_ON;
}

struct Pools {
    bool                        member[kEnemyRegions][kSpeciesCount] = {};
    int                         home[kSpeciesCount]; // lowest region laying the kind out, or -1
    float                       average[kEnemyRegions + 1][kStats] = {};
    float                       cap[kEnemyRegions][kStats] = {}; // highest of this region and the earlier ones
    const std::vector<Monster> *look_alikes[kSpeciesCount] = {};
    const Monster              *mimic_line[kSpeciesCount] = {};
    int                         mimic_index[kSpeciesCount] = {};
};

Pools Build() {
    Pools pools;
    std::fill(std::begin(pools.home), std::end(pools.home), -1);
    ForEachLaidOutKind([&](int dungeon, int floor, int kind) {
        if (ValidKind(kind)) {
            int region = EnemyRegion(dungeon, floor);
            pools.member[region][kind] = true;
            int &home = pools.home[kind];
            home = home == -1 ? region : std::min(home, region);
        }
    });
    // Expanded's kinds are at home where their stand-ins are.
    for (int kind = kRetailSpecies; kind < kSpeciesCount; kind++) {
        pools.home[kind] = pools.home[SpeciesStandIn(kind)];
    }
    for (int region = 0; region < kEnemyRegions; region++) {
        for (int s = 0; s < kStats; s++) {
            EnemyStat stat = static_cast<EnemyStat>(s);
            long long sum = 0;
            int       count = 0, most = 0;
            for (int kind = 0; kind < kSpeciesCount; kind++) {
                int value = Native(kind, stat);
                if (!pools.member[region][kind] || !IsFloorEnemy(kind) || (IsAttack(stat) && value <= 0)) {
                    continue;
                }
                sum += value;
                most = count == 0 ? value : std::max(most, value);
                count++;
            }
            pools.average[region][s] = count > 0 ? static_cast<float>(static_cast<double>(sum) / count) : 0.0f;
            pools.cap[region][s] = std::max(static_cast<float>(most), region > 0 ? pools.cap[region - 1][s] : 0.0f);
        }
    }
    // Past the last band the average keeps the last two bands' trend, never below the last.
    for (int s = 0; s < kStats; s++) {
        float last = pools.average[kEnemyRegions - 1][s], before = pools.average[kEnemyRegions - 2][s];
        pools.average[kBeyond][s] = std::max(last, 2 * last - before);
    }
    for (const std::vector<Monster> &chain : kLookAlikes) {
        for (Monster kind : chain) {
            pools.look_alikes[No(kind)] = &chain;
        }
    }
    for (int i = 0; i < kMimicLineLength; i++) {
        pools.mimic_line[No(kMimicLine[i])] = kMimicLine;
        pools.mimic_index[No(kMimicLine[i])] = i;
        pools.mimic_line[No(kKingMimicLine[i])] = kKingMimicLine;
        pools.mimic_index[No(kKingMimicLine[i])] = i;
    }
    return pools;
}

// Built once per data directory, whose script damage the averages take in.
const Pools &State() {
    static std::filesystem::path built_for;
    static std::optional<Pools>  pools;
    if (!pools || built_for != PathsDataRoot()) {
        built_for = PathsDataRoot();
        pools = Build();
    }
    return *pools;
}

// Stronger enemies: the next region's average over this one's in ten-thousandths, at least 10000.
int StrongerRatio(EnemyStat stat, int region) {
    float current = State().average[region][static_cast<int>(stat)];
    float next = State().average[region + 1][static_cast<int>(stat)];
    if (current <= 0 || next <= 0) {
        return 10000;
    }
    return std::max(10000, static_cast<int>(std::nearbyint(static_cast<double>(next) / current * 10000)));
}

// The next mimic's stat, or the stats past the last one.
std::optional<int> NextMimicStat(const Monster *line, int index, EnemyStat stat) {
    if (index + 1 < kMimicLineLength) {
        return Native(No(line[index + 1]), stat);
    }
    const EnemyStats &beyond = line == kKingMimicLine ? kKingMimicBeyond : kMimicBeyond;
    switch (stat) {
        case EnemyStat::Hp:
            return beyond.hp;
        case EnemyStat::Defense:
            return beyond.defense;
        case EnemyStat::Hardness:
            return beyond.hardness;
        case EnemyStat::Melee:
            return beyond.melee;
        default:
            return std::nullopt;
    }
}

// The pool rule alone, then Stronger enemies.
int PoolValue(int model_no, EnemyStat stat, int region, bool stronger) {
    const Pools &pools = State();
    int          native = Native(model_no, stat);
    int          home = pools.home[model_no];
    if (home == -1 || !IsFloorEnemy(model_no)) {
        return native;
    }
    int  value = native;
    bool mimic = pools.mimic_line[model_no] != nullptr;
    if (!mimic && !pools.member[region][model_no] && !(IsAttack(stat) && native <= 0)) {
        float average = pools.average[region][static_cast<int>(stat)], cap = pools.cap[region][static_cast<int>(stat)];
        if (home < region && average > 0) {
            value = std::max(value, static_cast<int>(std::round(static_cast<double>(average))));
        }
        if (home > region && cap > 0) {
            value = std::min(value, static_cast<int>(std::round(static_cast<double>(cap))));
        }
    }
    if (!stronger || stat == EnemyStat::Abs || value <= 0) {
        return value;
    }
    int ratio = StrongerRatio(stat, region);
    if (ratio != 10000) {
        value = std::max(1, static_cast<int>(std::round(value * ratio / 10000.0)));
    }
    if (mimic) {
        if (std::optional<int> next = NextMimicStat(pools.mimic_line[model_no], pools.mimic_index[model_no], stat)) {
            value = std::max(value, *next);
        }
    }
    return value;
}

// Model slot listener: a kind set up on a floor is normalized for the floor's region, with Stronger Enemies.
void Normalize(CMonstorUnit &unit, int model_slot, int model_no) {
    int region = UserStatus != nullptr ? EnemyRegion(selectMapNo, UserStatus->cur_floor) : -1;
    if (region < 0) {
        return;
    }
    if (std::optional<EnemyStats> boss = BossRegionStats(model_no, region)) {
        SetSpeciesStats(unit, model_slot, *boss);
        return;
    }
    EnemyStats stats = NormalizedStats(model_no, region, StrongerEnemies());
    if (!(stats == NativeStats(model_no))) {
        SetSpeciesStats(unit, model_slot, stats);
    }
}

const bool kRegistered = (AddEnemyModelListener(Normalize), true);

} // namespace

int DemonShaftBand(int floor) {
    return std::clamp(floor / kDemonShaftBandFloors, 0, kDemonShaftBands - 1);
}

int EnemyRegion(int dungeon, int floor) {
    if (dungeon < 0 || dungeon > DUNGEON_DEMON_SHAFT) {
        return -1;
    }
    return dungeon < DUNGEON_DEMON_SHAFT ? dungeon : DUNGEON_DEMON_SHAFT + DemonShaftBand(floor);
}

int NormalizedStat(int model_no, EnemyStat stat, int region, bool stronger) {
    if (!ValidKind(model_no) || region < 0 || region >= kEnemyRegions) {
        return Native(model_no, stat);
    }
    const std::vector<Monster> *chain = State().look_alikes[model_no];
    if (chain == nullptr) {
        return PoolValue(model_no, stat, region, stronger);
    }
    // Down from the strongest look-alike, each capped at the one above it.
    int cap = INT_MAX;
    for (auto member = chain->rbegin(); member != chain->rend(); ++member) {
        int value = PoolValue(No(*member), stat, region, stronger);
        if (!IsAttack(stat) || value > 0) {
            value = std::min(value, cap);
            cap = value;
        }
        if (No(*member) == model_no) {
            return value;
        }
    }
    return Native(model_no, stat);
}

EnemyStats NormalizedStats(int model_no, int region, bool stronger) {
    EnemyStats native = NativeStats(model_no);
    if (!ValidKind(model_no) || region < 0 || region >= kEnemyRegions || !IsFloorEnemy(model_no) ||
        State().home[model_no] == -1) {
        return native;
    }
    bool own = State().member[region][model_no];
    if (own && !stronger) {
        return native;
    }
    return {.hp = NormalizedStat(model_no, EnemyStat::Hp, region, stronger),
            .abs = own ? native.abs : NormalizedStat(model_no, EnemyStat::Abs, region, false),
            .defense = NormalizedStat(model_no, EnemyStat::Defense, region, stronger),
            .hardness = NormalizedStat(model_no, EnemyStat::Hardness, region, stronger),
            .melee = NormalizedStat(model_no, EnemyStat::Melee, region, stronger),
            .shot = NormalizedStat(model_no, EnemyStat::Shot, region, stronger)};
}

} // namespace expanded
