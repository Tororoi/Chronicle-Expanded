#include "expanded/weapons/toan/buster_sword.hpp"

#include "itemdata.hpp"

namespace expanded {

void BusterBoostPlaced(const WEAPON_HAVE *weapon, ATTACH_LIST &entry) {
    if (entry.item_no < ITEM_ATTACH_DINOSLAYER || entry.item_no > ITEM_ATTACH_MAGE_SLAYER) {
        return;
    }
    const ATTACH_DATA *data = GetAttachData(entry.item_no);
    if (data == nullptr) {
        return;
    }
    int  anti = entry.item_no - ITEM_ATTACH_DINOSLAYER; // the attachments are in the anti bytes' order
    bool boosted = weapon != nullptr && weapon->item_no == ITEM_WEAPON_BUSTER_SWORD;
    entry.vs_monster[anti] = static_cast<s8>(data->vs_monster[anti] + (boosted ? kBusterBoost : 0));
}

void BusterBoostSwapped(const WEAPON_HAVE &weapon, ATTACH_LIST &placed, ATTACH_LIST &taken) {
    BusterBoostPlaced(&weapon, placed);
    BusterBoostPlaced(nullptr, taken);
}

} // namespace expanded
