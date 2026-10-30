#pragma once

// Toan's fishing fight poses, built from his fishing model at extraction. Includes no game header, so dcdata can build
// with it.

#include <span>
#include <vector>

#include "expanded/data/arm_reach.hpp"
#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

// Toan's fishing model, which the game reads from the rebuilt disc folder (kRebuiltDiscFolder) once it is built.
inline constexpr const char *kFishingModelFile = "chara/c01d_turi.chr";

// The poses, each a KEY after the disc's sixteen holding one frame (keyed on it and the next, speed 0), on the next
// multiple of ten after the motion's last keyed frame (370) and ten apart, in key order. Toan faces +Z, his left is +X.
// The fight's pull, KEY 12 at kFishingPullFrame (the rod in his right hand alone), is the centre between the leans.
// - kFishingPullLeftKey, kFishingPullRightKey: the pull, the spine turned kFishingTurn degrees and leant kFishingLean
//   degrees to that side, the neck turned kFishingHeadBack back, the rod turned with the spine about the spine's joint,
//   then leant kFishingRodLean more about the right wrist.
// - kFishingRelaxedKey: the wait for a bite (KEY 5) at kFishingWaitFrame, the rod raised kFishingRodRaise degrees about
//   the right wrist, the left arm (kFishingLeftArmRoot and under it) as in the pull.
// In each the right hand holds the rod where the disc's motion keeps it, through the arm (arm_reach.hpp: the elbow put
// where the wrist bends as in the pose's own frame of the disc, the collarbone swung to let it), and the rod's bend is
// the pull's. The shadow motion takes the same turns on its own joints.
inline constexpr int    kFishingPullLeftKey = 16, kFishingPullRightKey = 17, kFishingRelaxedKey = 18;
inline constexpr int    kFishingPullFrame = 360, kFishingWaitFrame = 219;
inline constexpr double kFishingTurn = 25.0, kFishingLean = 12.0, kFishingHeadBack = 12.0, kFishingRodLean = 10.0;
inline constexpr double kFishingRodRaise = 40.0;

// The fishing model's frames the poses move: the spine, the neck, the rod (a frame of its own under the root, which the
// motion keeps in the right hand), the right arm (collarbone, shoulder, elbow, wrist) and the left arm's first frame.
struct FishingArm {
    const char *collarbone, *shoulder, *elbow, *wrist;
};

inline constexpr const char *kFishingSpine = "jnt2_1", *kFishingNeck = "jnt11_1", *kFishingRod = "turizao";
inline constexpr FishingArm  kFishingRightArm = {"jnt16_1", "jnt17_1", "jnt17_2", "jnt18_1"};
inline constexpr const char *kFishingLeftArmRoot = "chn8";

// A posed fishing model's frames the right hand's grip needs: the rod and the right arm.
struct FishingGrip {
    int       rod;
    ArmJoints right;
};

// The right hand put back on the rod in a pose mixed from others (each holding it as the disc does): the wrist where the
// motion as played (played) holds it on the rod, reached through the arm with its wrist bent as mixed. False when the arm
// falls short.
bool HoldTheRod(Skeleton &mixed, const Skeleton &played, const FishingGrip &grip);

// The fishing model with the poses, from the disc's.
Bytes BuildFishingPoses(std::span<const unsigned char> model);

// The fishing model.
std::vector<DataBuild> FishingPoseBuilds();

} // namespace expanded
