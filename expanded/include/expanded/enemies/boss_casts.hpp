#pragma once

#include "expanded/enemies/monsters.hpp"

class CMonstorUnit;

namespace expanded {

struct Roster;

// A boss fought as several monsters whose scripts talk to each other (expanded/docs/bosses.md): the Ice Queen
// and her barrier, King's Curse's coffin and king. On a regular floor a cast keeps together: its scripts
// share a mailbox of their own instead of the floor's globals, and a script's monster n is the cast's nth.

// Globals a cast's mailbox holds, as many as the floor's.
inline constexpr int kBossMailbox = 10;

// A roster with any of a cast keeps all of it, each spawned once, while the party member the fight needs
// (the one who can hurt the boss) is in the party; otherwise none of it, as with no room for the rest.
void BossCastRoster(Roster &roster);

// Whether a monster plays in a cast on this floor.
bool BossCastMember(int monster);

// The monster playing a cast's kind on this floor, or -1; the mailbox of the cast it is in, or null.
int  BossCastMonster(Monster kind);
int *BossCastMailbox(Monster kind);

// Hooks: _SET_GLOBAL_INT and _GET_GLOBAL_INT (the globals the monster's script uses), _GET_MONSTOR_POS,
// _GET_MONSTOR_VECTOR and _SET_MONSTOR_POS (the monster a script's monster number means), _SET_POSITION (true
// drops it, else it may move the position: a cast member setting up keeps its spot, and a hiding place is
// taken by its partner's side), a monster's life running out (false when it is no kill: the boss's partners),
// the dungeon step (a dormant follower waits by its partner, to wake with it) and the boss's defeat (its
// partners leave with it).
int *BossCastGlobals(int monster, int *globals);
int  BossCastMonsterRef(int monster, int target);
bool BossCastPlace(CMonstorUnit &unit, int monster, float *position);
bool BossCastCountsKill(int monster);
void BossCastStep(CMonstorUnit &unit);
void BossCastDefeated(CMonstorUnit &unit, int monster);

} // namespace expanded
