#pragma once

#include "expanded/enemies/monsters.hpp"

namespace expanded {

// The dungeon bosses the roster can place on a floor: each with the part fighting beside it (the King's Curse its
// coffin raises, the Black Knight's mount; itself when none), its dungeon and its arena's sound set, which only the
// arena's event loads (_LOAD_SOUND_SET).
struct BossEntry {
    Monster kind;
    Monster partner;
    int     dungeon;
    int     sound_set;
};

inline constexpr BossEntry kBossTable[] = {
    {Monster::DRAN,               Monster::DRAN,               0, 200},
    {Monster::MASTER_UTAN,        Monster::MASTER_UTAN,        1, 201},
    {Monster::ICE_QUEEN,          Monster::ICE_QUEEN,          2, 202},
    {Monster::KINGS_CURSE_COFFIN, Monster::KINGS_CURSE,        3, 203},
    {Monster::MINOTAUR_JOE,       Monster::MINOTAUR_JOE,       4, 204},
    {Monster::BLACK_KNIGHT,       Monster::BLACK_KNIGHT_MOUNT, 6, 207},
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

} // namespace expanded
