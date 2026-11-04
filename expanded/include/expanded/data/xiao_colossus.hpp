#pragma once

// Xiao's Colossus (Super Steve with the Tall Hammer's SynthSphere) look, built at extraction into a pack the game does
// not load (kXiaoColossusFile), its copies read by the look alone (colossus_look.hpp). Includes no game header, so dcdata
// can build with it.
//
//   - kXiaoGrowBank, as she grows: her collar (the cord her bell hangs on, himo) and her rings (lring, rring), drawn
//     from kXiaoTurquoise's tiles on kXiaoSheet, in the Tall Hammer's turquoise, their reds' hue set to
//     kGoroTurquoiseHue, saturation and lightness their own.
//   - kXiaoShrinkBank, as she shrinks: her bell (suzu, drawn from kXiaoBell's two tiles on kXiaoSheet) in silver, its
//     golds as the Tall Hammer's in its silver copy; and, in kSteveShrinkBank, Super Steve's sheet with his hair
//     (kSuperSteveHairTile on kSteveSheet) as Steve's own, the Steve slingshot's (kSteveFile, the same layout) texels
//     there.

#include <array>
#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kXiaoColossusFile = "expanded/look/xiao_colossus.chr";
inline constexpr const char *kXiaoDungeonFile = "dun/mainchara/c04b.chr";
inline constexpr const char *kXiaoBank = "c04b01.img";
inline constexpr const char *kXiaoSheet = "c04b02";
inline constexpr const char *kSuperSteveBank = "c04w13.img";
inline constexpr const char *kSteveSheet = "c04w13";
inline constexpr const char *kSteveFile = "commenu/weapon/c04w04.chr";
inline constexpr const char *kSteveOwnBank = "c04w04.img";
inline constexpr const char *kSteveOwnSheet = "c04w04";
inline constexpr const char *kXiaoGrowBank = "grow.img";
inline constexpr const char *kXiaoShrinkBank = "shrink.img";
inline constexpr const char *kSteveShrinkBank = "steve_shrink.img";

// Tiles (x, y from; x, y to) on their sheets.
inline constexpr int kXiaoTurquoise[2][4] = {
    {0,   128, 64,  160}, // the collar
    {192, 96,  256, 160}  // her rings (lring, rring)
};
inline constexpr int kXiaoBell[2][4] = {
    {0,  64,  64,  128},
    {64, 128, 128, 160}
};
inline constexpr int kSuperSteveHairTile[4] = {28, 78, 92, 99};

// A colour (0-255) at x, y in each copy.
std::array<int, 3> XiaoGrowColour(int x, int y, const std::array<int, 3> &rgb);
std::array<int, 3> XiaoShrinkColour(int x, int y, const std::array<int, 3> &rgb);
// Whether Super Steve's texel at x, y is his hair.
bool SuperSteveHair(int x, int y, const std::array<int, 3> &rgb);

// The look's pack, from Xiao's dungeon pack, Super Steve's and the Steve slingshot's.
Bytes BuildXiaoColossus(std::span<const unsigned char> xiao, std::span<const unsigned char> super_steve, std::span<const unsigned char> steve);

std::vector<DataBuild> XiaoColossusBuilds();

} // namespace expanded
