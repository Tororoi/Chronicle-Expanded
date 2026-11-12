#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Swallow "Wing Clipper": with the Swallow in hand, or Super Steve carrying its SynthSphere, the weapon's hits on flying
// monsters (the sky family: the Hornet, the Phantom, the Cave and Evil Bats, the Crescent Baron) deal kWingClipperDamage
// times their damage, and a flying monster it kills gives kWingClipperAbs times its ABS.
inline constexpr float kWingClipperDamage = 2.0f;
inline constexpr int   kWingClipperAbs = 2;

bool WingClipperActive();

// A weapon hit's damage on a monster after its anti-monster bonus (CheckDmg).
float WingClipperDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int WingClipperAbs(const CMonstorUnit &unit, int monster, int abs);

// Swallow "Stopping Power": each of Osmond's hits that lands with the Swallow, or the G Crusher and the Star Breaker it
// builds into, throws the monster back as a Baselard swing does, a tenth as far (kLightKickBack); Super Steve's pellets
// with one of their SynthSpheres throw the same. Whether a hit is one of them.
bool StoppingPowerHit(const COLLISION_HIT &record);

} // namespace expanded
