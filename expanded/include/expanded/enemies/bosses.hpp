#pragma once

#include "expanded/enemies/monsters.hpp"

class CCharacter;
class CMonstorUnit;

namespace expanded {

// Whether a boss can fight on a regular floor: Minotaur Joe and Master Utan for now. Others a staged roster
// places there play as on their arenas.
bool BossSupported(int model_no);

inline bool BossSupported(Monster monster) {
    return BossSupported(No(monster));
}

// A supported boss a staged roster put on the floor: it stays where the floor placed it, raises no event,
// and once beaten falls and fades out like any monster.
bool BossOutOfPlace(int monster);

// Joe's disc death, which he falls with when the data lacks his built model.
inline constexpr int kJoeDiscDeath = 9;

// Steps a beaten boss fades over.
inline constexpr int kBossFadeFrames = 96;

// The motions a character holds, up to the KEY table's end mark.
int MotionCount(const CCharacter &character);

// Hooks: SetupBaseModel's pack (Joe's built model when the data has it), _SET_POSITION and _SET_MONSTOR_POS
// from a boss setting up or beaten (true keeps the monsters where they are), _RUN_SCRIPT and _STATUS_SET_EVENT
// (true drops the event), _SET_SND_NOW (true drops the arena's crowd), and Step before a monster's script runs (true steps the death instead).
void BossPackFile(char *filename, int model_no);
bool BossKeepsPlace(const CMonstorUnit &unit, int monster);
bool BossEventHeld(int monster);
bool BossSoundHeld(int monster, int sound);
bool BossDeathStep(CMonstorUnit &unit, int monster);

} // namespace expanded
