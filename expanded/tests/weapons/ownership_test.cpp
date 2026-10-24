#include <gtest/gtest.h>

#include "dngstatusdata.hpp"
#include "expanded/weapons/ownership.hpp"
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

TEST(WeaponOwnership, CountsEveryCopy) {
    alignas(64) static CSaveData counted;
    CUserStatus                 *status = counted.GetDngStatus();
    CStockItem                  *stock = counted.GetStockItem();

    EXPECT_EQ(expanded::WeaponsOwned(status, stock, ITEM_WEAPON_FLAMINGO), 0);
    status->chara_weapons[1][0].item_no = ITEM_WEAPON_FLAMINGO;
    status->chara_weapons[1][4].item_no = ITEM_WEAPON_FLAMINGO;
    stock->weapons[3].item_no = ITEM_WEAPON_FLAMINGO;
    status->chara_weapons[1][10].item_no = ITEM_WEAPON_FLAMINGO;
    EXPECT_EQ(expanded::WeaponsOwned(status, stock, ITEM_WEAPON_FLAMINGO), 3);
}
