#pragma once

#include "dungeon.h"

#include "expanded/enemies/monsters.hpp"

namespace expanded {

// Joe's disc death, which he falls with when the data lacks his built model.
inline constexpr int kJoeDiscDeath = 9;

// A dungeon boss the roster can place on a floor.
struct BossEntry {
    Monster kind;
    Monster partner; // the part fighting beside it (the King's Curse its coffin raises, the Black Knight's mount), or itself
    int     dungeon;
    int     sound_set; // its arena's, which only the arena's event loads (_LOAD_SOUND_SET)
    // Beaten off its arena: the motion it falls with, the one it loops while fading (-1 holds the fall's last frame),
    // and for a boss in the air the fall's frame it lands on (0 for one on the ground).
    int   death;
    int   death_loop = -1;
    float death_landing = 0.0f;
    bool  arena_frame = false; // its script works in its arena's coordinates (boss_moves.hpp)
};

// Joe's and the Black Knight's deaths here are those without their built models (data/boss_model.hpp).
inline constexpr BossEntry kBossTable[] = {
    {.kind = Monster::DRAN, .partner = Monster::DRAN, .dungeon = DUNGEON_DIVINE_BEAST_CAVE, .sound_set = 200, .death = 6, .death_landing = 115.0f, .arena_frame = true},
    {.kind = Monster::MASTER_UTAN, .partner = Monster::MASTER_UTAN, .dungeon = DUNGEON_WISE_OWL_FOREST, .sound_set = 201, .death = 13},
    {.kind = Monster::ICE_QUEEN, .partner = Monster::ICE_QUEEN, .dungeon = DUNGEON_SHIPWRECK, .sound_set = 202, .death = 11, .death_loop = 12},
    {.kind = Monster::KINGS_CURSE_COFFIN, .partner = Monster::KINGS_CURSE, .dungeon = DUNGEON_SUN_MOON_TEMPLE, .sound_set = 203, .death = 5, .death_loop = 6},
    {.kind = Monster::MINOTAUR_JOE, .partner = Monster::MINOTAUR_JOE, .dungeon = DUNGEON_MOON_SEA, .sound_set = 204, .death = kJoeDiscDeath},
    {.kind = Monster::BLACK_KNIGHT, .partner = Monster::BLACK_KNIGHT_MOUNT, .dungeon = DUNGEON_DEMON_SHAFT, .sound_set = 207, .death = 7, .death_loop = 8},
};

// The boss a kind is, or null.
constexpr const BossEntry *BossEntryOf(int model_no) {
    for (const BossEntry &boss : kBossTable) {
        if (No(boss.kind) == model_no) {
            return &boss;
        }
    }
    return nullptr;
}

constexpr const BossEntry *BossEntryOf(Monster boss) {
    return BossEntryOf(No(boss));
}

} // namespace expanded
