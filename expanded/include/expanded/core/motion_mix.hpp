#pragma once

#include <array>

namespace dc::game {
class CFrame;
struct tagMOTION_TYPE;
} // namespace dc::game

namespace expanded {

// A frame's local matrix, as the model's frames hold them (row vectors: a child's world is its local times its parent's).
using MixLocal = float[4][4];

// A motion posed from a mix of its own frames instead of the frame it plays: every joint a rotation or translation
// track drives takes its tracks read at each of the mix's times, blended by their shares (rotations slerped, positions
// lerped), then weight of the way from the pose the motion played; its vertex tracks are read at the time with the
// largest share once weight is past half. weight heads for target by step each pose; a mix whose weight is back at 0
// with a target of 0 lets go of its motion.
struct MotionMix {
    static constexpr int kTimes = 4;

    std::array<float, kTimes> time{};
    std::array<float, kTimes> share{};
    float                     target = 0.0f;
    float                     step = 1.0f;
};

// After mixing: frame's frames as mixed, and as the motion played them (count frames, by their place in the block).
using MixFix = void (*)(dc::game::CFrame *frame, const MixLocal *played, int count);

// Mixes motion by mix (each motion its own weight, from 0), fix (if any) run after each mix; until it lets go or is
// unmixed. A motion whose tracks change (the model reloaded) is let go of.
void MixMotion(dc::game::tagMOTION_TYPE *motion, const MotionMix *mix, MixFix fix = nullptr);
void UnmixMotion(dc::game::tagMOTION_TYPE *motion);
bool MotionMixed(const dc::game::tagMOTION_TYPE *motion);

// Hook: SetMotionEX, once its tracks have posed frame.
void MixPosed(dc::game::CFrame *frame, dc::game::tagMOTION_TYPE *motion);

} // namespace expanded
