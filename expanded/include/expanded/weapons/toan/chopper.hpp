#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Chopper "Utility Knife": with the Chopper in hand, or Super Steve carrying its SynthSphere, the weapon's hits on
// organic monsters deal kUtilityKnifeDamage times their damage, and an organic monster it kills gives kUtilityKnifeAbs times
// its ABS (rounded). Organic: every beast (Dran, Master Utan and Minotaur Joe among them) and every plant.
inline constexpr float kUtilityKnifeDamage = 1.5f;
inline constexpr float kUtilityKnifeAbs = 1.5f;

bool UtilityKnifeActive();

// Whether a monster is organic.
bool Organic(const CMonstorUnit &unit, int monster);

// A weapon hit's damage on a monster after its anti-monster bonus (CheckDmg).
float UtilityKnifeDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int UtilityKnifeAbs(const CMonstorUnit &unit, int monster, int abs);

} // namespace expanded
