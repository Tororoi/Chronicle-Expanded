#pragma once

#include <array>

namespace expanded::fish {

// Fish kinds: fish_info's index (8 is none).
inline constexpr int kBobo = 0, kGobbler = 1, kNonky = 2, kKaiji = 3, kBakuBaku = 4, kMardanGarayan = 5, kGummy = 6, kNiler = 7;
inline constexpr int kUmadakara = 9, kTarton = 10, kPiccoly = 11, kBon = 12, kHamahama = 13, kNegie = 14, kDen = 15, kHeela = 16;
inline constexpr int kBaronGarayan = 17;
inline constexpr int kKindCount = 18;

// The Garayans (GetFishMsgNo names them).
inline constexpr std::array<int, 2> kGarayans = {kMardanGarayan, kBaronGarayan};

} // namespace expanded::fish
