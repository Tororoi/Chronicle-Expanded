#pragma once

class CMonstorUnit;

namespace expanded {

// Javelin "Marine Hunter": with the Javelin in hand, or Super Steve carrying its SynthSphere, every marine monster has
// none of its defense: no defense taken off a hit's damage and no hardness wearing the weapon that hits it. A marine
// monster killed then gives kMarineHunterAbs times its ABS.
inline constexpr int kMarineHunterAbs = 2;

bool MarineHunterActive();

// A monster's defense and hardness as CheckDmg reads them, its own as the arguments.
int MarineHunterDefense(const CMonstorUnit &unit, int monster, int defense);
int MarineHunterHardness(const CMonstorUnit &unit, int monster, int hardness);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int MarineHunterAbs(const CMonstorUnit &unit, int monster, int abs);

} // namespace expanded
