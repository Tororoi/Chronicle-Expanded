#pragma once

// Goro in the Tall Hammer's colours, built into his dungeon pack at extraction (dun/mainchara/c06b.chr, rebuilt into
// kRebuiltDiscFolder) for Colossus: as he grows, his body sheets fade to their gold-and-turquoise copies and the gold
// parts take the game's frame tint. Includes no game header, so dcdata can build with it.
//
//   - The copies: each of kGoroGoldSheets in its gold copy (the same name in kGoroGoldBank, a record the cfg does not
//     load), texel by texel by region (GoroRegionOf): the gold its hue set to kGoroGoldHue, the turquoise to
//     kGoroTurquoiseHue, saturation and lightness their own, the rest as it is; the colours merged back to 256 as the
//     look pages merge them.
//   - The gold frame: the body (kGoroBody) split by triangle, each by the region most of its texels are in: the gold
//     ones into a frame of their own (kGoroGoldBody) beside it, the same vertices and weights, drawn from its own vertex
//     buffer, so the gold can be tinted alone. The cape (kGoroCape) is all gold as it is.

#include <array>
#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kGoroDungeonFile = "dun/mainchara/c06b.chr";
inline constexpr const char *kGoroBank = "c06a01.img";
inline constexpr const char *kGoroGoldBank = "c06gold.img";
inline constexpr const char *kGoroGoldSheets[] = {"c06a02", "c06a03"};
inline constexpr const char *kGoroBody = "skin";
inline constexpr const char *kGoroGoldBody = "goldskin";
inline constexpr const char *kGoroCape = "mant__A7F5";
inline constexpr const char *kGoroWrapsSheet = "c06a02";
inline constexpr int         kGoroWraps[4] = {96, 144, 160, 224};            // the tan wraps: x, y from; x, y to
inline constexpr double      kGoroGoldHue = 42.0, kGoroTurquoiseHue = 193.0; // the Tall Hammer's (c06w10)

// A texel's region by its colour (0-255) and place: green the shirt; the browns (fur, belt) the gold; flesh, the red
// strip, bone, metal, the blackest lines and the tan wraps kept.
enum class GoroRegion {
    Kept,
    Gold,
    Turquoise
};
GoroRegion GoroRegionOf(const char *sheet, int x, int y, const std::array<int, 3> &rgb);
// A colour in its region's copy.
std::array<int, 3> GoroGoldColour(GoroRegion region, const std::array<int, 3> &rgb);

// Goro's dungeon pack with the copies and the gold frame.
Bytes BuildGoroGold(std::span<const unsigned char> goro);

std::vector<DataBuild> GoroGoldBuilds();

} // namespace expanded
