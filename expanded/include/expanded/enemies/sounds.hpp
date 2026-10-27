#pragma once

#include "dungeon.h"

#include <vector>

#include "expanded/enemies/monsters.hpp"

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The dungeon sound sets: snd100 + a floor's region (normalize.hpp), each dungeon's, then Demon Shaft's five bands from
// snd106 to snd110.
inline constexpr int kDungeonSoundSets = 100;
inline constexpr int kDemonShaftSoundSets = kDungeonSoundSets + DUNGEON_DEMON_SHAFT;

// The dungeon sound set of a dungeon's floor (UserStatus->cur_floor).
int DungeonSoundSet(int dungeon, int floor);

// The sound sets that carry a kind's sounds: the dungeon sound set of each floor laying it out, and the arena set of a
// boss whose event loads its own.
std::vector<int> HomeSoundSets(int model_no);

inline std::vector<int> HomeSoundSets(Monster monster) {
    return HomeSoundSets(No(monster));
}

// Hooks: BtLoadMonstor loaded a staged floor's kinds, which brings in the monster banks of their home sets;
// CSound::SE_Play picks the bank a monster port effect plays from (CC 0), or -1 to leave the port alone.
void MonsterSoundsFloorLoaded(const CMonstorUnit &unit);
int  MonsterSoundBank(int port, int program, int note);

// A kind brought onto the floor besides its own (a summoned ally): the monster banks of its home sets too.
void MonsterSoundsAddKind(int model_no);

} // namespace expanded
