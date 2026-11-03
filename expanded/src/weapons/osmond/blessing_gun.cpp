#include "expanded/weapons/osmond/blessing_gun.hpp"

#include <format>

#include "ebattle.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/ownership.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "menu_inventory.hpp"

namespace expanded {

namespace {

constexpr int kElements = 5;

} // namespace

bool BlessedBaitKept() {
    if (!WeaponOwned(ITEM_WEAPON_BLESSING_GUN)) {
        return false;
    }
    LogEvent("bait_kept", std::format("\"item\":{},\"sank\":{}", FishingGetEsaItemNo(), chara_fishing == ED_FISHING_SINK));
    return true;
}

bool BlessingBoosted(const WEAPON_HAVE *weapon) {
    return weapon != nullptr && CarriesWeapon(*weapon, ITEM_WEAPON_BLESSING_GUN);
}

void BlessingBoostEntry(ATTACH_LIST &entry, bool boosted) {
    if (entry.item_no < ITEM_ATTACH_START) {
        return;
    }
    if (entry.item_no == ITEM_ATTACH_SYNTHESIS_SPHERE) {
        // Its values are its own: the point added to (or taken from) each element it raises, once.
        bool holds = (entry.unk_1F & kBlessingBoostedSphere) != 0;
        if (holds == boosted) {
            return;
        }
        for (s8 &element : entry.elem) {
            if (element > 0) {
                element = static_cast<s8>(element + (boosted ? kBlessingBoost : -kBlessingBoost));
            }
        }
        entry.unk_1F = static_cast<u8>(boosted ? entry.unk_1F | kBlessingBoostedSphere : entry.unk_1F & ~kBlessingBoostedSphere);
        return;
    }
    const ATTACH_DATA *data = GetAttachData(entry.item_no);
    if (data == nullptr) {
        return;
    }
    for (int element = 0; element < kElements; element++) {
        if (data->elem[element] > 0) {
            entry.elem[element] = static_cast<s8>(data->elem[element] + (boosted ? kBlessingBoost : 0));
        }
    }
}

void BlessingBoostWeapon(WEAPON_HAVE &weapon) {
    bool boosted = BlessingBoosted(&weapon);
    for (ATTACH_LIST &entry : weapon.attach) {
        BlessingBoostEntry(entry, boosted);
    }
}

} // namespace expanded
