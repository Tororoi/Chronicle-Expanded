#include "expanded/status/poison.hpp"

#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

// Steps the floor has run with monsters stepping unpaused.
int step_clock;

// step_clock when each monster was poisoned.
int poison_start[kFloorMonsters];

} // namespace

bool MonstersStepping() {
    return BtAllDrawFlag != 0 && BtEventMode == 0 && (driveStepHold | CMonUnitHold) == 0;
}

void MonsterStepClockAdvance() {
    if (MonstersStepping()) {
        step_clock++;
    }
}

void Poison(MONSTOR *target, int slot) {
    if (target->poison_timer <= 0) {
        target->poison_timer = 180;
        poison_start[slot] = step_clock;
        MonsterPoisoned(*target);
    }

    target->slow_timer = 0;
}

int PoisonTickDamage(const MONSTOR *target) {
    int damage = target->max_hp * target->status_chance / 1000;

    return damage < 1 ? 1 : damage;
}

void PoisonCureLapsed(MONSTOR *target, int slot) {
    if (target->poison_timer > 0 && step_clock - poison_start[slot] > kPoisonSteps) {
        target->poison_timer = 0;
    }
}

void PoisonCureFinished(MONSTOR *target, int slot) {
    if (step_clock - poison_start[slot] >= kPoisonSteps) {
        target->poison_timer = 0;
    }
}

void PoisonCureDead(MONSTOR *target) {
    target->poison_timer = 0;
}

} // namespace expanded
