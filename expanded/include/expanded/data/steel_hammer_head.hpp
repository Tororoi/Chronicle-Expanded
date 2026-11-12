#pragma once

// The Steel Hammer's head thrown as the Bomb Gemron's bomb is (the apple shot, dun/effect/ringo_ex.chr, with the head's
// mesh and the hammer's pictures), built at extraction into kSteelHeadThrowFile: Super Steve's pellets with the hammer's
// SynthSphere fly as it (weapons/xiao/steel_hammer_shot.hpp). The head is the hammer's mesh (commenu/weapon/c06w02.chr,
// its one mesh frame) cut to the triangles whose every corner lies past kSteelHeadCut along the handle (z), the spiked
// ball without the handle. The spike on the ball's top (its submeshes on material kSteelHeadTopSpike) is copied,
// mirrored through the ball's middle, where the handle was, and the ball's middle (midway along the z of its own
// material, kSteelHeadBall) moved to the model's origin. The hammer's pictures (row-major, in an IMG bank) go into the
// apple's IM2 bank in its block order. Includes no game header, so dcdata can build with it.

#include <span>
#include <vector>

#include "expanded/data/bytes.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

inline constexpr const char *kSteelHeadThrowFile = "expanded/effect/steel_head_throw.chr";
inline constexpr const char *kSteelHammerPack = "commenu/weapon/c06w02.chr";
inline constexpr const char *kSteelHammerModel = "c06w02.mds";
inline constexpr const char *kSteelHammerPictures = "c06w02.img";
inline constexpr double      kSteelHeadCut = 2.0; // the handle ends at z 0.54, the ball begins at 4.46
inline constexpr int         kSteelHeadBall = 8, kSteelHeadTopSpike = 5;

// The hammer's head alone, its mesh's bytes (an MDT), from the hammer's model.
Bytes SteelHammerHead(std::span<const unsigned char> hammer_mds);

// The thrown head's pack, from the apple shot's and the hammer's packs.
Bytes BuildSteelHeadThrow(std::span<const unsigned char> apple, std::span<const unsigned char> hammer);

std::vector<DataBuild> SteelHammerHeadBuilds();

} // namespace expanded
