#pragma once

#include <cstdint>

#include "expanded/weapon_abilities.hpp"

class CMonstorUnit;
struct PortDecodedTexture;

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

// A landed hit: confuses the monster 5% of the time when the hit carries Confuse.
void ConfuseRoll(CMonstorUnit &unit, int slot);

// The position a monster's script sees as the player's: false to keep the player's.
bool ConfuseTarget(CMonstorUnit &unit, int slot, float *position);

// The player's position as the current monster's script sees it.
void ConfuseScriptPlayerPosition(float *position);

// The index of another monster's hit that may hurt monster victim's sphere at position, or -1.
int  ConfuseFriendlyHit(CMonstorUnit &unit, int victim, const float *position, float radius);
// Records that hit landed on victim, and ends it there if it is a shot.
void ConfuseLanded(CMonstorUnit &unit, int hit, int victim);

// Another monster's hit on sphere of the current monster: CheckDmg's result, or -1 for none.
int ConfuseMonsterHit(CMonstorUnit &unit, int sphere);

// Draws the stars over the confused monsters.
void ConfuseStarsDraw();

// Lays the Confuse icon into the charaface sheet's empty tile beside Steal.
void ConfuseIconTexture(const char *name, PortDecodedTexture &texture);

} // namespace expanded
