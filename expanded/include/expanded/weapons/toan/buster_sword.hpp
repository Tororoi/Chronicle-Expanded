#pragma once

namespace dc::game {
struct ATTACH_LIST;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Buster Sword "Buster Boost": every anti-monster stat an attachment raises (the Dinoslayer to the Mage Slayer, the
// gems, the Sun, a SynthSphere) is worth one more on a Buster Sword, and on Super Steve while he carries its SynthSphere.
// Its entry holds the value, which the menus, damage, build-up checks and the level-up's absorb all read (so Super Steve
// levelling up keeps the boost though the sphere is absorbed with the rest); off those weapons it is the attachment's
// own again. An attachment's own values come from its data; a SynthSphere's are its own, so its entry carries
// kBusterBoostedSphere while it holds the point.
inline constexpr int kBusterBoost = 1;
inline constexpr int kBusterBoostedSphere = 0x01; // in ATTACH_LIST::unk_1F

// Buster Sword "Buster Slot": any weapon carrying the Buster Sword's SynthSphere has one more regular attachment slot
// after its own while it does, and on Super Steve that slot doubles what it holds. Take the sphere off (or swap another
// attachment in for it) and the extra slot's attachment moves into an empty regular slot, or, with every slot full, into
// the slot the sphere left; a swap that would leave it nowhere is refused. A slot still holding an attachment stays open.

// Whether the weapon's entries take the boost: the Buster Sword, or Super Steve carrying its sphere.
bool BusterBoosted(const WEAPON_HAVE *weapon);

// An attachment entry with the boost on or off.
void BusterBoostEntry(ATTACH_LIST &entry, bool boosted);

// The weapon's attachments boosted as it now takes them, and its extra slot doubled on Super Steve (else not); also
// run as WeaponAllValueSet totals a weapon, so a weapon set up before (a save, a stage) follows.
void BusterBoostWeapon(WEAPON_HAVE &weapon);

// Hook: the weapon menu having moved an attachment entry onto a weapon, or (weapon null) off one onto the board or
// into the hand.
void BusterBoostPlaced(WEAPON_HAVE *weapon, ATTACH_LIST &entry);

// Hook: the weapon menu having set an entry onto a weapon, swapping out whatever was there into the hand.
void BusterBoostSwapped(WEAPON_HAVE &weapon, ATTACH_LIST &placed, ATTACH_LIST &taken);

// The slots the weapon has now (holes: its own): one more while the extra slot is open.
int BusterHoles(const WEAPON_HAVE &weapon, int holes);

// Slot hole's kind (kind: the weapon data's; 0 none, 1 regular, 2 SynthSphere): regular for the open extra slot.
int BusterHoleKind(const WEAPON_HAVE &weapon, int hole, int kind);

// Hook: the weapon menu about to set the held attachment into slot hole (swapping out what is there): whether to refuse
// it, the extra slot's attachment having nowhere to go once the sphere leaves.
bool BusterSwapRefused(const WEAPON_HAVE &weapon, int hole, const ATTACH_LIST &held);

// Hook: the weapon menu having taken an attachment off slot hole into the hand (taken), or swapped one in for it: when
// that was the Buster Sword's sphere, the extra slot's attachment moved (vacated: hole when it is empty now, else -1).
void BusterSphereLeft(WEAPON_HAVE &weapon, const ATTACH_LIST &taken, int vacated);

// Hook: the weapon menu having put the held attachment back on the weapon it came from, swapping out what had moved into
// its slot (held, now in the hand, held_no the hand's item): that put back into the reopened extra slot, the hand
// emptied (true).
bool BusterCancelled(WEAPON_HAVE &weapon, ATTACH_LIST &held, short &held_no);

} // namespace expanded
