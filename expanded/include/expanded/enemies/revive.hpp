#pragma once

class CMonstorUnit;

namespace expanded {

// Percent chance a monster with a real get-up motion revives with Harder Enemy AI on.
inline constexpr int kReviveChance = 30;
// Percent chance for the undead whose own death script revives them (retail: 10 to 18).
inline constexpr int kNativeReviveChance = 45;
// Share of its most life a monster gets back.
inline constexpr float kReviveLife = 0.4f;
// Steps a revived monster is safe from hits, from its collapse.
inline constexpr int kReviveInvincibleSteps = 120;
// Steps it lies still before getting up: this many and up to kReviveLieSpread more.
inline constexpr int kReviveLieSteps = 30;
inline constexpr int kReviveLieSpread = 40;
// Frames before a motion's end at which the next part of the revival starts.
inline constexpr float kReviveMotionEndMargin = 5.0f;
// The get-up motion and the idle every reviving monster has.
inline constexpr int kGetUpMotion = 10;
inline constexpr int kIdleMotion = 0;

// How a monster can come back from a kill.
enum class Revival {
    kGetUp,  // collapses, lies still and gets up (Harder Enemy AI)
    kScript, // its own death script rolls a revival
};

// What a monster of a script revives with, and the motion it collapses with for kGetUp.
struct ReviveSpecies {
    const char *script;
    Revival     revival;
    int         collapse_motion = 0;
    int         death_sound = -1;         // sound its death plays first, or -1
    bool        drops_loop_sound = false; // its death lets go of its loop sound
};

// Whether the Harder Enemy AI option is on.
bool HarderEnemyAiOn();

// The revival of a monster script, by name ("e50a"); nullptr for one that never revives.
const ReviveSpecies *ReviveSpeciesOf(const char *script);

// A weapon's rule that monster_no, just killed, stays down (Cross Hinder, the bone weapons).
using StaysDownRule = bool (*)(const CMonstorUnit &unit, int monster_no);

// Adds a rule; false when it is null or already added.
bool AddStaysDownRule(StaysDownRule rule);

// Whether any rule keeps monster_no down.
bool StaysDown(const CMonstorUnit &unit, int monster_no);

// The current monster's CheckDmg result: a kill may start a revival in place of its death script.
int ReviveCheckDmg(CMonstorUnit &unit, int result);

// Steps monster_no's revival in place of its script; false when it is not reviving.
bool ReviveStep(CMonstorUnit &unit, int monster_no);

// A script's _GET_RAND roll: the revival roll a kill decided, the first time a death script rolls 100.
int ReviveRoll(int bound, int roll);

// The floor's monsters were put back: ends every revival.
void ReviveClear();

} // namespace expanded
