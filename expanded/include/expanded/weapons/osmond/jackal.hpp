#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Jackal "Predator": with the Jackal in hand, or Super Steve carrying its SynthSphere, the weapon's hits on beasts deal
// kPredatorDamage times their damage, and a beast it kills gives kPredatorAbs times its ABS. Beasts: every beast-family
// monster (Dran, Master Utan, Minotaur Joe and King Prickly among them).
inline constexpr float kPredatorDamage = 2.0f;
inline constexpr int   kPredatorAbs = 2;

bool PredatorActive();

// A weapon hit's damage on a monster after its anti-monster bonus (CheckDmg).
float PredatorDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int PredatorAbs(const CMonstorUnit &unit, int monster, int abs);

} // namespace expanded
