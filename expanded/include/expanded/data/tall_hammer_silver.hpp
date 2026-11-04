#pragma once

// The Tall Hammer in silver, built into its pack at extraction (commenu/weapon/c06w10.chr, rebuilt into
// kRebuiltDiscFolder) for Colossus: as Goro shrinks below his own size, the hammer's sheet fades to this copy. Includes
// no game header, so dcdata can build with it.
//
// The sheet's copy (the same name in kTallHammerSilverBank, a record the cfg does not load): its gold texels
// (TallHammerGoldTexel) as silver, kTallHammerSilverHue at kTallHammerSilverSaturation, their lightness their own; its
// turquoise (TallHammerTurquoiseTexel) dulled, its hue its own at kTallHammerDullSaturation of its saturation and
// kTallHammerDullLightness of its lightness; the rest as it is; the colours merged back to 256 as the look pages merge
// them.

#include <array>
#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kTallHammerFile = "commenu/weapon/c06w10.chr";
inline constexpr const char *kTallHammerBank = "c06w10.img";
inline constexpr const char *kTallHammerSheet = "c06w10";
inline constexpr const char *kTallHammerSilverBank = "c06silver.img";
inline constexpr double      kTallHammerSilverHue = 210.0, kTallHammerSilverSaturation = 8.0; // a cool grey (degrees, percent)
inline constexpr double      kTallHammerDullSaturation = 0.35, kTallHammerDullLightness = 0.9;

// Whether a colour (0-255) is the hammer's gold, or its turquoise, and a colour in the copy.
bool               TallHammerGoldTexel(const std::array<int, 3> &rgb);
bool               TallHammerTurquoiseTexel(const std::array<int, 3> &rgb);
std::array<int, 3> TallHammerSilverColour(const std::array<int, 3> &rgb);

// The hammer's pack with the copy.
Bytes BuildTallHammerSilver(std::span<const unsigned char> hammer);

std::vector<DataBuild> TallHammerSilverBuilds();

} // namespace expanded
