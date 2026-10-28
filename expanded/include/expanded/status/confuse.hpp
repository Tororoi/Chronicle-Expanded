#pragma once

#include <cstdint>
#include <optional>

#include "expanded/weapons/abilities.hpp"

namespace dc::game {
enum class MonsterDamageResult : int;
class CMonstorUnit;
} // namespace dc::game

namespace dc::render {
struct PortDecodedTexture;
} // namespace dc::render

namespace expanded {

// Its option bit and its name, the system bank's message 0x45.
inline constexpr int kConfuseOptionBit = 14;
inline constexpr int kConfuseNameMessage = 0x45;

// The system bank message naming an option bit in the menus.
int ConfuseOptionMessage(int bit);

// Counts confusion down, once a step the monsters are not held.
void ConfuseTick();

// The floor's monsters were set up: ends every confusion and loads the stars.
void ConfuseFloorLoaded();

// The floor's monsters were put back: ends every confusion, and drops the stars when drop_stars.
void ConfuseFloorReset(bool drop_stars);

// Start of a monster's damage check: tracks its unit and who it goes after.
void ConfuseStep(CMonstorUnit &unit, int slot);

// A landed hit: confuses a monster not yet confused 5% of the time when the hit carries Confuse.
void ConfuseRoll(CMonstorUnit &unit, int slot);

// A weapon's effect confusing a living monster for steps: true when it did. A monster already confused keeps its own end
// (false): no confusion resets or extends another's. And one monster's confusion ended now, the others' left as they are.
bool ConfuseMonster(CMonstorUnit &unit, int slot, int steps);
void ConfuseEnd(int slot);

// Whether a monster of unit is confused.
bool MonsterConfused(const CMonstorUnit &unit, int slot);

// Every monster's confusion ended, and what a confused one provoked.
void ConfuseEndAll();

// The position a monster's script sees as the player's: false to keep the player's. A confused monster goes for the
// nearest across the ground of the other monsters in play and the player, chosen afresh at every look (one killed is passed
// over at once), a provoked one for its provoker; a monster it goes for is marked where an attacker goes for its body
// (MonsterAimPoint).
bool ConfuseTarget(CMonstorUnit &unit, int slot, float *position);

// The monster a monster's script goes after through confusion (a confused one's target, a provoked one's provoker), or
// -1 (the player, or none).
int ConfuseTargetMonster(const CMonstorUnit &unit, int slot);

// The player's position as the current monster's script sees it.
void ConfuseScriptPlayerPosition(float *position);

// The index of another monster's hit that may hurt monster victim's sphere at position, or -1.
int  ConfuseFriendlyHit(CMonstorUnit &unit, int victim, const float *position, float radius);
// Records that hit landed on victim, and ends it there if it is a shot.
void ConfuseLanded(CMonstorUnit &unit, int hit, int victim);

// Another monster's hit on sphere of the current monster: CheckDmg's result, or nothing for none.
std::optional<MonsterDamageResult> ConfuseMonsterHit(CMonstorUnit &unit, int sphere);

// Draws the stars over the confused monsters.
void ConfuseStarsDraw();

// The stars at fade of their size (1 their own).
void ConfuseStarsFade(float fade);

// Lays the Confuse icon into the charaface sheet's empty tile beside Steal.
void ConfuseIconTexture(const char *name, PortDecodedTexture &texture);

} // namespace expanded
