#include "expanded/status/poison.hpp"

#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

// Steps the floor has run with monsters stepping unpaused.
int step_clock;

// step_clock when each monster was poisoned, and a summoned ally's (ally_unit.hpp) after the floor's.
int poison_start[kFloorMonsters + 1];

int &PoisonStart(int slot) {
    return poison_start[AllyStepping() ? kFloorMonsters : slot];
}

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
        PoisonStart(slot) = step_clock;
        MonsterPoisoned(*target);
    }

    target->slow_timer = 0;
}

int PoisonTickDamage(const MONSTOR *target) {
    int damage = target->max_hp * target->status_chance / 1000;

    return damage < 1 ? 1 : damage;
}

void PoisonCureLapsed(MONSTOR *target, int slot) {
    if (target->poison_timer > 0 && step_clock - PoisonStart(slot) > kPoisonSteps) {
        target->poison_timer = 0;
    }
}

void PoisonCureFinished(MONSTOR *target, int slot) {
    if (step_clock - PoisonStart(slot) >= kPoisonSteps) {
        target->poison_timer = 0;
    }
}

void PoisonCureDead(MONSTOR *target) {
    target->poison_timer = 0;
}

} // namespace expanded
