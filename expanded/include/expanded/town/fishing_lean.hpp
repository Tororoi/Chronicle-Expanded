#pragma once

#include "expanded/core/motion_mix.hpp"

namespace dc::game {
class CCharacter;
class CFrame;
} // namespace dc::game

namespace expanded {

// In a fishing fight Toan's rod leans with the stick, as the screen shows it: the stick points across the water as the
// fight reads it (fishing_fight.hpp's FightStickYaw, the camera's frame) and is turned into his own frame as he faces
// (LeanStickFor), so the rod leans the way it points on screen whichever way the camera looks. His fishing motion (and
// its shadow) is posed as a mix of the fight poses (data/fishing_poses.hpp), the stick across him (x) taking the pull
// from the left lean (KEY 16) through the disc's pull (KEY 12 at its frame 360) to the right lean (KEY 17), the stick
// back toward him (y) from the relaxed stance (KEY 18) to that pull. The lean follows the stick kLeanFollow of the way each frame, and fades in and out over kLeanFade frames as a
// fight starts and ends. After each mix the right hand is put back on the rod, which a joint-by-joint mix lets slide.
// Without the poses in the data (a model built before them), the fight plays the disc's pull.
inline constexpr float kLeanFollow = 0.25f;
inline constexpr int   kLeanFade = 10;

// Each pose's share for a stick at (x, y) (as GetLXf and GetLYf: right and down, pulled back, positive), summing to 1.
struct LeanShares {
    float left, centre, right, relaxed;
};

LeanShares LeanSharesFor(float x, float y);

// The stick in Toan's frame: x to his right, y back (behind him), as far as it is pushed as read.
struct LeanStick {
    float x, y;
};

// A stick read at (x, y) (as GetLXf and GetLYf) with the camera looking camera_yaw, for Toan facing facing_yaw (yaws as
// atan2(x, z)).
LeanStick LeanStickFor(float camera_yaw, float facing_yaw, float x, float y);

// Hooks: a fight's frame, with the camera looking camera_yaw and the stick as read (the lean begins on the first); the
// fight over (the lean fades out).
void LeanRod(dc::game::CCharacter &angler, float camera_yaw, float stick_x, float stick_y);
void EndLean();

// The mix's fix on the fishing model: the right hand back on the rod, its wrist where the motion as played holds it.
void HoldTheRodAfterMix(dc::game::CFrame *frame, const MixLocal *played, int count);

} // namespace expanded
