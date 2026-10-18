#pragma once

// The Expanded Gemrons' models (expanded/docs/gemrons.md), built from the disc at extraction. Includes no game
// header, so dcdata can build with it.

#include <cstdio>
#include <filesystem>
#include <span>

#include "expanded/data/chr_pack.hpp"

namespace expanded {

// The Bomb Gemron: Ice Gemron's pack (the five Gemrons share one rig and sheet layout) in its own look, its three gems
// the thrown bomb, a glow disc on the big bomb and a self-destruct motion; and its thrown bomb's pack.
inline constexpr const char *kBombGemronModelFile = "expanded/monstor/e167a.chr";
inline constexpr const char *kBombThrowFile = "expanded/effect/bomb_throw.chr";
inline constexpr const char *kBombGemronSource = "dun/monstor/e112a.chr";
inline constexpr const char *kBombModelFile = "dun/item/main_data/bakudan.mds";
inline constexpr const char *kBombPictureFile = "dun/item/main_data/bakudan.img";
inline constexpr const char *kGlowQuadFile = "dun/effect/_b_boll.chr";
inline constexpr const char *kBombThrowSource = "dun/effect/ringo_ex.chr";

// The self-destruct: KEY 14 (a repeat of the attack the script never plays) over frames 200-218 at 0.15, the guard's
// return reversed then the guard loop, held one frame past its final pose (217).
inline constexpr int kBombSelfDestructKey = 14;
inline constexpr int kBombSelfDestructStart = 200;
inline constexpr int kBombSelfDestructEnd = 218;

// The glow disc's frame, appended to the rig under the big bomb.
inline constexpr const char *kBombGlowFrame = "bombglow__czappba";

extern const char *const kBombGemronLook;

// The Crystal Gemron: Holy Gemron's pack in its own look, each gem the e209 event's crystal ball (the big one with its
// glints, beams and rings), shattering through a death rebuilt from frame 200, its painted eyes a frame of their own;
// and Holy Gemron's script, its death sounding the shatter and its shots reaching to 140.
inline constexpr const char *kCrystalGemronModelFile = "expanded/monstor/e168a.chr";
inline constexpr const char *kCrystalGemronScriptFile = "expanded/monstor/e168a.stb";
inline constexpr const char *kCrystalGemronSource = "dun/monstor/e115a.chr";
inline constexpr const char *kCrystalGemronScriptSource = "dun/monstor/e115a.stb";
inline constexpr const char *kCrystalBallFile = "gedit/s34/chara/e209ball_b.chr";
inline constexpr const char *kCrystalEyeFrame = "eyes";

extern const char *const kCrystalGemronLook;

Bytes BuildCrystalGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> ball);
Bytes BuildCrystalGemronScript(std::span<const unsigned char> stb);

Bytes BuildBombGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> bomb_mds,
                           std::span<const unsigned char> bomb_img, std::span<const unsigned char> quad_chr,
                           std::span<const unsigned char> glow_pack);
Bytes BuildBombThrowPack(std::span<const unsigned char> apple, std::span<const unsigned char> bomb_mds,
                         std::span<const unsigned char> bomb_img);

// Builds every Gemron's files under data; the number that failed.
int BuildGemronModels(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
