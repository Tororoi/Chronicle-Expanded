#pragma once

// A posed skeleton's arm bent so its hand takes a frame: the collarbone swung to make room, the shoulder and elbow as
// two bones, the wrist turned. Row vectors (a child's world is its local times its parent's), as the models' frames are.
// Includes no game header, so dcdata can build with it; the game uses it on a model's frames as posed.

#include <vector>

#include "expanded/data/rig_math.hpp"

namespace expanded {

// Each frame's parent (below 0 for none) and local matrix.
struct Skeleton {
    std::vector<int>  parent;
    std::vector<Mat4> local;

    Mat4 World(int joint) const;
    Vec3 Position(int joint) const;
    // Turns a joint by a world rotation about a world point (its own origin for Turn), its children with it.
    void TurnAbout(int joint, const Mat3 &turn, const Vec3 &pivot);
    void Turn(int joint, const Mat3 &turn);
    // Swings a joint the shortest way that brings the point end (a joint under it) to target.
    void Aim(int joint, int end, const Vec3 &target);
    // The direction from a joint to another (its parent: the forearm from the wrist), in the joint's own frame.
    Vec3 BoneIn(int joint, int toward) const;
};

// An arm's frames: collarbone, shoulder, elbow, wrist, each the parent's descendant.
struct ArmJoints {
    int collarbone, shoulder, elbow, wrist;
};

// How short of its full length an arm reaches, and how far (degrees, each way about two axes square to it) its
// collarbone may swing to bring the shoulder where the elbow and wrist want it.
inline constexpr double kReachSlack = 0.05, kCollarboneSwing = 30.0;

// Puts the wrist's frame on target through the arm: the elbow as near as the arm lets it to where forearm (the
// forearm's direction in the wrist's frame) puts it, so the wrist bends as forearm has it, the collarbone swung to let
// it. False when the arm falls short (it then reaches straight for the target, the wrist turned).
bool PlaceHand(Skeleton &skeleton, const ArmJoints &arm, const Mat4 &target, const Vec3 &forearm);

} // namespace expanded
