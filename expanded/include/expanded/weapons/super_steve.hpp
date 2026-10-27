#pragma once

struct WEAPON_HAVE;

namespace expanded {

// Super Steve takes on the effect of the weapon whose SynthSphere is attached, one sphere at a time.
// Ownership passives and upgrade abilities stay with the weapon itself: test those with WeaponWielded.

// The weapon the SynthSphere attached to weapon was made from, or 0 for none.
int SphereWeapon(const WEAPON_HAVE &weapon);

// The weapon Super Steve channels in the active character's hand (NowWeaponHave), or 0.
int SuperSteveSphere();

// The weapon in the active character's hand (NowWeaponHave), or 0 (none, or plain weapons).
int HeldWeapon();

// Whether weapon_no is the active character's weapon (NowWeaponHave).
bool WeaponWielded(int weapon_no);

// Whether weapon_no's effect is on: wielded, or channelled by Super Steve's sphere.
bool WeaponActive(int weapon_no);

// The weapon whose cell of basefx01 the player's pellets are drawn with: the Steel Slingshot's stone for Double Impact's
// pairs, a slingshot sphere's, else the wielded one's.
int PelletWeapon();

// Plain weapons (--plain-weapons, DC_PLAIN_WEAPONS), for a recording's "before": no weapon is wielded, channelled or
// owned as Expanded's effects ask (WeaponWielded, HeldWeapon, SphereWeapon and the ownership tests answer none), so they
// stay off and the game's own weapons are what plays. A weapon's upgrades in the menus are untouched.
void SetPlainWeapons(bool plain);
bool PlainWeapons();

// Copies the equipped Super Steve's sphere icon from the weapon icon sheet into itempack's spare cell.
void SphereIconCopy(char *source);

// Draws that icon over Steve's on the dungeon HUD, at the HUD's alpha.
void SphereIconDraw(int alpha);

} // namespace expanded
