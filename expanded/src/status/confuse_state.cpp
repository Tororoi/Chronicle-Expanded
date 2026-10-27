#include "expanded/status/confuse_state.hpp"

#include "collisiondata.hpp"
#include "monstorunit.hpp"

namespace expanded::confuse {

Confusion &State() {
    static Confusion state;
    return state;
}

bool InPlay(const CMonstorUnit &unit, int slot) {
    const MONSTOR &monster = unit.monster[slot];
    return monster.state == 2 && monster.hp > 0 && monster.revealed != 0;
}

bool Confused(const CMonstorUnit &unit, int slot) {
    return slot >= 0 && slot < kFloorMonsters && State().until[slot] != 0 && InPlay(unit, slot);
}

int Provoker(const CMonstorUnit &unit, int slot) {
    if (Confused(unit, slot)) {
        return -1;
    }
    for (int attacker : State().hit_by[slot]) {
        if (attacker < 0) {
            break;
        }
        if (Confused(unit, attacker)) {
            return attacker;
        }
    }
    return -1;
}

int Attacker(const COLLISION_HIT &hit) {
    if (hit.monster_no >= 0 && hit.monster_no < kFloorMonsters) {
        return hit.monster_no;
    }
    if (hit.owner >= 200 && (hit.owner - 200) % 5 == 0 && (hit.owner - 200) / 5 < kFloorMonsters) {
        return (hit.owner - 200) / 5;
    }
    return -1;
}

void Position(CMonstorUnit &unit, int slot, float *position) {
    unit.chara[slot][0].GetPosition(position);
}

} // namespace expanded::confuse
