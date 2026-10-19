#include <gtest/gtest.h>

#include "dngmessageman.hpp"
#include "expanded/weapons/toan/macho_sword.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// A save whose Toan holds the Macho Sword (or not) in his bag.
struct Save {
    expanded::test::TestSave save;

    explicit Save(bool macho) {
        if (macho) {
            save.status().chara_weapons[0][3].item_no = ITEM_WEAPON_MACHO_SWORD;
        }
    }
};

WEAPON_HAVE Weapon(int abs) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = ITEM_WEAPON_DAGGER;
    weapon.experience = static_cast<s16>(abs);
    return weapon;
}

} // namespace

TEST(MachoSword, WithoutItTheGameFillsToTheMax) {
    Save        save(false);
    WEAPON_HAVE weapon = Weapon(98);
    EXPECT_FALSE(expanded::OvertrainingAbs(weapon, 6, 100));
    EXPECT_EQ(weapon.experience, 98);
}

TEST(MachoSword, OwnedTheAbsFillsPastTheMaxToTwiceIt) {
    Save        save(true);
    WEAPON_HAVE weapon = Weapon(98);
    DngMessMan.message = 0;
    ASSERT_TRUE(expanded::OvertrainingAbs(weapon, 6, 100));
    EXPECT_EQ(weapon.experience, 104);
    EXPECT_EQ(DngMessMan.message, 150); // ABS MAX as it crosses the max

    DngMessMan.message = 0;
    ASSERT_TRUE(expanded::OvertrainingAbs(weapon, 90, 100));
    EXPECT_EQ(weapon.experience, 194);
    EXPECT_EQ(DngMessMan.message, 0);
    ASSERT_TRUE(expanded::OvertrainingAbs(weapon, 50, 100));
    EXPECT_EQ(weapon.experience, 200);

    WEAPON_HAVE low = Weapon(10);
    ASSERT_TRUE(expanded::OvertrainingAbs(low, 5, 100));
    EXPECT_EQ(low.experience, 15);
}

TEST(MachoSword, ALevelUpCarriesTheSurplus) {
    EXPECT_EQ(expanded::OvertrainingCarry(Weapon(116), 100), 16);
    EXPECT_EQ(expanded::OvertrainingCarry(Weapon(100), 100), 0);
    EXPECT_EQ(expanded::OvertrainingCarry(Weapon(40), 100), 0); // a Powerup Powder's level-up
}

TEST(MachoSword, TheGaugeFillsToFull) {
    EXPECT_EQ(expanded::OvertrainingGaugeAbs(150, 100), 100);
    EXPECT_EQ(expanded::OvertrainingGaugeAbs(60, 100), 60);
}
