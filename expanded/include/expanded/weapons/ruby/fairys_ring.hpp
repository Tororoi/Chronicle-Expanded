#pragma once

#include <array>

namespace expanded {

// Fairy's Ring "Fairy Fortune": while a Fairy's Ring is owned (any bag or the storage), every fish caught pays
// kFairyFortune times the fishing points (FP) it would, but the Garayans, which pay their own. The catch's message shows
// the points paid.
inline constexpr int kFairyFortune = 2;

// The Garayans, by fish kind (fish_info's index; GetFishMsgNo names them).
inline constexpr int                kMardanGarayan = 5;
inline constexpr int                kBaronGarayan = 17;
inline constexpr std::array<int, 2> kGarayans = {kMardanGarayan, kBaronGarayan};

// Hook: FishingGetAngleFishSize: the FP a catch of fish_kind pays, points the game's.
int FairyFortunePoints(int fish_kind, int points);

} // namespace expanded
