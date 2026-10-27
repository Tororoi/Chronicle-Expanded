#pragma once

struct WEAPON_HAVE;

namespace expanded {

// Super Steve takes on the effect of the weapon whose SynthSphere is attached, one sphere at a time.
// Ownership passives and upgrade abilities stay with the weapon itself: test those with WeaponWielded.

// The weapon the SynthSphere attached to weapon was made from, or 0 for none.
int SphereWeapon(const WEAPON_HAVE &weapon);

// The weapon Super Steve channels in the active character's hand (NowWeaponHave), or 0.
int SuperSteveSphere();

// The weapon in the active character's hand (NowWeaponHave), or 0 (none, or the Weapon Abilities option off).
int HeldWeapon();

// Whether weapon_no is the active character's weapon (NowWeaponHave).
bool WeaponWielded(int weapon_no);

// Whether weapon_no's effect is on: wielded, or channelled by Super Steve's sphere.
bool WeaponActive(int weapon_no);

// The weapon whose cell of basefx01 the player's pellets are drawn with: the Steel Slingshot's stone for Double Impact's
// pairs, a slingshot sphere's, else the wielded one's.
int PelletWeapon();

// The Weapon Abilities option (config.json's expanded.weapon_abilities, on by default). Off, no weapon is wielded,
// channelled or owned as Expanded's weapon effects ask (WeaponWielded, HeldWeapon, SphereWeapon and the ownership tests
// answer none), so every one of them stays off and the weapons play as the game's own. What a weapon was given when
// it was made or built up stays with it.
inline constexpr const char *kWeaponAbilities = "weapon_abilities";
bool WeaponAbilitiesOn();

// Copies the equipped Super Steve's sphere icon from the weapon icon sheet into itempack's spare cell.
void SphereIconCopy(char *source);

// Draws that icon over Steve's on the dungeon HUD, at the HUD's alpha.
void SphereIconDraw(int alpha);

} // namespace expanded
