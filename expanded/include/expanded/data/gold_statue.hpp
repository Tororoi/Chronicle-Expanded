#pragma once

// The gold the Midas Touch turns a monster to (weapons/goro/midas_touch.hpp): a polished gold ball, as a reflection map
// (render/draw3d.hpp: Draw3DSetReflecting) draws it, in a rock's pack (gedit/s96/chara/iwa.chr: the rock never drawn)
// in place of its picture, built at extraction into kGoldStatueFile. Includes no game header, so dcdata can build with it.
//
// The ball is kGoldBallSize texels a side: at each point of it the view reflected off it finds a bright sky above
// (kGoldSky), a warm floor below (kGoldFloor) and a dark band at the horizon between (kGoldHorizon, kGoldHorizonWidth)
// over kGoldBase, with a key light up and to the left (kGoldHighlight, kGoldHighlightSize), darkening toward its rim;
// that shine picks its colour along kGoldRamp, deep amber to a white-hot gold.

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
inline constexpr int                               kGoldBallSize = 128;
inline constexpr double                            kGoldSky = 0.85, kGoldFloor = 0.45, kGoldBase = 0.2;
inline constexpr double                            kGoldHorizon = 0.05, kGoldHorizonWidth = 0.18;
inline constexpr double                            kGoldHighlight[2] = {-0.35, 0.4};
inline constexpr double                            kGoldHighlightSize = 0.22;
inline constexpr std::array<std::array<int, 3>, 5> kGoldRamp = {
    {{40, 22, 2}, {130, 84, 14}, {214, 162, 40}, {255, 226, 120}, {255, 250, 220}}
};

// The gold ball as an 8-bit picture on templ's headers (swizzled for an IM2 bank).
Bytes GoldBall(std::span<const unsigned char> templ, bool swizzled);

// The rock's pack with its picture the gold ball.
Bytes BuildGoldStatue(std::span<const unsigned char> rock);

std::vector<DataBuild> GoldStatueBuilds();

} // namespace expanded
