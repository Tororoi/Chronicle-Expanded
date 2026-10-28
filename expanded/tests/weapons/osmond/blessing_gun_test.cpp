#include <gtest/gtest.h>

#include "expanded/weapons/osmond/blessing_gun.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "stockitem.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestSave;

} // namespace

TEST(BlessedBait, WithoutTheGunTheRollTakesTheBait) {
    TestSave save;
    save.status().chara_weapons[CHARA_OSMOND][0].item_no = ITEM_WEAPON_SKUNK;
    EXPECT_FALSE(expanded::BlessedBaitKept());
}

TEST(BlessedBait, OwnedTheBaitStaysOnTheHook) {
    {
        TestSave save;
        save.status().chara_weapons[CHARA_OSMOND][4].item_no = ITEM_WEAPON_BLESSING_GUN;
        EXPECT_TRUE(expanded::BlessedBaitKept());
    }
    {
        TestSave save;
        save.stock().weapons[12].item_no = ITEM_WEAPON_BLESSING_GUN;
        EXPECT_TRUE(expanded::BlessedBaitKept());
    }
}

TEST(BlessedBait, OffWithTheWeaponAbilities) {
    TestSave    save;
    OptionScope options;
    save.status().chara_weapons[CHARA_OSMOND][0].item_no = ITEM_WEAPON_BLESSING_GUN;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_FALSE(expanded::BlessedBaitKept());
}
