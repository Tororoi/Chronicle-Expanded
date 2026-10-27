#pragma once

namespace dc::game {
class CCharacter;
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// How much faster monsters move and play their motions with Faster Enemies on.
inline constexpr float kFasterMoveScale = 2.0f;
inline constexpr float kFasterMotionScale = 2.0f;

// Frames either side of an attack's hit window that keep the motion's own rate.
inline constexpr float kFasterHitWindowMargin = 1.0f;

// Whether the Faster Enemies option is on.
bool FasterEnemiesOn();

// A _SET_MOVE speed, scaled with Faster Enemies on (a summoned ally's, ally_unit.hpp, never).
float FasterMoveSpeed(float speed);

// Whether monster_no's motion is at frame time of one of its hit windows, give or take the margin.
bool FasterInHitWindow(const CMonstorUnit &unit, int monster_no, float time);

// Steps a monster's character's motion, faster outside its hit windows with Faster Enemies on (a summoned ally's never).
void FasterMotionStep(CMonstorUnit &unit, int monster_no, CCharacter &character);

} // namespace expanded
