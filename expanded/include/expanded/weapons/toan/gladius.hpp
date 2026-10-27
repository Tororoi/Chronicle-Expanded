#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// Gladius "Jacket Hunter": a Master Jacket (or its Enhanced form) killed with the Gladius in hand, or with Super
// Steve carrying its SynthSphere, gives four times its ABS.
inline constexpr int kJacketHunterAbs = 4;

// The ABS a kill gives, the monster's own abs as the engine reads it.
int JacketHunterAbs(const CMonstorUnit &unit, int monster, int abs);

} // namespace expanded
