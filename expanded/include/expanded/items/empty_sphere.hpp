#pragma once

namespace dc::game {
struct ATTACH_LIST;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// The empty SynthSphere (a shooting star's, star_breaker.hpp: no weapon's, nothing in it). In a weapon's first slot it
// lets the weapon Status Break below +5, the level the game asks; it changes neither the weapon's level nor its stats,
// goes into the break with the weapon's other attachments, giving nothing, and is spent with the weapon. The weapon
// menu keeps it to the first slot, as the game keeps every SynthSphere to the slot that takes one, and says so when it
// is put in another; the break's confirmation says it is spent. With the Weapon Abilities option off it does none of
// this, as in the original game.

// The weapon menu's warning for an empty sphere put in a slot that takes none (WepMenu.warning_no, past the game's ten
// and the 7 Branch Sword's), and its allmenu message.
inline constexpr int kEmptySphereWarning = 11;
inline constexpr int kEmptySphereSlotMessage = 4201;

// Whether an attachment is an empty SynthSphere.
bool EmptySphere(const ATTACH_LIST &attach);

// Whether weapon may Status Break by an empty sphere in its first slot (WeaponStatusBreakEnable).
bool EmptySphereBreaks(const WEAPON_HAVE &weapon);

// Whether the weapon menu warns as it refuses held for a slot that takes no SynthSphere (WeaponMenuAttachWepKey).
bool EmptySphereWarns(const ATTACH_LIST &held);

} // namespace expanded
