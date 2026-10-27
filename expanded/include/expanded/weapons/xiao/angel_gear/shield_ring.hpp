#pragma once

#include "expanded/weapons/xiao/angel_gear/shield_prop.hpp"

namespace expanded::angel_gear {

// The shield, while the slingshot stands:
//   - Every monster spawned with life farther than kRingRadius from Xiao finds her at its own point of a ring of that
//     radius about her (along the line to it), so it walks straight at her and stops and swings on reaching the slingshot;
//     one nearer finds her where she is. A monster's body stops kRingRadius from her, not the game's 6 units.
//   - A monster's swing (a hit to hurt her that is not a shot's) reaching the slingshot (kPropHitRadius about its grip, from
//     kPropHitBelow under it to kPropHitHeight over) is spent on it as on a guard: the guard's spark where it touched and the
//     guard's clink, and the shield takes the strike; for kStrikeFrames after, no swing is looked at (the mod's 200 ms). A
//     shot's blast reaching it is spent silently, as is every swing while it clears.
inline constexpr float kPropHitRadius = 6.0f, kPropHitHeight = 14.0f, kPropHitBelow = 2.0f;
inline constexpr float kRingRadius = kPropAhead + kPropHitRadius;
inline constexpr int   kStrikeFrames = 12;

void RingUp(bool up);
// The swings and blasts reaching the slingshot spent; whether one struck it (never while quiet).
bool SpendOnShield(bool quiet);
// A frame passing: the wait after a strike.
void RingStep();
void RingReset();

// Hooks: where a monster's script finds the player (_GET_POSITION, _GET_DISTANCE), true when the ring sets it; how far
// from her a monster's body stops (CMonstorUnit::MoveCheck2), radius the game's.
bool  RingTarget(int monster, float *position);
float RingBlockRadius(float radius);

} // namespace expanded::angel_gear
