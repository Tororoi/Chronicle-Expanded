#pragma once

#include <span>
#include <vector>

#include "expanded/quests/givers.hpp"

namespace dc::game {
class CDungeonMap;
} // namespace dc::game

namespace expanded {

// Item hunts: Laura (Norune), Ro (Matataki), Phil (Queens) and Zabo (Muska Lacka) each tell of a rare item lost on a
// dungeon's back floors: the Medusa Powder in the Divine Beast Cave, the Warp Powder in the Wise Owl Forest, the Shell
// Ring in the Shipwreck and the Hardening Powder in the Sun & Moon Temple. From the rumour on, until the player has
// the item (carried or stored), each back floor of that dungeon has a kHuntChance chance of it in a small chest. Once
// found, the giver hears of it, and the hunt is done; the item is the player's to keep.
//
//   unheard  -- the rumour -->  heard  -- the item had -->  found  -- the giver told -->  done
inline constexpr float kHuntChance = 1.0f / 3.0f;

struct ItemHunt {
    const char *quest;
    Giver       giver;
    int         dungeon;
    int         item;
};

std::span<const ItemHunt> ItemHunts();

void RegisterItemHunts();

// Every text the hunts show.
std::vector<dc::quest::Text> ItemHuntTexts();

// Hook: a floor's chests set out (CDungeonMap::buildEventData), its back floor's when back_floor: a hunt's item put in
// the first small chest of its dungeon's back floor, its kHuntChance roll willing.
void ItemHuntChests(CDungeonMap &map, int dungeon, bool back_floor);

// Whether a hunt is on: its rumour heard and its item not yet had (for the quest tracker).
bool ItemHuntOn(const ItemHunt &hunt);

} // namespace expanded
