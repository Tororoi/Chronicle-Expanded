#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Snail "Slime Trail": with the Snail in Osmond's hand, or Super Steve carrying its SynthSphere, each hit of the wielder's
// that takes life off a monster and leaves it standing has a kSlimeTrailChance chance to goo it, as a hit carrying goo does
// (GiveMonsterAilments: not a monster immune to ailments, nor one frozen, poisoned or angry).
inline constexpr double kSlimeTrailChance = 0.05;

// Hook: a monster's life taken by a hit (CheckDmg), amount the damage.
void SnailMonsterDamaged(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);

} // namespace expanded
