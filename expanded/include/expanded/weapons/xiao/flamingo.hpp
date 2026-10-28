#pragma once

namespace expanded {

// Flamingo "Lock-On Distance": with the Flamingo in hand, or a slingshot built from it (Dragon's Y, Divine Beast Title,
// Angel Shooter, Angel Gear), or Super Steve carrying any of their SynthSpheres, monsters can be locked on from
// kLockOnDistance times as far.
inline constexpr float kLockOnDistance = 2.0f;

bool LockOnDistanceActive();

// The active character's lock-on range scale (SetNearLockOnTarget, setTargetCursor).
float LockOnDistanceRate(float rate);

// Flamingo "Bait Flamboyance": while owned (any bag or the storage), every bait's notice radius, the distance from
// which a fish turns toward the hook, is kBaitFlamboyance more for each Flamingo, up to kBaitFlamboyanceMost of them.
inline constexpr float kBaitFlamboyance = 10.0f;
inline constexpr int   kBaitFlamboyanceMost = 3;

// A bait's notice radius, radius the game's, with Bait Flamboyance's units added.
float BaitFlamboyanceRadius(float radius);

} // namespace expanded
