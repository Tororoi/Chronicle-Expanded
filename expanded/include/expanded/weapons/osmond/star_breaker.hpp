#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Star Breaker "Shooting Stars": with the Star Breaker in Osmond's hand, each hit of his that lands on a monster, taking
// life or stopped by its guard, has a kShootingStarChance chance to send a shooting star (star_breaker/shooting_stars.hpp),
// which comes to lie in front of him as an empty SynthSphere for the taking. Super Steve carrying its SynthSphere gives
// each of Xiao's hits a kSuperSteveStarChance chance; as every Osmond sphere does, it also lets her move while she shoots
// (xiao/moves_while_firing.hpp). A hit rolls once, however many of a monster's spheres or monsters it meets
// (kStarRolledMark).
inline constexpr double kShootingStarChance = 0.001;
inline constexpr double kSuperSteveStarChance = 0.005;

// The chance a hit of owner's (COLLISION_HIT::owner) rolls with the weapon now in hand: 0 for another's.
double ShootingStarChance(int owner);

// Hook: a hit that landed on a monster (CheckDmg), taken or blocked by its guard.
void StarBreakerHitLanded(const CMonstorUnit &unit, int monster, COLLISION_HIT &hit, bool blocked);

} // namespace expanded
