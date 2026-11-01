#include "expanded/weapons/toan/antique_sword.hpp"

#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

int AntiqueSwordPrice(int item_no, int sell, int price) {
    return sell != 0 && item_no == ITEM_WEAPON_ANTIQUE_SWORD && WeaponAbilitiesOn() ? kAntiqueSwordSellPrice : price;
}

bool TreasureSenseActive() {
    return WeaponActive(ITEM_WEAPON_ANTIQUE_SWORD);
}

} // namespace expanded
