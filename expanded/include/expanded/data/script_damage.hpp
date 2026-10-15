#pragma once

// Each monster script's damage (dun/monstor/<name>.stb), for the enemy stat scaling. Includes no game
// header, so dcdata can build with it.

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace expanded {

// The file under the data directory.
inline constexpr const char *kScriptDamageFile = "expanded/enemies/script_damage.txt";

// A script's strongest _SET_DMG_PARA hit and strongest _SET_SHOT/_SET_SHOT2 damage argument (0 for none),
// and whether it fires shot 0 (_SET_SHOT) or shot 1 (_SET_SHOT2) without one, at its BT_SHOT_EFFECT's.
struct ScriptDamage {
    int  melee = 0;
    int  shot = 0;
    bool default_shot[2] = {};

    bool operator==(const ScriptDamage &) const = default;
};

// Reads a script's damage, following damage passed into a function to its callers' constants; nullopt when
// it is no script.
std::optional<ScriptDamage> ReadScriptDamage(std::span<const std::uint8_t> script);

// The file's text, a line per script, and back.
std::string                                    FormatScriptDamage(const std::map<std::string, ScriptDamage> &scripts);
std::map<std::string, ScriptDamage, std::less<>> ParseScriptDamage(std::string_view text);

// Builds the file from data's dun/monstor scripts. Returns the failures.
int BuildScriptDamage(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
