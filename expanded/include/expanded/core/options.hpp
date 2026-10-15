#pragma once

#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>

namespace expanded {

// An Expanded gameplay toggle: config.json's expanded.<key> and a row of the Options screen's Expanded page.
struct Toggle {
    const char *key = nullptr;   // "faster_enemies"
    const char *label = nullptr; // "Faster Enemies"
    const char *help = nullptr;  // "\"Faster Enemies\"\nMonsters move faster."
    bool        on = false;      // the default
};

// The toggles set away from their defaults, by key (Config::expanded_options).
struct OptionValues {
    std::map<std::string, bool, std::less<>> changed;

    bool operator==(const OptionValues &) const = default;
};

// The most toggles the Expanded page shows.
inline constexpr std::size_t kMaxToggles = 32;

// Adds a toggle before config.json loads, e.g. from a static initialiser in the feature's file:
// `const bool kRegistered = AddToggle({...});`. Returns false for a key already added or past kMaxToggles.
bool AddToggle(const Toggle &toggle);

// Every toggle, in the order added.
std::span<const Toggle> Toggles();

// Whether a toggle is on in these settings; false for a key no toggle has.
bool OptionOn(const OptionValues &values, std::string_view key);

// Whether a toggle is on in the current settings (ConfigGet).
bool OptionOn(std::string_view key);

// Turns a toggle on or off in these settings. Returns false for a key no toggle has.
bool SetOption(OptionValues &values, std::string_view key, bool on);

} // namespace expanded
