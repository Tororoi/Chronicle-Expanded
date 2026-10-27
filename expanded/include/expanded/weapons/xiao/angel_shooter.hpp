#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

// Angel Shooter "Guardian Grace": with the Angel Shooter or the Angel Gear in hand (its Heal ability not
// stripped; the Angel Gear always has it, abilities.hpp), or Super Steve carrying either's SynthSphere, while Xiao guards
// the Heal ability's tick comes every kGuardianGraceFrames (kGuardianGraceGearFrames with the Angel Gear's): its counter is
// held no further than that from the tick, so the game's own +1 HP comes each time (Super Steve's own record lacks the
// ability, so the tick heals with it here). As the guard starts the Angel Shooter's
// gives the healing spring's sparkle, a soft white flash and the heal chime, the Angel Gear's the golden burst of a
// character change, a warm flash and its jingle; for the whole guard her colour swells from the look's low to its high and
// back once every kGuardianGraceFrames, peaking as the tick heals (every other tick of the Angel Gear's).
inline constexpr int   kGuardianGraceFrames = 60;
inline constexpr int   kGuardianGraceGearFrames = 30;
inline constexpr float kGuardianGraceLow[3] = {12.0f, 12.0f, 12.0f}, kGuardianGraceHigh[3] = {48.0f, 48.0f, 48.0f};
inline constexpr float kGuardianGraceFlash[3] = {110.0f, 110.0f, 110.0f};
inline constexpr float kGuardianGraceGearLow[3] = {16.0f, 14.0f, 8.0f}, kGuardianGraceGearHigh[3] = {60.0f, 52.0f, 32.0f};
inline constexpr float kGuardianGraceGearFlash[3] = {130.0f, 114.0f, 75.0f};

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
