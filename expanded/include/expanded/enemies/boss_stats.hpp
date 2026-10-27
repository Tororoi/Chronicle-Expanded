#pragma once

#include <optional>

#include "expanded/enemies/scaling.hpp"

namespace expanded {

// Bosses' stats by region (expanded/docs/bosses.md): set by hand for each dungeon and Demon Shaft band, a
// boss in its own dungeon keeping its own (but the Black Knight's ABS). Defense and hardness are always
// their own.

// A boss's (or its part's) stats on a floor of region, or none to keep its own.
std::optional<EnemyStats> BossRegionStats(int model_no, int region);

// The Ice Queen's pieces' own damage: her arrow's and meteor's, and her aura's and tornadoes'.
inline constexpr int kIceQueenArrowDamage = 69;
inline constexpr int kIceQueenAuraDamage = 74;

// The damage of one of the Ice Queen's pieces (its own, one of those) on a floor of region.
int IceQueenPieceDamage(int native_damage, int region);

} // namespace expanded
