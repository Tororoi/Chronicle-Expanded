#include "expanded/weapons/ownership.hpp"

#include "dngstatusdata.hpp"
#include "savedata.hpp"
#include "stockitem.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// Slots 0-9 of each character's weapons; slot 10 holds a weapon waiting for room.
constexpr int kBagSlots = 10;

} // namespace

bool WeaponOwned(int weapon_no) {
    return WeaponsOwned(weapon_no) > 0;
}

bool WeaponOwned(const CUserStatus *status, const CStockItem *stock, int weapon_no) {
    return WeaponsOwned(status, stock, weapon_no) > 0;
}

int WeaponsOwned(int weapon_no) {
    if (SaveData == nullptr) {
        return 0;
    }

    return WeaponsOwned(SaveData->GetDngStatus(), SaveData->GetStockItem(), weapon_no);
}

int WeaponsOwned(const CUserStatus *status, const CStockItem *stock, int weapon_no) {
    if (weapon_no <= 0) {
        return 0;
    }

    int owned = 0;
    for (int chara = 0; chara < 6; chara++) {
        for (int slot = 0; slot < kBagSlots; slot++) {
            if (status->chara_weapons[chara][slot].item_no == weapon_no) {
                owned++;
            }
        }
    }

    for (const WEAPON_HAVE &weapon : stock->weapons) {
        if (weapon.item_no == weapon_no) {
            owned++;
        }
    }

    return owned;
}

} // namespace expanded
