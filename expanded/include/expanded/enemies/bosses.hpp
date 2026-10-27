#pragma once

#include <string>

#include "expanded/enemies/monsters.hpp"

namespace dc::game {
class CCharacter;
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// Whether a boss can fight on a regular floor: those of kBossTable. Others a staged roster places there play as on
// their arenas.
bool BossSupported(int model_no);

inline bool BossSupported(Monster monster) {
    return BossSupported(No(monster));
}

// A supported boss a staged roster put on the floor: it stays where the floor placed it, raises no event,
// and once beaten falls and fades out like any monster.
bool BossOutOfPlace(int monster);

// Steps a beaten boss fades over.
inline constexpr int kBossFadeFrames = 96;

// The motions a character holds, up to the KEY table's end mark.
int MotionCount(const CCharacter &character);

// SetupBaseModel's pack: Joe's or the Black Knight's built model when the data has it.
void BossPackFile(std::string &filename, int model_no);

// _SET_MONSTOR_POS from a boss setting up or beaten: true keeps the monsters where they are.
bool BossKeepsPlace(const CMonstorUnit &unit, int monster);

// SetupViewMonstor and DrawMonstor: whether the monster is King's Curse's king, drawn as his smoke instead of his
// blocks: on his floor monster 1, elsewhere the king a staged roster brought.
bool BossSmokes(int monster, int model_slot);

// _SET_POSITION: true drops it; a cast's rules may move it (boss_casts.hpp).
bool BossPlace(CMonstorUnit &unit, int monster, float *position);

// _RUN_SCRIPT and _STATUS_SET_EVENT: true drops the event.
bool BossEventHeld(int monster);

// _SET_SND_NOW: true drops the arena's crowd.
bool BossSoundHeld(int monster, int sound);

// Step before a monster's script runs: true steps the death instead.
bool BossDeathStep(CMonstorUnit &unit, int monster);

} // namespace expanded
