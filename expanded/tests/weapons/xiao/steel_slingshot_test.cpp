#include <gtest/gtest.h>

#include "expanded/weapons/xiao/steel_slingshot.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"
#include "weaponlevelup.hpp"

namespace {

// weapon_no in Xiao's hand (carrying sphere when given), whp of its 40 WHP left on her equipped record.
struct Holding {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding;

    Holding(int weapon_no, float whp, int sphere = 0) : wielding(weapon_no, sphere) {
        CUserStatus &status = floor.status();
        status.cur_chara = CHARA_XIAO;
        status.equipped_weapon_slot[CHARA_XIAO] = 0;
        WEAPON_HAVE &record = status.chara_weapons[CHARA_XIAO][0];
        record = wielding.weapon();
        record.durability = 40;
        record.durability_f = whp;
    }
};

} // namespace

TEST(SteelSlingshot, AtLowWhpAShotCostsHalf) {
    {
        Holding steel(ITEM_WEAPON_STEEL_SLINGSHOT, 4.0f); // a tenth: the warning
        EXPECT_FLOAT_EQ(expanded::EnduranceUpShotWear(1.0f), 0.5f);
    }
    {
        Holding steel(ITEM_WEAPON_STEEL_SLINGSHOT, 5.0f);
        EXPECT_FLOAT_EQ(expanded::EnduranceUpShotWear(1.0f), 1.0f);
    }
    {
        Holding other(ITEM_WEAPON_WOODEN_SLINGSHOT, 4.0f);
        EXPECT_FLOAT_EQ(expanded::EnduranceUpShotWear(1.0f), 1.0f);
    }
    Holding steve(ITEM_WEAPON_SUPER_STEVE, 4.0f, ITEM_WEAPON_STEEL_SLINGSHOT);
    EXPECT_FLOAT_EQ(expanded::EnduranceUpShotWear(1.0f), 0.5f);
}

TEST(SteelSlingshot, ALevelUpAddsTwoEnduranceAndTwiceTheWhpRoll) {
    WEAPON_HAVE steel = {};
    steel.item_no = ITEM_WEAPON_STEEL_SLINGSHOT;
    steel.endurance = 10;
    steel.durability = 30;
    WEAPON_HAVE leveled = {};
    WeaponLevelUpValueCalc(&steel, &leveled, 1, 0);
    EXPECT_EQ(leveled.endurance, 12);
    EXPECT_GE(leveled.durability, 32); // twice a roll of 1 to 3
    EXPECT_LE(leveled.durability, 36);
    EXPECT_EQ((leveled.durability - 30) % 2, 0);

    WEAPON_HAVE wooden = steel;
    wooden.item_no = ITEM_WEAPON_WOODEN_SLINGSHOT;
    WeaponLevelUpValueCalc(&wooden, &leveled, 1, 0);
    EXPECT_EQ(leveled.endurance, 11);
    EXPECT_LE(leveled.durability, 33);
}
