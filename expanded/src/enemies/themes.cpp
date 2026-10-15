#include "expanded/enemies/themes.hpp"

#include <algorithm>
#include <cmath>

#include "expanded/enemies/randomize.hpp"

namespace expanded {

namespace {

constexpr int    kDungeons = 7;
constexpr int    kDemonShaft = 6;
constexpr double kFarWeight = 0.10;
constexpr double kHomeBoost = 4.0;

// Demon Shaft's five bands, each led by its Gemron.
const std::vector<int> kDemonShaftFire = {111, 119, 115, 112, 116, 117, 118, 120, 113, 114};
const std::vector<int> kDemonShaftIce = {121, 129, 128, 124, 122, 123, 125, 126, 127};
const std::vector<int> kDemonShaftThunder = {132, 140, 138, 135, 134, 133, 137, 139, 136};
const std::vector<int> kDemonShaftWind = {143, 147, 144, 150, 148, 151, 149, 146, 145};
const std::vector<int> kDemonShaftHoly = {154, 162, 158, 159, 156, 155, 160, 161, 157};

// MonstorTable kinds by name, as the groups list them.
const std::vector<Theme> kThemes = {
    {"Cards", {38, 39, 40, 41, 42}, true},
    {"Cards (Enhanced)", {146, 128, 120, 138, 159}, true},
    {"Days of the Week", {10, 11, 12, 13, 14, 15, 16}, true},
    {"Dragons", {51, 65, 66}, false, -1, 2.0},
    {"Outlaws", {77, 26, 61}},
    {"Pirates", {23, 24, 21, 95, 94}, true, -1, 0.5, {23, 95, 94}, true},
    {"Gemron Elementals", {111, 121, 132, 143, 154}, true},
    {"Fire", {111, 118, 26, 51}, true},
    {"Ice", {121, 57, 77, 65}, true},
    {"Thunder", {132, 135, 9, 49, 61}, true},
    {"Wind", {143, 27, 25, 144, 50}, true},
    {"Holy", {154, 155, 136, 122, 125}, true},
    {"Undead", {1, 0, 155, 122, 144, 45}, false, -1, 2.0, {45}},
    {"Marine", {63, 19, 20, 98, 64}, false, -1, 2.0},
    {"Rock", {97, 27, 28, 69}, false, -1, 2.0},
    {"Plants", {8, 59, 60, 5}, false, -1, 2.0},
    {"Beasts", {3, 4, 113, 55}, false, -1, 2.0},
    {"Sky Dwellers", {52, 53, 68}, false, -1, 2.0},
    {"Metal", {35, 37, 56, 21, 47}, false, -1, 2.0},
    {"Mages", {46, 17, 133, 112}, false, -1, 2.0},
    {"Not the Bees", {6, 59}},
    {"Bombs Away", {43, 147}, false, -1, 2.0},
    {"Whack-a-Mole", {9, 99}, false, -1, 0.5},
    {"Bait", {100, 58, 18, 99}, false, -1, 3.0, {100}},
    {"Halloween", {7, 36, 1, 119, 4, 53}, false, -1, 2.0},
    {"Gorgon", {22, 2}},
    {"Precious", {95, 94, 54}, false, -1, 0.5},
    {"Graveyard Shift", {117, 1, 122, 144}, false, -1, 2.0},
    {"Demon Shaft: Fire", kDemonShaftFire, false, kDemonShaft},
    {"Demon Shaft: Ice", kDemonShaftIce, false, kDemonShaft},
    {"Demon Shaft: Thunder", kDemonShaftThunder, false, kDemonShaft},
    {"Demon Shaft: Wind", kDemonShaftWind, false, kDemonShaft},
    {"Demon Shaft: Holy", kDemonShaftHoly, false, kDemonShaft},
};

std::vector<int> Join(std::initializer_list<const std::vector<int> *> groups) {
    std::vector<int> kinds;
    for (const std::vector<int> *group : groups) {
        kinds.insert(kinds.end(), group->begin(), group->end());
    }
    return kinds;
}

const std::vector<int> kNatives[kDungeons] = {
    {1, 0, 2, 3, 52, 36, 51, 69, 97, 96, 98},
    {8, 9, 5, 6, 7, 4, 18, 100, 99, 10, 11, 12, 13, 14, 15, 16},
    {23, 24, 19, 20, 21, 22, 67, 60, 77},
    {25, 44, 50, 43, 63, 27, 28, 56, 65, 94, 95, 26},
    {35, 17, 49, 58, 64, 62, 48, 28, 68, 54},
    {37, 61, 66, 57, 39, 46, 59, 40, 53, 38, 42, 45, 47, 55, 41},
    Join({&kDemonShaftFire, &kDemonShaftIce, &kDemonShaftThunder, &kDemonShaftWind, &kDemonShaftHoly}
    ),
};

bool Chance(std::mt19937 &random, double chance) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(random) < chance;
}

// The repeatable monsters that backfill a theme's one-of-each: the mimics or the natives, by chance.
std::vector<int> Fillers(int dungeon, int floor, std::mt19937 &random) {
    if (Chance(random, kThemeMimicFillChance)) {
        DungeonMimics mimics = MimicsOf(dungeon, floor);
        return {mimics.mimic, mimics.king};
    }
    std::vector<int> natives = DungeonNatives(dungeon);
    std::shuffle(natives.begin(), natives.end(), random);
    return natives;
}

// Adds a kind, or finds it there already; false once the roster is full.
bool Place(Roster &roster, int kind, bool once) {
    return roster.Contains(kind) || roster.Add(kind, once);
}

} // namespace

std::span<const Theme> Themes() {
    return kThemes;
}

const std::vector<int> &DungeonNatives(int dungeon) {
    static const std::vector<int> none;
    return dungeon >= 0 && dungeon < kDungeons ? kNatives[dungeon] : none;
}

double ThemeWeight(const Theme &theme, int dungeon) {
    if (theme.home < 0) {
        return theme.weight;
    }
    double near = 1.0 - std::abs(dungeon - theme.home) / static_cast<double>(kDungeons - 1);
    double pull = kFarWeight + (1.0 - kFarWeight) * near;
    return theme.weight * (dungeon == theme.home ? pull * kHomeBoost : pull);
}

Roster ThemedRoster(int dungeon, int floor, std::mt19937 &random, std::string_view *name) {
    // The themes by weight, the dungeon's Mimics last at 1.
    double total = 1.0;
    for (const Theme &theme : kThemes) {
        total += ThemeWeight(theme, dungeon);
    }
    double       roll = std::uniform_real_distribution<double>(0.0, total)(random);
    const Theme *theme = nullptr;
    for (const Theme &candidate : kThemes) {
        roll -= ThemeWeight(candidate, dungeon);
        if (roll < 0.0) {
            theme = &candidate;
            break;
        }
    }

    DungeonMimics    mimics = MimicsOf(dungeon, floor);
    std::vector<int> members = theme != nullptr ? theme->members : std::vector<int>{mimics.mimic, mimics.king};
    std::shuffle(members.begin(), members.end(), random);
    if (name != nullptr) {
        *name = theme != nullptr ? theme->name : "Mimics";
    }
    auto single = [&](int kind) {
        return theme != nullptr && std::find(theme->single.begin(), theme->single.end(), kind) != theme->single.end();
    };

    Roster roster;
    if (theme == nullptr || theme->whole_set) {
        // The whole set first; then the mimics it brings, or by chance one-of-each over fillers that fit.
        for (int kind : members) {
            Place(roster, kind, single(kind));
        }
        bool capped = false;
        if (theme != nullptr && theme->with_mimics) {
            Place(roster, mimics.mimic, false);
            Place(roster, mimics.king, false);
        } else if (theme != nullptr && Chance(random, kThemeCapOneChance)) {
            for (int kind : Fillers(dungeon, floor, random)) {
                if (!roster.Contains(kind) && roster.Add(kind, false)) {
                    capped = true;
                }
            }
        }
        if (capped) {
            for (int kind : members) {
                roster.SetOnce(kind, true);
            }
        }
    } else if (!Chance(random, kThemeCapOneChance)) {
        // The group fills the floor.
        for (int kind : members) {
            Place(roster, kind, single(kind));
        }
    } else {
        // One of each, over a repeatable filler placed first so the floor fills.
        std::vector<int> fillers = Fillers(dungeon, floor, random);
        roster.Add(fillers.front(), false);
        for (int kind : members) {
            Place(roster, kind, true);
        }
        for (int kind : fillers) {
            Place(roster, kind, false);
        }
    }
    // A king mimic spawns once, but for the dungeon's Mimics.
    if (theme != nullptr) {
        roster.SetOnce(mimics.king, true);
    }
    return roster;
}

} // namespace expanded
