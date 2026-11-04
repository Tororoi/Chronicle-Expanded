#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// Satan's Ax "Pull of the Underworld": once Goro lets his full charge go with Satan's Ax in hand (GoroKey_Play's smash,
// until it ends), every monster in play within kUnderworldReach of him is drawn toward him, on top of its own step and
// kept by its walls, until its body meets his; while he holds the charge before it, kUnderworldChargeShare as strongly.
// A frame's pull grows exponentially the nearer it is: kUnderworldPullNear at no distance (4 a frame 10 units off, 0.2
// at 40), falling by e every kUnderworldFalloff units, times its weight (the scale hits throw it by: 0 for one no hit
// moves, Opar's and the bosses'). Weak enough far off that a light monster standing beyond kUnderworldSafe is not drawn
// to him before the smash ends (kUnderworldSmashFrames).
inline constexpr float kUnderworldReach = 100.0f;
inline constexpr float kUnderworldChargeShare = 0.25f;
inline constexpr float kUnderworldPullNear = 11.0f;
inline constexpr float kUnderworldFalloff = 10.0f;
inline constexpr float kUnderworldSafe = 60.0f;
inline constexpr int   kUnderworldSmashFrames = 120;

// The pull a monster of weight 1 takes a frame distance from him (0 beyond the reach).
float UnderworldPullAt(float distance);

// How strongly the pull is on, Goro in play with Satan's Ax: 1 while he smashes, kUnderworldChargeShare while he holds
// the charge, 0 otherwise.
float UnderworldStrength();

// A monster's step, once its own movement and the game's body checks have set it (MoveChecMonster): the pull added,
// before its walls are tested. The pull it took the step before is taken back off first, while its step is still the
// one the pull left (its script set no other), so it draws the monster on without gathering speed.
void UnderworldMonsterMove(CMonstorUnit &unit, int monster);

// The floor changed (tests too).
void UnderworldReset();

} // namespace expanded
