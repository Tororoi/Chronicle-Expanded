#pragma once

class CCharacter;

namespace expanded {

// Angel Shooter "Guardian Grace", as the mod has it: with the Angel Shooter in hand (its Heal ability not stripped), or
// Super Steve carrying its SynthSphere, while Xiao guards the Heal ability's tick comes every kGuardianGraceFrames: its
// counter is held no further than that from the tick, so the game's own +1 HP comes each time (Super Steve's own record
// lacks the ability, so the tick heals with it here). As the guard starts she gets the healing spring's sparkle, a soft
// white flash and the heal chime; for the whole guard her colour swells from kGuardianGraceLow to kGuardianGraceHigh and
// back with the tick, peaking as it heals.
inline constexpr int   kGuardianGraceFrames = 60;
inline constexpr float kGuardianGraceLow = 12.0f, kGuardianGraceHigh = 48.0f;
inline constexpr float kGuardianGraceFlash = 110.0f;

// Whether the weapon in Xiao's hand carries it.
bool GuardianGraceCarried();
// Whether it is on: carried, with the ability, while she guards.
bool GuardianGraceOn();

// Hooks: a dungeon step (the guard's start and end); the heal tick's counter after its step (period the tick's), held
// within kGuardianGraceFrames of it while on; whether the tick heals for a weapon whose flags lack the Heal ability;
// her colour while she guards (CCharacter::Draw).
void GuardianGraceStep();
int  GuardianGraceCounter(int counter, int period);
bool GuardianGraceHeals();
void GuardianGraceLight(const CCharacter &character, float *ambient);

// Taken down (the floor changed), for tests too.
void GuardianGraceReset();

} // namespace expanded
