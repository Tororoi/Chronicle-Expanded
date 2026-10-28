#include "expanded/status/confuse.hpp"

#include <libvu0.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>

#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/status/poison.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

using namespace confuse;

constexpr int   kConfuseSteps = 20 * kStepsPerSecond;
constexpr float kConfuseChance = 5.0f;

// A monster's confusion, in the event log: confused, its 20 seconds rolled by a hit carrying Confuse, its target (another
// monster's slot, -1 the player) or its senses back.
void LogConfusion(const char *step, int slot, int target = -2) {
    LogEvent("confuse", target == -2 ? std::format("\"step\":\"{}\",\"monster\":{}", step, slot)
                                     : std::format("\"step\":\"{}\",\"monster\":{},\"target\":{}", step, slot, target));
}

void LogTarget(int slot, int target) {
    if (State().target[slot] == target) {
        return;
    }
    State().target[slot] = target;
    LogConfusion("target", slot, target);
    if (target >= 0) {
        std::fprintf(stderr, "confuse: monster %d goes after monster %d\n", slot, target);
    } else {
        std::fprintf(stderr, "confuse: monster %d goes after the player\n", slot);
    }
}

// The state, kept for unit: another unit's is put away.
Confusion &Adopt(CMonstorUnit &unit) {
    Confusion &state = State();
    if (state.unit != &unit) {
        StarsOff();
        int tick = state.tick;
        state = Confusion();
        state.unit = &unit;
        state.tick = tick;
    }
    return state;
}

} // namespace

int ConfuseOptionMessage(int bit) {
    return bit == kConfuseOptionBit ? kConfuseNameMessage : bit + 69;
}

void ConfuseTick() {
    if (!MonstersStepping()) {
        return;
    }
    Confusion &state = State();
    state.tick++;
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        if (state.until[slot] == 0) {
            continue;
        }
        if (state.unit != nullptr && state.unit->monster[slot].hp <= 0) {
            state.Forget(slot);
            continue;
        }
        if (state.tick >= state.until[slot]) {
            state.Forget(slot);
            std::fprintf(stderr, "confuse: monster %d comes to its senses\n", slot);
            LogConfusion("senses", slot);
        }
    }
}

void ConfuseFloorReset(bool drop_stars) {
    StarsOff();
    State() = Confusion();
    if (drop_stars) {
        StarsDrop();
    }
}

void ConfuseFloorLoaded() {
    ConfuseFloorReset(true);
    StarsLoad();
}

void ConfuseStep(CMonstorUnit &unit, int slot) {
    if (AllyUnit(unit)) {
        return;
    }
    Confusion &state = Adopt(unit);
    if (unit.monster[slot].hp <= 0) {
        state.Forget(slot);
        return;
    }
    int provoker = Provoker(unit, slot);
    if (provoker >= 0) {
        LogTarget(slot, provoker);
    } else if (state.until[slot] == 0 && state.target[slot] != -2) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d goes back to the player\n", slot);
    }
}

void ConfuseRoll(CMonstorUnit &unit, int slot) {
    if (!(unit.effect[slot].hit_attributes & kAbilityConfuse) || AllyUnit(unit)) {
        return;
    }
    MONSTOR &monster = unit.monster[slot];
    if (monster.kind == MONSTER_KIND_NO_LOCK_ON || monster.hp <= 0) {
        return;
    }
    if (!(100.0f * GameFraction() < kConfuseChance)) {
        return;
    }
    Confusion &state = State();
    if (state.until[slot] == 0) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d is confused\n", slot);
        LogConfusion("confused", slot);
    }
    LogConfusion("rolled", slot);
    state.until[slot] = state.tick + kConfuseSteps;
}

void ConfuseMonster(CMonstorUnit &unit, int slot, int steps) {
    if (slot < 0 || slot >= kFloorMonsters || steps <= 0 || AllyUnit(unit) || unit.monster[slot].hp <= 0) {
        return;
    }
    Confusion &state = Adopt(unit);
    if (state.until[slot] == 0) {
        state.target[slot] = -2;
        std::fprintf(stderr, "confuse: monster %d is confused\n", slot);
        LogConfusion("confused", slot);
    }
    state.until[slot] = std::max(state.until[slot], state.tick + steps);
}

bool MonsterConfused(const CMonstorUnit &unit, int slot) {
    return State().unit == &unit && Confused(unit, slot);
}

void ConfuseEndAll() {
    StarsOff();
    Confusion &state = State();
    for (int slot = 0; slot < kFloorMonsters; slot++) {
        if (state.until[slot] != 0) {
            std::fprintf(stderr, "confuse: monster %d comes to its senses\n", slot);
            LogConfusion("senses", slot);
        }
        state.Forget(slot);
    }
    std::memset(state.contact, 0, sizeof(state.contact));
}

void ConfuseEnd(int slot) {
    if (slot < 0 || slot >= kFloorMonsters || State().until[slot] == 0) {
        return;
    }
    State().Forget(slot);
    std::fprintf(stderr, "confuse: monster %d comes to its senses\n", slot);
    LogConfusion("senses", slot);
}

bool ConfuseTarget(CMonstorUnit &unit, int slot, float *position) {
    if (slot < 0 || slot >= kFloorMonsters || State().unit != &unit) {
        return false;
    }
    sceVu0FVECTOR self;
    Position(unit, slot, self);
    if (Confused(unit, slot)) {
        // The nearest across the ground of the other monsters in play and the player.
        sceVu0FVECTOR other;
        int           best = -1;
        float         best_distance = std::hypot(CharaMain.pos[0] - self[0], CharaMain.pos[2] - self[2]);
        for (int i = 0; i < kFloorMonsters; i++) {
            if (i == slot || !InPlay(unit, i)) {
                continue;
            }
            Position(unit, i, other);
            float distance = std::hypot(other[0] - self[0], other[2] - self[2]);
            if (distance < best_distance) {
                best_distance = distance;
                best = i;
            }
        }
        LogTarget(slot, best);
        if (best < 0) {
            return false;
        }
        MonsterAimPoint(unit, best, self[0], self[2], position);
        return true;
    }
    int provoker = Provoker(unit, slot);
    if (provoker < 0) {
        return false;
    }
    MonsterAimPoint(unit, provoker, self[0], self[2], position);
    return true;
}

void ConfuseScriptPlayerPosition(float *position) {
    if (!ConfuseTarget(*NowMonstorUnit, NowMonstorUnit->current_monster, position)) {
        sceVu0CopyVector(position, CharaMain.pos);
    }
}

} // namespace expanded
