#pragma once

// The burst Colossus pops back to his size in: the game's explosion (dun/effect/explosion.chr, as Big Bang's Detonate
// blows) in turquoise, built at extraction into kColossusPopFile. Includes no game header, so dcdata can build with it.
//
// The explosion is drawn from its one picture, unlit and added (its materials are white): the picture's hue is set to
// kColossusPopHue, each texel's saturation and lightness its own, the colours merged back to 256 as the look pages merge
// them, and the picture named kColossusPopPicture (the meshes' materials with it), so it is not taken for the game's
// own explosion when both are loaded.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kColossusPopSource = "dun/effect/explosion.chr";
inline constexpr const char *kColossusPopFile = "expanded/effect/colossus_pop.chr";
inline constexpr const char *kColossusPopPicture = "colpop";
inline constexpr double      kColossusPopHue = 185.0; // turquoise

// The explosion in turquoise.
Bytes BuildColossusPop(std::span<const unsigned char> explosion);

std::vector<DataBuild> ColossusPopBuilds();

} // namespace expanded
