#pragma once

#include "dungeon.h"

#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/scaling.hpp"

namespace expanded {

// Demon Shaft's bands of floors.
inline constexpr int kDemonShaftBands = 5;
inline constexpr int kDemonShaftBandFloors = 20;

// Regions monsters are normalized to: the first six dungeons, then Demon Shaft's bands.
inline constexpr int kEnemyRegions = DUNGEON_DEMON_SHAFT + kDemonShaftBands;

// The band of a Demon Shaft floor (UserStatus->cur_floor, 0 for its first): 0 to kDemonShaftBands - 1.
int DemonShaftBand(int floor);

// The region of a dungeon's floor (UserStatus->cur_floor, 0 for its first), or -1 outside the seven dungeons.
int EnemyRegion(int dungeon, int floor);

// The stats normalization bounds.
enum class EnemyStat { Hp, Abs, Defense, Hardness, Melee, Shot };

// A kind's stat in a region: one from an earlier region is lifted to the region's average, one from a later
// region capped at the highest so far, then look-alikes kept in order; the region's own monsters, mimics,
// bosses and their parts keep theirs. stronger: Stronger enemies' scaling on top (ABS stays).
int NormalizedStat(int model_no, EnemyStat stat, int region, bool stronger);

// A kind's stats on a floor of region: its native ones for the region's own monsters (unless stronger).
EnemyStats NormalizedStats(int model_no, int region, bool stronger);

// The same, for a named monster.
inline int NormalizedStat(Monster monster, EnemyStat stat, int region, bool stronger) {
    return NormalizedStat(No(monster), stat, region, stronger);
}
inline EnemyStats NormalizedStats(Monster monster, int region, bool stronger) {
    return NormalizedStats(No(monster), region, stronger);
}

} // namespace expanded
