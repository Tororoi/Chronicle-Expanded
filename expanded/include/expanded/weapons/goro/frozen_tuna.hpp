#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Frozen Tuna "Cold Storage", with the Frozen Tuna in Goro's hand, or Super Steve carrying its SynthSphere in Xiao's:
//   - Each WHP the weapon loses banks kColdStorageHealPerWhp life into a pool. Once the wielder takes damage, the pool
//     drains into his life, a point every kColdStorageDrainFrames frames the monsters step, while his life is above 0
//     and under its maximum (paused there, the pool kept). Damage taken with nothing banked starts no drain. A repair
//     (the WHP rising), the weapon leaving the hand of the member in play and a new dungeon visit empty the pool. Goro's
//     weapon and Super Steve's sphere each have a pool of their own.
//   - Each hit of the wielder's that takes life off a monster has a kColdStorageChance chance to stop every monster in
//     play on the floor (stepping near the player and out of any chest) but the ice ones (Blizzard, Sam and Ice Gemron)
//     for kColdStorageMonsterFrames, as the game's freeze does (no poison, goo or anger beside it), and to freeze the
//     wielder for kColdStorageWielderFrames through the game's own freeze (BtSetStatusErr), which a hit on him breaks.
//     The freeze holds him in the pose it caught him in, a swing too: the frozen wielder's hits land nothing. Each body
//     it freezes stands in the Ice Queen's prison (frozen_tuna/ice_prisons.hpp) while its freeze lasts.
inline constexpr float  kColdStorageHealPerWhp = 1.0f;
inline constexpr int    kColdStorageDrainFrames = 30;
inline constexpr double kColdStorageChance = 0.05;
inline constexpr int    kColdStorageMonsterFrames = 300;
inline constexpr int    kColdStorageWielderFrames = 180;

// The life banked in Goro's pool, or in Super Steve's (sphere).
float ColdStorageBanked(bool sphere);

// Hooks: a dungeon step (the pools, the prisons), a monster's life taken by a hit (CheckDmg; amount the damage), whether
// a monster refuses a hit (CheckDmg), the effects' draw, the floor's monsters put back, and a dungeon visit beginning.
void ColdStorageStep();
void ColdStorageMonsterDamaged(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);
bool ColdStorageHitRefused(const COLLISION_HIT &hit);
void ColdStorageDraw();
void ColdStorageCleared();
void ColdStorageVisitBegins();

} // namespace expanded
