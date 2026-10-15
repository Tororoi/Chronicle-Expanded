#pragma once

namespace expanded {

// The Stronger Enemies toggle's key (config.json's expanded.stronger_enemies).
inline constexpr const char *kStrongerEnemies = "stronger_enemies";

// Whether Stronger Enemies is on: every monster takes the next region's stats.
bool StrongerEnemies();

} // namespace expanded
