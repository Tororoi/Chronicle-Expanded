#pragma once

// Toan's fishing fight poses, built from his fishing model at extraction. Includes no game header, so dcdata can build
// with it.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

// Toan's fishing model, which the game reads from the rebuilt disc folder (kRebuiltDiscFolder) once it is built.
inline constexpr const char *kFishingModelFile = "chara/c01d_turi.chr";

// The poses, each a KEY after the disc's sixteen holding one frame (keyed on it and the next, speed 0), on the next
// multiple of ten after the motion's last keyed frame (370) and ten apart. Toan faces +Z, his left is +X.
// - kFishingPullLeftKey, kFishingPullRightKey: KEY 12's pull at kFishingPullFrame, the spine (jnt2_1) turned
//   kFishingTurn degrees and leant kFishingLean degrees to that side, the neck (jnt11_1) turned kFishingHeadBack back,
//   the rod (turizao) turned with the spine about the spine's joint, then leant kFishingRodLean more about the right
//   hand.
// - kFishingRelaxedKey: the wait for a bite (KEY 5) at kFishingWaitFrame, the rod raised kFishingRodRaise degrees
//   about the right hand and the left arm swung to keep its hand where the rod took it.
// The shadow motion takes the same turns on its own joints.
inline constexpr int    kFishingPullLeftKey = 16, kFishingPullRightKey = 17, kFishingRelaxedKey = 18;
inline constexpr int    kFishingPullFrame = 360, kFishingWaitFrame = 219;
inline constexpr double kFishingTurn = 25.0, kFishingLean = 12.0, kFishingHeadBack = 12.0, kFishingRodLean = 10.0;
inline constexpr double kFishingRodRaise = 40.0;

// The fishing model with the poses, from the disc's.
Bytes BuildFishingPoses(std::span<const unsigned char> model);

// The fishing model.
std::vector<DataBuild> FishingPoseBuilds();

} // namespace expanded
