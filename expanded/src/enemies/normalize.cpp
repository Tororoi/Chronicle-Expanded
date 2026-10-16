#include "expanded/enemies/normalize.hpp"
#include "expanded/enemies/monsters.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <optional>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/enemies/stronger.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kKinds = 167;
constexpr int kStats = 6;
constexpr int kDemonShaft = 6;
constexpr int kBands = 5;
constexpr int kBandFloors = 20;
constexpr int kBeyond = kEnemyRegions; // past Demon Shaft's last band, for Stronger enemies

// Look-alikes (one rig and its reskins), weakest to strongest: none outstrips the next on a floor.
const std::vector<std::vector<int>> kLookAlikes = {
    {MONSTER_CAVE_BAT, MONSTER_EVIL_BAT, MONSTER_CAVE_BAT_ENHANCED, MONSTER_EVIL_BAT_ENHANCED},
    {MONSTER_CRESCENT_BARON, MONSTER_CRESCENT_BARON_ENHANCED},
    {MONSTER_SKELETON_SOLDIER, MONSTER_MASTER_JACKET, MONSTER_MASTER_JACKET_ENHANCED, MONSTER_HORN_HEAD, MONSTER_SILVER_GEAR, MONSTER_GACIOUS_ENHANCED},
    {MONSTER_DASHER, MONSTER_RASH_DASHER, MONSTER_RASH_DASHER_ENHANCED},
    {MONSTER_HORNET, MONSTER_PHANTOM},
    {MONSTER_HALLOWEEN, MONSTER_HALLOWEEN_ENHANCED},
    {MONSTER_CANNIBAL_PLANT, MONSTER_CURSED_ROSE, MONSTER_DARK_FLOWER, MONSTER_CURSED_ROSE_ENHANCED},
    {MONSTER_EARTH_DIGGER, MONSTER_MOON_DIGGER},
    {MONSTER_SUNDAY, MONSTER_MONDAY, MONSTER_TUESDAY, MONSTER_WEDNESDAY, MONSTER_THURSDAY, MONSTER_FRIDAY, MONSTER_SATURDAY, MONSTER_HELL_POCKLE},
    {MONSTER_WITCH_ILLZA, MONSTER_WITCH_HELLZA, MONSTER_WITCH_HELLZA_ENHANCED},
    {MONSTER_GYON, MONSTER_SPACE_GYON, MONSTER_GYON_ENHANCED, MONSTER_SPACE_GYON_ENHANCED},
    {MONSTER_PIRATES_CHARIOT, MONSTER_PIRATES_CHARIOT_ENHANCED},
    {MONSTER_AUNTIE_MEDU, MONSTER_AUNTIE_MEDU_ENHANCED},
    {MONSTER_CAPTAIN, MONSTER_CAPTAIN_ENHANCED},
    {MONSTER_CORCEA, MONSTER_CORCEA_ENHANCED},
    {MONSTER_GOLEM, MONSTER_SIL, MONSTER_GOL, MONSTER_STEEL_GIANT, MONSTER_TITAN, MONSTER_BLIZZARD, MONSTER_SIL_ENHANCED, MONSTER_STEEL_GIANT_ENHANCED, MONSTER_GOL_ENHANCED, MONSTER_TITAN_ENHANCED},
    {MONSTER_DUNE, MONSTER_VULCAN, MONSTER_VULCAN_ENHANCED},
    {MONSTER_ARTHUR, MONSTER_ARTHUR_ENHANCED},
    {MONSTER_GHOST, MONSTER_LICH, MONSTER_LICH_ENHANCED},
    {MONSTER_ALEXANDER, MONSTER_ALEXANDER_ENHANCED},
    {MONSTER_HEART, MONSTER_CLUB, MONSTER_DIAMOND, MONSTER_SPADE, MONSTER_JOKER, MONSTER_DIAMOND_ENHANCED, MONSTER_CLUB_ENHANCED, MONSTER_SPADE_ENHANCED, MONSTER_HEART_ENHANCED, MONSTER_JOKER_ENHANCED},
    {MONSTER_BOMBER_HEAD, MONSTER_BOMBER_HEAD_ENHANCED},
    {MONSTER_MUMMY, MONSTER_MUMMY_ENHANCED},
    {MONSTER_STATUE, MONSTER_LIVING_ARMOR, MONSTER_LIVING_ARMOR_ENHANCED},
    {MONSTER_WEREWOLF, MONSTER_WHITE_FANG, MONSTER_WHITE_FANG_ENHANCED},
    {MONSTER_SAM, MONSTER_MR_BLARE, MONSTER_BILLY},
    {MONSTER_CRABBY_HERMIT, MONSTER_CRABBY_HERMIT_ENHANCED},
    {MONSTER_DRAGON, MONSTER_BLUE_DRAGON, MONSTER_BLACK_DRAGON},
    {MONSTER_GEMRON_FIRE, MONSTER_GEMRON_ICE, MONSTER_GEMRON_THUNDER, MONSTER_GEMRON_WIND, MONSTER_GEMRON_HOLY},
    {MONSTER_MASK_OF_PRAJNA, MONSTER_MASK_OF_PRAJNA_ENHANCED},
    {MONSTER_ROCKANOFF, MONSTER_ROCKANOFF_ENHANCED},
    {MONSTER_YAMMICH, MONSTER_YAMMICH_ENHANCED},
    {MONSTER_STATUE_DOG, MONSTER_STATUE_DOG_ENHANCED},
};

// The Mimic and the King Mimic, dungeon by dungeon then Demon Shaft's Enhanced, and their stats past the last.
constexpr int kMimics[] = {
    MONSTER_MIMIC_DBC,
    MONSTER_MIMIC_WOF,
    MONSTER_MIMIC_SW,
    MONSTER_MIMIC_SMT,
    MONSTER_MIMIC_MS,
    MONSTER_MIMIC_GOT,
    MONSTER_MIMIC_DS,
    MONSTER_MIMIC_DS_ENHANCED,
    MONSTER_MIMIC_DS_ENHANCED_TWICE,
    MONSTER_MIMIC_DS_ENHANCED_THRICE,
};
constexpr int kKingMimics[] = {
    MONSTER_KING_MIMIC_DBC,
    MONSTER_KING_MIMIC_WOF,
    MONSTER_KING_MIMIC_SW,
    MONSTER_KING_MIMIC_SMT,
    MONSTER_KING_MIMIC_MS,
    MONSTER_KING_MIMIC_GOT,
    MONSTER_KING_MIMIC_DS,
    MONSTER_KING_MIMIC_DS_ENHANCED,
    MONSTER_KING_MIMIC_DS_ENHANCED_TWICE,
    MONSTER_KING_MIMIC_DS_ENHANCED_THRICE,
};
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
    return MonstorTable[model_no].kind != MONSTER_KIND_NO_LOCK_ON;
}

struct Pools {
    bool                             member[kEnemyRegions][kKinds] = {};
    int                              home[kKinds];      // lowest region laying the kind out, or -1
    float                            average[kEnemyRegions + 1][kStats] = {};
    float                            cap[kEnemyRegions][kStats] = {}; // highest of this region and the earlier ones
    const std::vector<int>          *look_alikes[kKinds] = {};
    const int                       *mimic_line[kKinds] = {};
    int                              mimic_index[kKinds] = {};
};

Pools Build() {
    Pools pools;
    std::fill(std::begin(pools.home), std::end(pools.home), -1);
    for (int dungeon = 0; dungeon <= kDemonShaft; dungeon++) {
        for (BT_ENEMY_FLOOR *layout : {BtEnemyLayoutList[dungeon], BtUraEnemyLayoutList[dungeon]}) {
            for (int floor = 0; floor < maxFloorTbl__3[dungeon]; floor++) {
                int region = EnemyRegion(dungeon, floor);
                for (const BT_ENEMY_LAYOUT &entry : layout[floor].monster) {
                    if (entry.monster_no < 0 || entry.monster_no >= kKinds) {
                        break;
                    }
                    pools.member[region][entry.monster_no] = true;
                    int &home = pools.home[entry.monster_no];
                    home = home == -1 ? region : std::min(home, region);
                }
            }
        }
    }
    for (int region = 0; region < kEnemyRegions; region++) {
        for (int s = 0; s < kStats; s++) {
            EnemyStat stat = static_cast<EnemyStat>(s);
            long long sum = 0;
            int       count = 0, most = 0;
            for (int kind = 0; kind < kKinds; kind++) {
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
    for (const std::vector<int> &chain : kLookAlikes) {
        for (int kind : chain) {
            pools.look_alikes[kind] = &chain;
        }
    }
    for (int i = 0; i < 10; i++) {
        pools.mimic_line[kMimics[i]] = kMimics;
        pools.mimic_index[kMimics[i]] = i;
        pools.mimic_line[kKingMimics[i]] = kKingMimics;
        pools.mimic_index[kKingMimics[i]] = i;
    }
    return pools;
}

const Pools &State() {
    static const Pools pools = Build();
    return pools;
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
std::optional<int> NextMimicStat(const int *line, int index, EnemyStat stat) {
    if (index + 1 < 10) {
        return Native(line[index + 1], stat);
    }
    const EnemyStats &beyond = line == kKingMimics ? kKingMimicBeyond : kMimicBeyond;
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
    EnemyStats stats = NormalizedStats(model_no, region, StrongerEnemies());
    if (!(stats == NativeStats(model_no))) {
        SetSpeciesStats(unit, model_slot, stats);
    }
}

const bool kRegistered = (AddEnemyModelListener(Normalize), true);

} // namespace

int EnemyRegion(int dungeon, int floor) {
    if (dungeon < 0 || dungeon > kDemonShaft) {
        return -1;
    }
    return dungeon < kDemonShaft ? dungeon : kDemonShaft + std::clamp(floor / kBandFloors, 0, kBands - 1);
}

int NormalizedStat(int model_no, EnemyStat stat, int region, bool stronger) {
    if (model_no < 0 || model_no >= kKinds || region < 0 || region >= kEnemyRegions) {
        return Native(model_no, stat);
    }
    const std::vector<int> *chain = State().look_alikes[model_no];
    if (chain == nullptr) {
        return PoolValue(model_no, stat, region, stronger);
    }
    // Down from the strongest look-alike, each capped at the one above it.
    int cap = INT_MAX;
    for (auto member = chain->rbegin(); member != chain->rend(); ++member) {
        int value = PoolValue(*member, stat, region, stronger);
        if (!IsAttack(stat) || value > 0) {
            value = std::min(value, cap);
            cap = value;
        }
        if (*member == model_no) {
            return value;
        }
    }
    return Native(model_no, stat);
}

EnemyStats NormalizedStats(int model_no, int region, bool stronger) {
    EnemyStats native = NativeStats(model_no);
    if (model_no < 0 || model_no >= kKinds || region < 0 || region >= kEnemyRegions || !IsFloorEnemy(model_no) ||
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
