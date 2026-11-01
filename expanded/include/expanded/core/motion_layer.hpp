#pragma once

namespace dc::game {
class CFrame;
struct MOTION_STATE;
struct tagMOTION_TYPE;
} // namespace dc::game

namespace expanded {

// A second clock over a motion's lower body. SetMotionEX poses every joint from the motion's own state; the joints outside
// the seam's subtree then take the same tracks read at the layer's frame instead, blended in by weight, and the seam (with
// the joint under it, when given) turns so the upper body keeps the pose the motion gave it in the world, or rides the
// layered hips as far as torso_turn says.
struct MotionLayer {
    const char *seam = nullptr;      // the first joint of the upper body: it and everything under it keep the motion's pose
    const char *seam_next = nullptr; // the joint under the seam that shares its turn, or nullptr for the seam alone
    float       torso_turn = 0.0f;   // 0: the upper body holds its pose in the world; 1: it turns with the layered hips
    float       time = 0.0f;         // the frame the lower body takes
    float       from_time = -1.0f;   // a frame the lower body cross-fades from, held still; below 0 for none
    float       from_weight = 1.0f;  // how far from from_time toward time (1: all the way)
    float       weight = 0.0f;       // how much of the layer the lower body takes over the motion's own pose
    int         carry_key = -1;      // a key the motion is handed: its cross-fade arrives at carry_frame, not the key's start
    float       carry_frame = 0.0f;
};

// Lays layer over motion wherever SetMotionEX poses it (a model's motion and its shadow's take one each), until lifted.
// Lifting puts back the joint under the seam when no track drives it; restore false leaves it, for a model that is gone.
void LayMotion(tagMOTION_TYPE *motion, MotionLayer *layer);
void LiftMotion(tagMOTION_TYPE *motion, bool restore);

// Hooks: SetMotionEX, before the motion's state moves on (a key change takes the carry), and once its tracks have posed
// frame (before the look-at and the skin), where a motion mix (motion_mix.hpp) goes first.
void MotionPosing(tagMOTION_TYPE *motion, MOTION_STATE *state);
void MotionPosed(CFrame *frame, tagMOTION_TYPE *motion);

} // namespace expanded
