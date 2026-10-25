#pragma once

// The mod's monster-script bakes, built from the disc's dun/monstor scripts into kBakedScriptDir, which the game loads
// in their place (BakedScriptFile). Includes no game header, so dcdata can build with it.
//   - Blizzard (e65a) takes Titan's four hurt spheres, Sam (e86a) and Billy (e69a) Mr. Blare's two.
//   - Minotaur Joe (c16a): every hurt sphere takes the Divine Beast cat's hit in full, its params kSpiritBeastSpherePercent
//     and kSpiritBeastSphereKick (the spare entries 0 and 1 of _SET_BODY_COL_PARA).
//   - Every chest mimic's wake is a guard, not invincibility: its _STATUS_SET_MUTEKI(100) is gone, and its first label
//     registers a guard window over the "appear" clip (kMimicWakeGuard, a king mimic's kKingMimicWakeGuard), after its
//     own guard window (a king, which has none, after its first hurt sphere).
// A rebuilt script carries kBakedScriptMark, and one that has it is not rebuilt.

#include <array>
#include <cstdio>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "expanded/data/bytes.hpp"

namespace expanded {

inline constexpr const char      *kBakedScriptDir = "expanded/monstor/";
inline constexpr std::string_view kBakedScriptMark{"DCE-SPHERES-v1\0", 15};

inline constexpr int   kSpiritBeastSpherePercent = 100, kSpiritBeastSphereKick = 2;
inline constexpr float kMimicWakeGuardStart = 10.0f, kMimicWakeGuardEnd = 28.0f, kKingMimicWakeGuardEnd = 27.0f;

inline constexpr std::array<const char *, 7> kMimicScripts = {"e35a", "e37a", "e39a", "e79a", "e81a", "e83a", "e109a"};
inline constexpr std::array<const char *, 7> kKingMimicScripts = {"e34a", "e36a", "e38a", "e78a", "e80a", "e82a", "e110a"};

// Whether the script (its name, as "e109a") is one rebuilt, and where its rebuilt copy is.
bool        ScriptIsBaked(std::string_view script);
std::string BakedScriptFile(std::string_view script);

// The script rebuilt from the disc's, and what was done; throws when the script is not one rebuilt, when it has
// nothing the bake changes, or when it has the mark.
Bytes BakeMonsterScript(std::string_view script, std::span<const unsigned char> stb, std::string *done = nullptr);

// Builds every one from data's dun/monstor scripts. Returns the failures.
int BuildMonsterScripts(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
