#pragma once

namespace dc::game {
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Steel Slingshot "Endurance Up": while its WHP is low (the game's warning, the HUD gauge blinking at a tenth of its
// maximum or less), each shot costs kEnduranceUpShot of the WHP; the weapon's own wear then applies Durable and
// Fragile on top, so they stack. Super Steve carrying its SynthSphere has that too, on its own WHP. A level-up of the
// Steel Slingshot itself adds kEnduranceUpEndurance endurance instead of 1, and kEnduranceUpDurability times the
// game's roll to its max WHP.
inline constexpr float kEnduranceUpShot = 0.5f;
inline constexpr float kEnduranceUpWarning = 0.1f;
inline constexpr int   kEnduranceUpEndurance = 2;
inline constexpr int   kEnduranceUpDurability = 2;

// Hook: BattleActionPlay_Jinn: the WHP a shot of Xiao's costs, wear the game's.
float EnduranceUpShotWear(float wear);

// Hooks: a level-up of weapon (CWeaponLevelUp::SetLevelUpWeaponData, WeaponLevelUpValueCalc): the endurance it adds
// (add, the game's 1) and its max-WHP roll (the game's).
int EnduranceUpEndurance(const WEAPON_HAVE &weapon, int add);
int EnduranceUpDurability(const WEAPON_HAVE &weapon, int roll);

} // namespace expanded
