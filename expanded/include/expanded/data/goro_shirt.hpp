#pragma once

// Goro's shirt in the Tall Hammer's turquoise, built into his dungeon pack at extraction (dun/mainchara/c06b.chr,
// rebuilt into kRebuiltDiscFolder) for Colossus: as he grows, his body sheets fade to these copies. Includes no game
// header, so dcdata can build with it.
//
// Each of kGoroShirtSheets has its copy (the same name in kGoroShirtBank, a record the cfg does not load): the shirt's
// texels (GoroShirtTexel) with their hue set to kGoroTurquoiseHue, saturation and lightness their own, every other
// texel as it is; the colours merged back to 256 as the look pages merge them.

#include <array>
#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kGoroDungeonFile = "dun/mainchara/c06b.chr";
inline constexpr const char *kGoroBank = "c06a01.img";
inline constexpr const char *kGoroShirtBank = "c06shirt.img";
inline constexpr const char *kGoroShirtSheets[] = {"c06a02", "c06a03"};
inline constexpr double      kGoroTurquoiseHue = 193.0; // the Tall Hammer's (c06w10)

// Whether a colour (0-255) is the shirt's: its green.
bool GoroShirtTexel(const std::array<int, 3> &rgb);
// A colour in the copy.
std::array<int, 3> GoroShirtColour(const std::array<int, 3> &rgb);

// Goro's dungeon pack with the copies.
Bytes BuildGoroShirt(std::span<const unsigned char> goro);

std::vector<DataBuild> GoroShirtBuilds();

} // namespace expanded
