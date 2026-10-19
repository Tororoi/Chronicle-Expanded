#pragma once

struct ATTACH_LIST;
struct WEAPON_HAVE;

namespace expanded {

// Buster Sword "Buster Boost": an anti-monster attachment (Dinoslayer to Mage Slayer) is worth one more on a Buster
// Sword (+4 for +3). Its entry holds the value, which the menus, damage, build-up checks and the level-up's absorb all
// read; off the sword it is the attachment's own again.
inline constexpr int kBusterBoost = 1;

// Hook: the weapon menu having moved an attachment entry onto a weapon, or (weapon null) off one onto the board.
void BusterBoostPlaced(const WEAPON_HAVE *weapon, ATTACH_LIST &entry);

// Hook: the weapon menu having set an entry onto a weapon, swapping out whatever was there into the hand.
void BusterBoostSwapped(const WEAPON_HAVE &weapon, ATTACH_LIST &placed, ATTACH_LIST &taken);

} // namespace expanded
