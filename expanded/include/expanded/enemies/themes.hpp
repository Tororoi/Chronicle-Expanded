#pragma once

#include <random>
#include <span>
#include <string_view>
#include <vector>

#include "expanded/enemies/roster.hpp"

namespace expanded {

// The chance a randomized floor is a themed group, and within a theme the chances its members spawn once
// each (backfilled with repeatable monsters) and that the backfill is the mimics rather than the natives.
inline constexpr double kThemeChance = 0.50;
inline constexpr double kThemeCapOneChance = 0.50;
inline constexpr double kThemeMimicFillChance = 0.50;

// A themed group of MonstorTable kinds.
struct Theme {
    std::string_view name;
    std::vector<int> members;
    bool             whole_set = false;   // placed whole, never cut down
    int              home = -1;           // the dungeon it is native to, or -1
    double           weight = 1.0;        // on top of its home's pull
    std::vector<int> single = {};         // members that spawn once even when the group fills the floor
    bool             with_mimics = false; // always brings the dungeon's mimic and king mimic
};

// Every theme but the dungeon's own Mimics.
std::span<const Theme> Themes();

// The regular monsters a dungeon lays out natively, mimics left out.
const std::vector<int> &DungeonNatives(int dungeon);

// A theme's pull in a dungeon: 1, or for a native theme from 0.1 in the farthest dungeon up to 1 at home,
// times 4 there.
double ThemeWeight(const Theme &theme, int dungeon);

// A themed roster for a dungeon's floor, both its sides, and the theme's name ("Mimics" for the dungeon's
// own pair). Its members fill the floor, or spawn once each over repeatable mimics or natives.
Roster ThemedRoster(int dungeon, int floor, std::mt19937 &random, std::string_view *name = nullptr);

} // namespace expanded
