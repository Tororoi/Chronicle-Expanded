#pragma once

// Minotaur Joe's battle model with a death of his own: his defeat cutscene's stagger and fall appended to his
// motions (expanded/docs/bosses.md). Includes no game header, so dcdata can build with it.

#include <cstdio>
#include <filesystem>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

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

// The roar and collapse, which event 406 plays 75 ticks into the fall (frame 105 at its speed).
inline constexpr double kJoeRoarFrame = 105;
inline constexpr int    kJoeRoarSound = 1809;

// The battle pack with the fall grafted by joint name past its last frame as KEY 11, and KEY 12 holding the
// fall's last frame, as other monsters' death loops do. The fall's sounds come along as EVENT lines: the
// cutscene cfg's stomps and the roar.
Bytes BuildJoeModel(std::span<const unsigned char> battle, std::span<const unsigned char> cutscene);

// The cfg with lines inserted before the MOTION_END after its KEY_START.
Bytes AppendKeys(std::span<const unsigned char> cfg, std::string_view lines);

// The cfg with lines inserted after the MOTION_END after its KEY_START.
Bytes AppendEvents(std::span<const unsigned char> cfg, std::string_view lines);

// The cfg's sound EVENT lines (kind 1) as (frame, sound).
std::vector<std::pair<double, int>> SoundEvents(std::span<const unsigned char> cfg);

// Builds the model into data, skipping it when the data lacks its sources. Returns the failures.
int BuildBossModels(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
