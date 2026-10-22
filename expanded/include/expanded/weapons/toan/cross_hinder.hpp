#pragma once

class CMonstorUnit;
struct COLLISION_HIT;

namespace expanded {

// Cross Hinder "Sanctifier": with the Cross Hinder in hand, or Super Steve carrying its SynthSphere, the weapon's hits
// on undead deal kSanctifierDamage times their damage, an undead it kills gives kSanctifierAbs times its ABS, and an
// undead that would get back up after a kill stays down. The wielder also locks on from kHolyReach times as far, as
// they do with the blades the Cross Hinder builds into (Big Bang, the Sword of Zeus).
inline constexpr float kSanctifierDamage = 2.0f;
inline constexpr int   kSanctifierAbs = 2;
inline constexpr float kHolyReach = 2.0f;

bool SanctifierActive();
bool HolyReachActive();

// A weapon hit's damage on a monster after its anti-monster bonus (CheckDmg).
float SanctifierDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// The ABS a kill gives, the monster's own abs as the engine reads it.
int SanctifierAbs(const CMonstorUnit &unit, int monster, int abs);

// The active character's lock-on range scale (SetNearLockOnTarget, setTargetCursor).
float HolyReachRate(float rate);

} // namespace expanded
