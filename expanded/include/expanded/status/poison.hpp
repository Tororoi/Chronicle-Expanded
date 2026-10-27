#pragma once

namespace dc::game {
struct MONSTOR;
} // namespace dc::game

namespace expanded {

// Steps a monster's poison lasts: ten of its 180-step ticks, 30 seconds.
inline constexpr int kPoisonSteps = 1800;

// Whether the monsters step this frame: the floor is drawn and no event or hold stops them. Poison, confusion and
// blindness count only these frames.
bool MonstersStepping();

// Advances the floor step clock poison is timed by, on each step monsters step unpaused.
void MonsterStepClockAdvance();

// Poisons a monster; one already poisoned keeps its poison as it is.
void Poison(MONSTOR *target, int slot);

// Life a poison tick takes: max_hp * status_chance / 1000, at least 1.
int PoisonTickDamage(const MONSTOR *target);

// Cures a monster whose poison ran out while it wasn't stepped, before it ticks again.
void PoisonCureLapsed(MONSTOR *target, int slot);

// Cures a monster whose poison has lasted its full time, after its last tick.
void PoisonCureFinished(MONSTOR *target, int slot);

// Ends a dead monster's poison.
void PoisonCureDead(MONSTOR *target);

} // namespace expanded
