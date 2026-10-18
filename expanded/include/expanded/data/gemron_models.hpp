#pragma once

// The Expanded Gemrons' models (expanded/docs/gemrons.md), built from the disc at extraction. Includes no game
// header, so dcdata can build with it.

#include <cstdio>
#include <filesystem>
#include <span>

#include "expanded/data/chr_pack.hpp"

namespace expanded {

// Every Gemron's gems: the big one on its head and the two small ones.
inline constexpr const char *kGemronBigGem = "tama1__m";
inline constexpr const char *kGemronSmallGems[] = {"tamas00__m", "tamas03__m"};

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
// The frame its ball shatters on: the death clip's (from 200) copy of the vanilla death's frame 121 (from 105).
inline constexpr int kCrystalShatterFrame = 216;

extern const char *const kCrystalGemronLook;

Bytes BuildCrystalGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> ball);
Bytes BuildCrystalGemronScript(std::span<const unsigned char> stb);

// The Atla Gemron: Ice Gemron's pack in its own look, holding a dungeon atla on its big gem and wearing Toan's
// Atlamillia on its forehead with the Atlamillia's light playing on it, fly_light glows on its wing gems and a spawn
// bone for its shot; its death copied to 200 and held, the held atla out of the draw from the swap (the copy of the
// gem landing), where the game's own atla takes its place and bounces free. Holy Gemron's script, firing its shot from
// the spawn bone along its facing, from closer and more often; and the shot's pack, the Atlamillia's burst.
inline constexpr const char *kAtlaGemronModelFile = "expanded/monstor/e169a.chr";
inline constexpr const char *kAtlaGemronScriptFile = "expanded/monstor/e169a.stb";
inline constexpr const char *kAtlaShotFile = "expanded/effect/atla_s.chr";
inline constexpr const char *kAtlaGemronSource = "dun/monstor/e112a.chr";
inline constexpr const char *kAtlaGemronScriptSource = "dun/monstor/e115a.stb";
inline constexpr const char *kAtlaPackFile = "dun/pack/maindat.pac";
inline constexpr const char *kAtlaPictureFile = "dun/etc/atrtx.img";
inline constexpr const char *kToanPackFile = "dun/mainchara/c01d.chr";
inline constexpr const char *kAtlamilliaLightFile = "gedit/s88/chara/e503ex_atall.chr";
inline constexpr const char *kFlyLightFile = "gedit/e01/chara/fly_light.chr";
inline constexpr const char *kAtlaGemronLens = "renzu__m";
inline constexpr const char *kAtlaShotSpawn = "shot0";
inline constexpr const char *kAtlaShotModel = "atla_s";
inline constexpr int         kAtlaGemronDeathStart = 200;
inline constexpr int         kAtlaGemronSwap = kAtlaGemronDeathStart + 122 - 105;
inline constexpr int         kAtlaGemronDeathEnd = kAtlaGemronSwap + 12;
// The shot: e503ex_atall's key 9 tail at 0.35.
inline constexpr int    kAtlaShotFrom = 221;
inline constexpr int    kAtlaShotTo = 250;
inline constexpr double kAtlaShotSpeed = 0.35;

extern const char *const kAtlaGemronLook;

Bytes BuildAtlaGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> atla_pack,
                           std::span<const unsigned char> atla_img, std::span<const unsigned char> toan,
                           std::span<const unsigned char> light, std::span<const unsigned char> fly);
Bytes BuildAtlaShotPack(std::span<const unsigned char> light);
Bytes BuildAtlaGemronScript(std::span<const unsigned char> stb);

Bytes BuildBombGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> bomb_mds,
                           std::span<const unsigned char> bomb_img, std::span<const unsigned char> quad_chr,
                           std::span<const unsigned char> glow_pack);
Bytes BuildBombThrowPack(std::span<const unsigned char> apple, std::span<const unsigned char> bomb_mds,
                         std::span<const unsigned char> bomb_img);

// Builds every Gemron's files under data; the number that failed.
int BuildGemronModels(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
