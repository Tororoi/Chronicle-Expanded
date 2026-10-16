#pragma once

#include <array>
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

// Weights of a roster's number of regular kinds, 1 to 9: 1% one, 1% two to four (4:9:16) and 98% five to nine
// (25:36:49:64:81), so a floor mostly holds many.
inline constexpr std::array<double, kRosterSize> kKindCountWeights = {
    0.01,
    0.01 * 4 / 29,
    0.01 * 9 / 29,
    0.01 * 16 / 29,
    0.98 * 25 / 255,
    0.98 * 36 / 255,
    0.98 * 49 / 255,
    0.98 * 64 / 255,
    0.98 * 81 / 255,
};

// A number of kinds from 1 to most (at most nine), by kKindCountWeights rescaled over that range; 0 below 1.
int RollKindCount(int most, std::mt19937 &random);

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

// A random mix for one side of a dungeon's floor: its mimic and king mimic by chance (the king once), then a
// rolled number of distinct kinds of the pool in the room they leave.
Roster RandomMix(int dungeon, int floor, std::mt19937 &random);

} // namespace expanded
