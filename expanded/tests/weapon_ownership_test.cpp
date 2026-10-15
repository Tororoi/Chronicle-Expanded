#include <gtest/gtest.h>

#include "dngstatusdata.hpp"
#include "expanded/weapon_ownership.hpp"
#include "savedata.hpp"

namespace {

alignas(64) CSaveData save;

} // namespace

TEST(WeaponOwnership, FindsBagAndStorageWeaponsOfAnyCharacter) {
    CUserStatus *status = save.GetDngStatus();
    CStockItem  *stock = save.GetStockItem();

    EXPECT_FALSE(expanded::WeaponOwned(status, stock, ITEM_WEAPON_MACHO_SWORD));

    status->chara_weapons[0][9].item_no = ITEM_WEAPON_MACHO_SWORD;
    EXPECT_TRUE(expanded::WeaponOwned(status, stock, ITEM_WEAPON_MACHO_SWORD));

    status->chara_weapons[5][0].item_no = ITEM_WEAPON_BLESSING_GUN;
    EXPECT_TRUE(expanded::WeaponOwned(status, stock, ITEM_WEAPON_BLESSING_GUN));

    stock->weapons[29].item_no = ITEM_WEAPON_SCORPION;
    EXPECT_TRUE(expanded::WeaponOwned(status, stock, ITEM_WEAPON_SCORPION));
    EXPECT_FALSE(expanded::WeaponOwned(status, stock, ITEM_WEAPON_FLAMINGO));
}

TEST(WeaponOwnership, IgnoresTheWaitingSlotAndEmptyIds) {
    CUserStatus *status = save.GetDngStatus();
    CStockItem  *stock = save.GetStockItem();

    status->chara_weapons[1][10].item_no = ITEM_WEAPON_FLAMINGO;
    EXPECT_FALSE(expanded::WeaponOwned(status, stock, ITEM_WEAPON_FLAMINGO));
    EXPECT_FALSE(expanded::WeaponOwned(status, stock, 0));
}
