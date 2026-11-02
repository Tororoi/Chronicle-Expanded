#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Chopper "Butcher's Edge": with the Chopper in hand, or Super Steve carrying its SynthSphere, the weapon's hits on
// organic monsters deal kButchersDamage times their damage, and an organic monster it kills gives kButchersAbs times
// its ABS (rounded). Organic: every beast (Dran, Master Utan and Minotaur Joe among them) and every plant.
inline constexpr float kButchersDamage = 1.5f;
inline constexpr float kButchersAbs = 1.5f;

bool ButchersEdgeActive();

// Whether a monster is organic.
bool Organic(const CMonstorUnit &unit, int monster);

// A weapon hit's damage on a monster after its anti-monster bonus (CheckDmg).
float ButchersEdgeDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int ButchersEdgeAbs(const CMonstorUnit &unit, int monster, int abs);

} // namespace expanded
