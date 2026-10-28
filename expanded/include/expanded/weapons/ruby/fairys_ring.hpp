#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace dc::game {
class ClsMes;
} // namespace dc::game

namespace expanded {

// Fairy's Ring "Fairy Fortune": while a Fairy's Ring is owned (any bag or the storage), every fish caught pays
// kFairyFortune times the fishing points (FP) it would, but the Garayans, which pay their own. The catch's message shows
// the points paid in red, with the ring's picture after them.
inline constexpr int kFairyFortune = 2;

// The Garayans, by fish kind (fish_info's index; GetFishMsgNo names them).
inline constexpr int                kMardanGarayan = 5;
inline constexpr int                kBaronGarayan = 17;
inline constexpr std::array<int, 2> kGarayans = {kMardanGarayan, kBaronGarayan};

// The catch's message, a town talk message: the fish, its size, the points paid ({value2}) and the total.
inline constexpr int kCatchMessage = 2000;

// Hook: FishingGetAngleFishSize: the FP a catch of fish_kind pays, points the game's.
int FairyFortunePoints(int fish_kind, int points);

// The catch message from its own text: when Fairy Fortune paid the last catch, the points red with their "+", and the
// ring's picture ({icon-sword}, drawn as the ring) after them; otherwise std::nullopt.
std::optional<std::string> FairyFortuneCatchText(std::string_view own);

// The item window mes draws in place of icon code: the Fairy's Ring for the catch message's {icon-sword} when Fairy
// Fortune paid the last catch, else 0.
int FairyFortunePicture(const ClsMes &mes, int code);

} // namespace expanded
