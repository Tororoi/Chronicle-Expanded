#pragma once

// The gold the Midas Touch turns a monster to (weapons/goro/midas_touch.hpp): a rock's pack (gedit/s96/chara/iwa.chr)
// with its picture made polished gold, built at extraction into kGoldStatueFile. The monster is drawn with the picture
// in place of its own; the pack's rock is never drawn. Includes no game header, so dcdata can build with it.
//
// Each texel's lightness, its contrast stretched from kGoldLow to kGoldHigh, picks its colour along kGoldRamp (deep
// amber, gold, a pale highlight), so the rock's grain reads as hammered metal.

#include <array>
#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char                       *kGoldStatueSource = "gedit/s96/chara/iwa.chr";
inline constexpr const char                       *kGoldStatueFile = "expanded/effect/gold_statue.chr";
inline constexpr const char                       *kGoldStatueCfg = "info.cfg";
inline constexpr const char                       *kGoldStatuePicture = "gldstat";
inline constexpr double                            kGoldLow = 0.02, kGoldHigh = 0.40;
inline constexpr std::array<std::array<int, 3>, 3> kGoldRamp = {
    {{96, 58, 6}, {214, 166, 44}, {255, 238, 156}}
};

// A texel's colour as gold.
std::array<int, 3> GoldOf(const std::array<int, 3> &rgb);

std::vector<DataBuild> GoldStatueBuilds();

} // namespace expanded
