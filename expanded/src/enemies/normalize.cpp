#include "expanded/enemies/normalize.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <optional>
#include <vector>

#include "dun/gameloop.hpp"
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
    {52, 53, 134, 156},                         // Cave Bat, Evil Bat and their Enhanced
    {68, 157},                                  // Crescent Baron
    {1, 0, 117, 122, 144, 155},                 // Skeleton Soldier, Master Jacket(+E), Horn Head, Silver Gear, Gacious (E)
    {3, 55, 139},                               // Dasher, Rash Dasher(+E)
    {6, 50},                                    // Hornet, Phantom
    {7, 116},                                   // Halloween
    {8, 60, 59, 149},                           // Cannibal Plant, Cursed Rose, Dark Flower, Cursed Rose (E)
    {9, 58},                                    // Earth Digger, Moon Digger
    {10, 11, 12, 13, 14, 15, 16, 54},           // Sunday to Saturday, Hell Pockle
    {18, 17, 126},                              // Witch Illza, Witch Hellza(+E)
    {20, 64, 137, 151},                         // Gyon, Space Gyon and their Enhanced
    {21, 150},                                  // Pirate's Chariot
    {22, 123},                                  // Auntie Medu
    {23, 140},                                  // Captain
    {24, 129},                                  // Corcea
    {25, 95, 94, 56, 28, 57, 115, 127, 135, 161}, // Golem, Sil, Gol, Steel Giant, Titan, Blizzard and Enhanced
    {27, 62, 118},                              // Dune, Vulcan(+E)
    {35, 114},                                  // Arthur
    {36, 45, 160},                              // Ghost, Lich(+E)
    {37, 145},                                  // Alexander
    {38, 39, 40, 41, 42, 120, 128, 138, 146, 159}, // Heart, Club, Diamond, Spade, Joker and Enhanced
    {43, 147},                                  // Bomber Head
    {44, 119},                                  // Mummy
    {2, 47, 162},                               // Statue, Living Armor(+E)
    {4, 48, 113},                               // Werewolf, White Fang(+E)
    {77, 26, 61},                               // Sam, Mr. Blare, Billy
    {63, 148},                                  // Crabby Hermit
    {51, 65, 66},                               // Dragon, Blue Dragon, Black Dragon
    {111, 121, 132, 143, 154},                  // Gemron Fire, Ice, Thunder, Wind, Holy
    {67, 136},                                  // Mask of Prajna
    {69, 124},                                  // Rockanoff
    {96, 125},                                  // Yammich
    {97, 158},                                  // Statue Dog
};

// The Mimic and the King Mimic, dungeon by dungeon then Demon Shaft's Enhanced, and their stats past the last.
constexpr int kMimics[] = {30, 71, 73, 32, 34, 75, 130, 141, 152, 163};
constexpr int kKingMimics[] = {29, 70, 72, 31, 33, 74, 131, 142, 153, 164};
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

// Model slot listener: a kind set up on a floor is normalized for the floor's region.
void Normalize(CMonstorUnit &unit, int model_slot, int model_no) {
    int region = UserStatus != nullptr ? EnemyRegion(selectMapNo, UserStatus->cur_floor) : -1;
    if (region < 0) {
        return;
    }
    EnemyStats stats = NormalizedStats(model_no, region, false);
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
