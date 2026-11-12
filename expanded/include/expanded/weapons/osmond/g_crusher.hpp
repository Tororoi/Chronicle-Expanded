#pragma once

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// G Crusher "Stopping Power", as the Swallow's (swallow.hpp).
//
// G Crusher "Guard Crusher": with the G Crusher or the Star Breaker in hand, or Super Steve carrying one of their
// SynthSpheres, each of the wielder's hits that a monster's guard would stop has a kGuardCrusherChance chance to break
// through it and land; the monster still raises its guard. A hit rolls once, however many of the monster's guard windows
// or spheres it meets (kGuardRolledMark).
inline constexpr double kGuardCrusherChance = 0.1;

// G Crusher "Giant Crusher": with the G Crusher in hand, or Super Steve carrying its SynthSphere, the weapon's hits on a
// heavy monster deal up to kGiantCrusherDamage times their damage, by how little a hit throws it: 1 + (kGiantCrusherDamage
// - 1) (1 - its knockback scale), so a monster no hit moves (the bosses, the Opar) takes twice, a Golem or a Dragon (half)
// 1.5 times, a Rockanoff (0.8) 1.2 times and one thrown as far as any none more. Plants, rooted as they are,
// are not heavy.
inline constexpr float kGiantCrusherDamage = 2.0f;

bool GuardCrusherActive();
bool GiantCrusherActive();

// Whether a hit that a monster's guard window is about to stop breaks through it instead (CheckDmg, the window open).
bool GuardCrusherBreaks(const CMonstorUnit &unit, int monster, COLLISION_HIT &hit);

// How much a hit's damage on a monster is raised for its weight, 1 for none.
float GiantCrusherRate(const CMonstorUnit &unit, int monster);

// A weapon hit's damage on a monster after its weight bonus (CheckDmg).
float GiantCrusherDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

} // namespace expanded
