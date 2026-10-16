#pragma once

#include <optional>
#include <random>
#include <span>

#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"

namespace expanded {

// The Randomize Enemies toggle.
inline constexpr const char *kRandomizeEnemies = "randomize_enemies";
bool                         RandomizeEnemies();

// The chances a random mix carries the dungeon's mimic and king mimic.
inline constexpr double kMimicChance = 0.40;
inline constexpr double kKingMimicChance = 0.28;

// The kinds a random mix draws from: every regular monster but the mimics. A kind standing in for another
// (Gacious (Enhanced) for the boss-record Gacious) is listed twice, so it keeps both chances.
std::span<const Monster> RandomizerPool();

// The mimic and king mimic of a dungeon's floor: the dungeon's own, in Demon Shaft its band's; none off the
// dungeons.
struct DungeonMimics {
    Monster mimic;
    Monster king;
};

std::optional<DungeonMimics> MimicsOf(int dungeon, int floor);

// Whether the randomizer stages a floor: any but the boss and event floors.
bool RandomizedFloor(int dungeon, int floor);

// The randomizer's generator.
std::mt19937 &RosterRandom();

// A random mix for one side of a dungeon's floor: its mimic and king mimic by chance (the king once), then
// distinct kinds of the pool up to nine.
Roster RandomMix(int dungeon, int floor, std::mt19937 &random);

} // namespace expanded
