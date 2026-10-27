#pragma once

struct CUserStatus;

namespace expanded {

// Curses weapons put on the party members who equip them (Evilcise, Maneater, the Crystal Ring).
// A member is cursed while a rule says so, again on every floor, and freed when it stops. An item lifting the
// curse is the rule's to answer, and the curse stays off until the next floor or the next equip. A knock-out
// is not a lift: the curse comes back with the revival.
struct CurseRule {
    // Whether the rule curses chara now.
    bool (*cursing)(const CUserStatus &status, int chara);
    // An item lifted chara's curse.
    void (*lifted)(CUserStatus &status, int chara);
    // Whether a cursed member takes no other harmful ailment (Freeze, Poison, Goo).
    bool immune;
};

// Adds a rule; false when it is null or already added.
bool AddCurseRule(const CurseRule *rule);

// The rule cursing chara now, or nullptr.
const CurseRule *CurseOn(int chara);

// Hook: a dungeon step: curses, frees and watches every party member.
void CurseStep();

// Hook: BtSetStatusErr: whether ailment may be set on the active character.
bool CurseAllowsAilment(int ailment);

} // namespace expanded
