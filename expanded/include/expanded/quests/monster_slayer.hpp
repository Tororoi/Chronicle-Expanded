#pragma once

#include <span>
#include <string>
#include <vector>

#include "expanded/enemies/monsters.hpp"
#include "expanded/quests/givers.hpp"

namespace expanded {

// Monster Slayer quests: Macho (Norune), Gob (Matataki), Jake (Queens) and Chief Bonka (Muska Lacka) each set the
// player to defeat kSlayerLeast to kSlayerMost of a kind of monster in a dungeon the player has been into, the kind one
// of kSlayerTargets' four for that dungeon. Macho's first is always kMachoFirst (Dashers in the Divine Beast Cave).
// Givers do not set a kind another's quest under way already wants, while another roll can find one.
//
// A giver's first word of it tells what the quests are and sets the first. Only kills of that kind (or its stronger
// copy) in that dungeon count, from when the quest is set; the last one is told in play. The giver then gives a
// Powerup Powder, and sets the next quest when next asked. A pack with no room keeps the reward for later.
//
//   unheard  -- asked -->  active  -- the last kill -->  complete  -- the reward -->  idle  -- asked -->  active
inline constexpr int kSlayerLeast = 8;
inline constexpr int kSlayerMost = 18;
inline constexpr int kSlayerDungeons = 6; // the Divine Beast Cave to the Gallery of Time
inline constexpr int kSlayerRerolls = 50;

inline constexpr Monster kSlayerTargets[kSlayerDungeons][4] = {
    {Monster::MASTER_JACKET, Monster::DASHER,       Monster::MIMIC_DBC, Monster::DRAGON         },
    {Monster::FLI_FLI,       Monster::EARTH_DIGGER, Monster::MIMIC_WOF, Monster::WEREWOLF       },
    {Monster::GUNNY,         Monster::GYON,         Monster::MIMIC_SW,  Monster::PIRATES_CHARIOT},
    {Monster::GOLEM,         Monster::DUNE,         Monster::MIMIC_SMT, Monster::BLUE_DRAGON    },
    {Monster::MOON_DIGGER,   Monster::SPACE_GYON,   Monster::MIMIC_MS,  Monster::CRESCENT_BARON },
    {Monster::RASH_DASHER,   Monster::JOKER,        Monster::MIMIC_GOT, Monster::ALEXANDER      },
};

struct SlayerTarget {
    int     dungeon;
    Monster monster;
};

inline constexpr SlayerTarget kMachoFirst = {.dungeon = 0, .monster = Monster::DASHER};

// The four givers, and each one's quest id.
struct SlayerGiver {
    const char *quest;
    Giver       giver;
};

std::span<const SlayerGiver> SlayerGivers();

void RegisterMonsterSlayer();

// Every text the quests show.
std::vector<dc::quest::Text> MonsterSlayerTexts();

// Hook: a monster killed (MonsterKilled), in dungeon (selectMapNo).
void MonsterSlayerKill(int dungeon, Monster monster);

// What a giver's quest wants and has left, for the quest tracker: false when none is under way.
struct SlayerQuest {
    SlayerTarget target;
    int          left = 0;
    int          total = 0;
    bool         complete = false;
};

bool MonsterSlayerQuest(const SlayerGiver &giver, SlayerQuest &quest);

// A kind's name, in the plural, and where a dungeon is ("in the Shipwreck"), as the quests say them.
std::string SlayerMonsterName(Monster monster);
std::string QuestDungeonPlace(int dungeon);

} // namespace expanded
