#pragma once

class CUserStatus;
class CStockItem;

namespace expanded {

// Whether the weapon is in any character's hand or bag, or in storage, in the current save.
bool WeaponOwned(int weapon_no);

// Whether the weapon is in any of the six characters' ten weapon slots or the thirty storage slots.
bool WeaponOwned(const CUserStatus *status, const CStockItem *stock, int weapon_no);

// How many of the weapon are in those slots.
int WeaponsOwned(int weapon_no);
int WeaponsOwned(const CUserStatus *status, const CStockItem *stock, int weapon_no);

} // namespace expanded
