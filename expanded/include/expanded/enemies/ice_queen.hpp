#pragma once

#include "expanded/enemies/monsters.hpp"

class CMonstorUnit;

namespace expanded {

struct Roster;

// The Ice Queen off her arena (expanded/docs/bosses.md): her barrier stays a monster, her other companions
// are shot effects fired for her script's requests, and her script and the barrier's talk through a mailbox
// of their own instead of the floor's globals.

// Her mailbox: the globals her script and her companions' read and write.
inline constexpr int kIceQueenMailbox = 9;

// A roster with her keeps her only while Ruby, the one who can hurt her, is in the party, and brings her
// barrier along.
void IceQueenRoster(Roster &roster);

// The monster playing her on this floor, or -1.
int IceQueenMonster();

// Hooks: _SET_GLOBAL_INT and _GET_GLOBAL_INT (the globals the monster's script uses), _GET_MONSTOR_POS and
// _GET_MONSTOR_VECTOR (the monster a script's monster number means), a monster's life running out (false
// when it is not a kill), the dungeon step after the shot effects (her companions), and her defeat (ends
// her effects and takes her barrier away).
int *IceQueenGlobals(int monster, int *globals);
int  IceQueenMonsterRef(int monster, int target);
bool IceQueenCountsKill(int monster);
void IceQueenStep(CMonstorUnit &unit);
void IceQueenDefeated(CMonstorUnit &unit);

} // namespace expanded
