#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Battle Ax "Woodcutter": with the Battle Ax in hand, or Super Steve carrying its SynthSphere, the weapon's hits on
// plants deal kWoodcutterDamage times their damage, and a plant it kills gives kWoodcutterAbs times its ABS.
inline constexpr float kWoodcutterDamage = 2.0f;
inline constexpr int   kWoodcutterAbs = 2;

// A weapon hit's damage on a monster after its anti-monster bonus (CheckDmg).
float WoodcutterDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int WoodcutterAbs(const CMonstorUnit &unit, int monster, int abs);

} // namespace expanded
