#pragma once

namespace dc::game {
struct ATTACH_LIST;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// The weapons whose attachments are worth more on them, each boost held in the attachment's entry: Buster Boost (anti
// stats: buster_sword.hpp, which also keeps its extra slot) and Blessing Boost (element stats: blessing_gun.hpp).

// The weapon's attachments boosted as it now takes them; also run as WeaponAllValueSet totals a weapon, so a weapon set
// up before (a save, a stage) follows.
void AttachBoostWeapon(WEAPON_HAVE &weapon);

// Hook: the weapon menu having moved an attachment entry onto a weapon, or (weapon null) off one onto the board or
// into the hand.
void AttachBoostPlaced(WEAPON_HAVE *weapon, ATTACH_LIST &entry);

// Hook: the weapon menu having set an entry onto a weapon, swapping out whatever was there into the hand.
void AttachBoostSwapped(WEAPON_HAVE &weapon, ATTACH_LIST &placed, ATTACH_LIST &taken);

} // namespace expanded
