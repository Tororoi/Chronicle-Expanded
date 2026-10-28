#pragma once

#include <vector>

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

struct MotionLayer;

// Her legs while she moves firing (moves_while_firing.hpp): Xiao's hips and legs take the walk or lock-on strafe the game
// plays for her move, at the game's own rate, under her slingshot action, and her upper body holds the shot's pose in the
// world as the hips sway under it (a MotionLayer below jnt2_1, laid over her model's motion and her shadow's). The legs
// fade in and out over kLegFadeFrames as she starts and stops; a step under way when a shot starts carries on under it,
// and on into the move when the shot ends. The step's foot sounds play from the legs' clock.

inline constexpr int kLegFadeFrames = 10;

// The move MoveChara plays for a step, and its rate: yaw her facing, move_x / move_z her move and stick its strength
// (BtActStatus), locked whether she is locked on, lx / ly the stick as keyCtrl reads it, run_rate the run's KEY rate.
// Facing the way she moves, unlocked, keyCtrl's run or walk; locked on, the forward strafe; aside or back, the strafes at
// the lock-on rate (unlocked, the game turns her to her move instead, and a shot keeps her facing).
struct LegMove {
    int   key;
    float rate;
};

LegMove GameLegMove(float yaw, float move_x, float move_z, float stick, bool locked, float lx, float ly, float run_rate);

// The frames of set_no's foot sounds and sound events (the cfg's FOOT and EVENT lines) that a step of a clock from one
// frame to another passes, by CCharacter::Step's rule: none for a step of a frame or more, or back (a loop).
std::vector<int> SoundFramesPassed(const CCharacter &chara, int set_no, float from, float to);

// Hook: motionDrive, the player's motion about to step (PlayerMotionStepping).
void FiringLegsStep(CCharacter &chara, int chara_no);

// The legs laid over her motion now, the key they play, or nullptr and -1.
const MotionLayer *FiringLegs();
int                FiringLegsKey();

} // namespace expanded
