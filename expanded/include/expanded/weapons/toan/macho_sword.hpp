#pragma once

struct WEAPON_HAVE;

namespace expanded {

// Macho Sword "Overtraining": while a Macho Sword is owned (in hand, a bag or storage), every weapon's ABS keeps
// filling past its max, up to twice it, and on a level-up the surplus over the max carries into the new level (owned
// or not, so a weapon filled before the sword left still keeps it).
inline constexpr int kOvertrainingLimit = 2;

// Hooks: CMonstorUnit::Step paying a kill's abs into the active character's weapon (true when it paid it: the
// sword owned), and CWeaponLevelUp::SetLevelUpWeaponData's ABS for the level it opens (max_abs the level's it leaves).
bool OvertrainingAbs(WEAPON_HAVE &weapon, int abs, int max_abs);
int  OvertrainingCarry(const WEAPON_HAVE &weapon, int max_abs);

// Hook: the dungeon HUD's ABS gauge, full at the max however far past it the weapon has filled.
int OvertrainingGaugeAbs(int abs, int max_abs);

} // namespace expanded
