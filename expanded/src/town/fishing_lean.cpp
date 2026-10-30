#include "expanded/town/fishing_lean.hpp"

#include <algorithm>
#include <cmath>

#include "character.hpp"
#include "expanded/data/fishing_poses.hpp"
#include "expanded/town/fishing_fight.hpp"
#include "framevu1.hpp"
#include "gameutil.hpp"

namespace expanded {

namespace {

struct Lean {
    MotionMix mix;
    float     x = 0.0f, y = 0.0f; // the stick in Toan's frame as the lean follows it
    bool      on = false;
};

Lean lean;

int Index(CFrame *frame, CFrame *joint) {
    return joint == nullptr ? -1 : static_cast<int>(static_cast<CFrameVu1 *>(joint) - static_cast<CFrameVu1 *>(frame));
}

// The model's frames as a skeleton (count of them, their locals from local(j)), the root's place in the world left out.
template <typename LocalOf>
Skeleton ToSkeleton(CFrame *frame, int count, LocalOf local_of) {
    auto    *frames = static_cast<CFrameVu1 *>(frame);
    Skeleton s;
    for (int j = 0; j < count; j++) {
        const int up = Index(frame, frames[j].parent);
        s.parent.push_back(up >= 0 && up < j ? up : -1);
        Mat4 local = {
            {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}
        };
        if (s.parent.back() >= 0) {
            const float (*m)[4] = local_of(j);
            for (std::size_t r = 0; r < 4; r++) {
                for (std::size_t c = 0; c < 4; c++) {
                    local[r][c] = m[r][c];
                }
            }
        }
        s.local.push_back(local);
    }
    return s;
}

} // namespace

LeanShares LeanSharesFor(float x, float y) {
    const float across = std::clamp(x, -1.0f, 1.0f), pulled = std::clamp(y, 0.0f, 1.0f);
    return {.left = pulled * std::max(0.0f, -across),
            .centre = pulled * (1.0f - std::abs(across)),
            .right = pulled * std::max(0.0f, across),
            .relaxed = 1.0f - pulled};
}

LeanStick LeanStickFor(float camera_yaw, float facing_yaw, float x, float y) {
    const float push = std::hypot(x, y);
    if (push == 0.0f) {
        return {.x = 0.0f, .y = 0.0f};
    }
    // Its way across the water, then that way from where he faces: 0 ahead of him, a quarter turn his right.
    const float from_him = facing_yaw - FightStickYaw(camera_yaw, FightStickAxis(x), FightStickAxis(y));
    return {.x = push * std::sin(from_him), .y = -push * std::cos(from_him)};
}

void HoldTheRodAfterMix(CFrame *frame, const MixLocal *played, int count) {
    auto       *frames = static_cast<CFrameVu1 *>(frame);
    auto        at = [&](const char *name) { return Index(frame, frame->SearchFrame(const_cast<char *>(name))); };
    FishingGrip grip = {
        .rod = at(kFishingRod),
        .right = {at(kFishingRightArm.collarbone), at(kFishingRightArm.shoulder), at(kFishingRightArm.elbow),
                  at(kFishingRightArm.wrist)}
    };
    for (int joint : {grip.rod, grip.right.collarbone, grip.right.shoulder, grip.right.elbow, grip.right.wrist}) {
        if (joint < 0 || joint >= count) {
            return;
        }
    }
    Skeleton mixed = ToSkeleton(frame, count, [&](int j) { return frames[j].local; });
    Skeleton as_played = ToSkeleton(frame, count, [&](int j) { return played[j]; });
    HoldTheRod(mixed, as_played, grip);
    for (int joint : {grip.right.collarbone, grip.right.shoulder, grip.right.elbow, grip.right.wrist}) {
        for (std::size_t r = 0; r < 4; r++) {
            for (std::size_t c = 0; c < 4; c++) {
                frames[joint].local[r][c] = static_cast<float>(mixed.local[std::size_t(joint)][r][c]);
            }
        }
        frames[joint].world_valid = false;
    }
}

void LeanRod(CCharacter &angler, float camera_yaw, float stick_x, float stick_y) {
    const MOTION_INFO *left = angler.GetMotionInfo(kFishingPullLeftKey), *right = angler.GetMotionInfo(kFishingPullRightKey);
    const MOTION_INFO *relaxed = angler.GetMotionInfo(kFishingRelaxedKey);
    if (angler.frame == nullptr || left == nullptr || right == nullptr || relaxed == nullptr) {
        return;
    }
    if (!lean.on) {
        lean = {};
        lean.on = true;
    }
    const LeanStick stick = LeanStickFor(camera_yaw, angler.GetRotation()->y, stick_x, stick_y);
    lean.x += (stick.x - lean.x) * kLeanFollow;
    lean.y += (stick.y - lean.y) * kLeanFollow;
    const LeanShares shares = LeanSharesFor(lean.x, lean.y);
    lean.mix.time = {static_cast<float>(left->start), static_cast<float>(kFishingPullFrame), static_cast<float>(right->start),
                     static_cast<float>(relaxed->start)};
    lean.mix.share = {shares.left, shares.centre, shares.right, shares.relaxed};
    lean.mix.target = 1.0f;
    lean.mix.step = 1.0f / static_cast<float>(kLeanFade);
    MixMotion(&angler.motion_type, &lean.mix, HoldTheRodAfterMix);
    MixMotion(&angler.shadow_motion_type, &lean.mix);
}

void EndLean() {
    lean.mix.target = 0.0f;
    lean.on = false;
}

} // namespace expanded
