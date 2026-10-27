#pragma once

#include <optional>

#include "expanded/enemies/monsters.hpp"

class CMonstorUnit;

namespace expanded {

struct Roster;

// A boss fought as several monsters whose scripts talk to each other (expanded/docs/bosses.md): the Ice Queen and her
// barrier, King's Curse's coffin and king, the Black Knight's mount and the Knight. On a regular floor a cast keeps
// together: its scripts share a mailbox of their own instead of the floor's globals, and a script's monster n is the
// cast's nth.

// Globals a cast's mailbox holds, as many as the floor's.
inline constexpr int kBossMailbox = 10;

// A kind's cast: both kinds and their names, which of them the kind is, the party member its boss needs (-1 for
// anyone; CHARA_*), and whether the kind comes alone when a roster has none of the rest. None for a kind in no cast.
struct BossCastInfo {
    Monster     kinds[2];
    const char *names[2];
    int         member;
    int         needs;
    bool        alone;
};

std::optional<BossCastInfo> BossCastOf(Monster kind);

// Whether the party lets a cast member's boss be placed (one with the party member who can hurt it).
bool BossCastAllowed(Monster kind);

// A roster with any of a cast keeps all of it, each spawned once, while the party member the fight needs
// (the one who can hurt the boss) is in the party; otherwise none of it, as with no room for the rest. A
// member that may come alone (the Black Knight) stays alone when the roster has none of the rest.
void BossCastRoster(Roster &roster);

// Whether a monster plays in a cast on this floor.
bool BossCastMember(int monster);

// The monster playing a cast's kind on this floor, or -1; the mailbox of the cast it is in, or null.
int  BossCastMonster(Monster kind);
int *BossCastMailbox(Monster kind);

// _SET_GLOBAL_INT and _GET_GLOBAL_INT: the globals the monster's script uses.
int *BossCastGlobals(int monster, int *globals);

// The opcodes taking a monster's number (_GET_/_SET_MONSTOR_*, _SET_/_DEL_REFERENCE): the monster it means; a partner
// that did not come is the member itself.
int BossCastMonsterRef(int monster, int target);

// _SET_POSITION: true drops it; else it may move the position: a cast member setting up keeps its spot, and a hiding
// place is taken by its partner's side.
bool BossCastPlace(CMonstorUnit &unit, int monster, float *position);

// A monster's life running out: false when it is no kill (the boss's partners).
bool BossCastCountsKill(int monster);

// The dungeon step: a dormant follower waits by its partner, to wake with it.
void BossCastStep(CMonstorUnit &unit);

// _SET_REFERENCE and _DEL_REFERENCE: a member attached to its partner (-1: let go) sits on the anchor.
void BossCastAttached(CMonstorUnit &unit, int monster, int partner);

// _SET_REFERENCE to a partner of the roster still to spawn: the member is attached once it does.
void BossCastReferLater(int monster, int target, const char *frame);

// DrawMonstor's distance to the player: an attached member wakes and sleeps by where its partner is, not by its offset
// there.
void BossCastWakePosition(CMonstorUnit &unit, int monster, float *position);

// The boss's defeat: its partners leave with it.
void BossCastDefeated(CMonstorUnit &unit, int monster);

} // namespace expanded
