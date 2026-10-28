#include <gtest/gtest.h>

#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"
#include "itemdata.hpp"
#include "stockitem.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

TEST(Flamingo, LockOnDistanceDoublesDownTheLine) {
    expanded::test::TestFloor floor;
    floor.status().cur_chara = CHARA_XIAO;
    for (int weapon : {ITEM_WEAPON_FLAMINGO, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_ANGE_GEAR}) {
        {
            expanded::test::Wielding slingshot(weapon);
            EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.4f), 2.8f) << weapon;
        }
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, weapon);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.4f), 2.8f) << weapon;
    }
    for (int weapon : {ITEM_WEAPON_MATADOR, ITEM_WEAPON_HARDSHOOTER, ITEM_WEAPON_SUPER_STEVE}) {
        expanded::test::Wielding other(weapon);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.4f), 1.4f) << weapon;
    }
}

TEST(Flamingo, SuperSteveCarryingAHolyBladeReachesAsFar) {
    expanded::test::TestFloor floor;
    floor.status().cur_chara = CHARA_XIAO;
    for (int weapon : {ITEM_WEAPON_CROSS_HINDER, ITEM_WEAPON_BIG_BANG, ITEM_WEAPON_SWORD_OF_ZEUS}) {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, weapon);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.4f), 2.8f) << weapon;
    }
}

TEST(Flamingo, EachOneOwnedWidensEveryBaitsNoticeUpToThree) {
    expanded::test::TestSave save;
    EXPECT_FLOAT_EQ(expanded::BaitNoticeRadius(ITEM_MIMI, 25.0f), 25.0f);

    save.status().chara_weapons[CHARA_XIAO][2].item_no = ITEM_WEAPON_FLAMINGO;
    EXPECT_FLOAT_EQ(expanded::BaitNoticeRadius(ITEM_MIMI, 25.0f), 35.0f);
    save.stock().weapons[0].item_no = ITEM_WEAPON_FLAMINGO;
    EXPECT_FLOAT_EQ(expanded::BaitNoticeRadius(ITEM_EVY, 50.0f), 70.0f);

    save.stock().weapons[1].item_no = ITEM_WEAPON_FLAMINGO;
    save.stock().weapons[2].item_no = ITEM_WEAPON_FLAMINGO;
    EXPECT_FLOAT_EQ(expanded::BaitNoticeRadius(ITEM_MIMI, 25.0f), 55.0f);
}
