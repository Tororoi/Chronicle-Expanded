#pragma once

#include <array>
#include <vector>

#include "expanded/quests/givers.hpp"

namespace dc::game {
class CDungeonMap;
} // namespace dc::game

namespace expanded {

// Yellow Drops' quests, given once all six have joined the party (their Boss, Osmond, last):
//
// The puzzle: Ledan has a Magical Crystal for whoever shows him kPuzzleItems in the bag's first four slots, in that
// order (the Gooey Peach, Escape Powder, Stamina Drink and Bomb: what Blue Terra's people needed). His Magical Crystal
// acts while carried as a dungeon's does once found: every floor's monsters are shown on the map from the start.
//
//   hidden  -- six in the party -->  offered  -- asked -->  asked  -- the four in order -->  done
//
// The challenge: Samba has a Map for whoever clears the Moon Sea's floor 7 back floor (kChallengeDungeon,
// kChallengeFloor) as Toan with only the Dagger, its healing springs dry while the challenge is taken. Coming onto that
// floor as Toan with the Dagger in hand begins it; any other member or weapon there, or an item thrown, ends it until
// the floor is come onto again. Every monster there defeated clears it. His Map acts while carried as a dungeon's does
// once found: every floor's map is shown from the start.
//
//   hidden  -- six in the party -->  offered  -- asked -->  taken  -- the floor cleared -->  cleared  -- the reward -->  done
//
// With either carried, the dungeons' own (each floor's first two small chests) hold another item in its place.
inline constexpr std::array<int, 4> kPuzzleItems = {167, 175, 150, 159};
inline constexpr int                kChallengeDungeon = 4;
inline constexpr int                kChallengeFloor = 6;
// Frames on the challenge's floor before it begins, for the floor to come into view and its name to go.
inline constexpr int kChallengeSettle = 150;

void RegisterYellowDropsQuests();

// Every text the quests show.
std::vector<dc::quest::Text> YellowDropsTexts();

// Hooks: each dungeon step (DungeonMonstersTick): the challenge kept; an item thrown (CMainItemModel::SetThrowModel).
void MoonSeaChallengeStep();
void ItemThrown();

// Hook: HealingWater: true while the springs are dry (the challenge's floor, the challenge taken).
bool SpringsDry();

// Hooks: a floor's chests set out (FloorChestsPlaced): a found Map's and Magical Crystal's chests given another item
// while the quests' are carried; and the floor built (BtCleatRandomMap, BtCleatFreeMap, after their reset): the
// carried ones acting.
void CarriedMapChests(CDungeonMap &map, int dungeon, int floor, bool back_floor);
void CarriedMapsAct();

// Where the quests stand, for the quest tracker: the puzzle asked and unsolved, the challenge taken or cleared.
bool PuzzleOn();
bool ChallengeOn();
bool ChallengeCleared();

} // namespace expanded
