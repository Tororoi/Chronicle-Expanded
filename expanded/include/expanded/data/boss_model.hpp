#pragma once

// Minotaur Joe's battle model with a death of his own: his defeat cutscene's stagger and fall appended to his
// motions (expanded/docs/bosses.md). Includes no game header, so dcdata can build with it.

#include <cstdio>
#include <filesystem>
#include <span>
#include <string_view>

#include "expanded/data/chr_pack.hpp"

namespace expanded {

// The built model under the data directory, and what it is built from.
inline constexpr const char *kJoeModelFile = "expanded/monstor/c16a.chr";
inline constexpr const char *kJoeBattleFile = "dun/monstor/c16a.chr";
inline constexpr const char *kJoeCutsceneFile = "gedit/s90/chara/e406.chr";
inline constexpr const char *kJoeCutsceneCfg = "e406c16a.cfg";

// The appended KEYs: the stagger and fall, then its last frame held.
inline constexpr int kJoeFallKey = 11;
inline constexpr int kJoeDeathLoopKey = 12;

// The cutscene's KEY 3, staggering and falling over, at its speed.
inline constexpr int    kJoeFallFirst = 90;
inline constexpr int    kJoeFallLast = 140;
inline constexpr double kJoeFallSpeed = 0.2;

// The battle pack with the fall grafted by joint name past its last frame as KEY 11, and KEY 12 holding the
// fall's last frame, as other monsters' death loops do.
Bytes BuildJoeModel(std::span<const unsigned char> battle, std::span<const unsigned char> cutscene);

// The cfg with lines inserted before the MOTION_END after its KEY_START.
Bytes AppendKeys(std::span<const unsigned char> cfg, std::string_view lines);

// Builds the model into data, skipping it when the data lacks its sources. Returns the failures.
int BuildBossModels(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
