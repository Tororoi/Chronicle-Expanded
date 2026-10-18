#pragma once

#include <optional>

#include "expanded/enemies/scaling.hpp"

namespace expanded {

// Bosses' stats by region (expanded/docs/bosses.md): set by hand for each dungeon and Demon Shaft band, a
// boss in its own dungeon keeping its own (but the Black Knight's ABS). Defense and hardness are always
// their own.

// A boss's (or its part's) stats on a floor of region, or none to keep its own.
std::optional<EnemyStats> BossRegionStats(int model_no, int region);

// The damage of one of the Ice Queen's pieces (its own: 69 for the arrow and meteors, 74 for the aura and
// tornadoes) on a floor of region.
int IceQueenPieceDamage(int native_damage, int region);

} // namespace expanded
