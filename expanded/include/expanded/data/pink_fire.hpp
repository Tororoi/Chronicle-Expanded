#pragma once

// Satan's Ax's fire: the monsters' fireball (dun/effect/f_boll_3.chr, the rising flame and Xiao's fireball pellet:
// inferno/rising_flame.hpp) and Mr. Blare's blast (dun/effect/zibaku_f2.chr: inferno/smash_blast.hpp) turned from
// orange to pink red, built at extraction into kPinkFireballFile and kPinkBlastFile. Includes no game header, so dcdata
// can build with it.
//
// Each is drawn from its one 8-bit picture: every texel's hue turned by kPinkFireHueTurn, its saturation and lightness
// its own, the colours merged back to 256 as the look pages merge them, and the picture renamed (the meshes' materials
// with it), so it is not taken for the orange one when both are loaded.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kPinkFireballSource = "dun/effect/f_boll_3.chr";
inline constexpr const char *kPinkFireballFile = "expanded/effect/pink_fireball.chr";
inline constexpr const char *kPinkFireballPicture = "pkfball";
inline constexpr const char *kPinkBlastSource = "dun/effect/zibaku_f2.chr";
inline constexpr const char *kPinkBlastFile = "expanded/effect/pink_blast.chr";
inline constexpr const char *kPinkBlastPicture = "pkblast";
inline constexpr double      kPinkFireHueTurn = -45.0; // their orange (26 to 31 degrees) onto Satan's Ax's pink red

// A pack's picture (in bank, named picture) turned kPinkFireHueTurn and renamed to renamed, in mds's meshes too.
Bytes BuildPinkFire(std::span<const unsigned char> source, const char *bank, const char *picture, const char *mds, const char *renamed);

std::vector<DataBuild> PinkFireBuilds();

} // namespace expanded
