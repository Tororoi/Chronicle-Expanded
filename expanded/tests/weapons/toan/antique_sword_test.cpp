#include <gtest/gtest.h>

#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/antique_sword.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

TEST(AntiqueSword, SellsForThreeThousand) {
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_ANTIQUE_SWORD, 1, 750), 3000);
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_ANTIQUE_SWORD, 0, 3000), 3000); // buying: the table's
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_DAGGER, 1, 50), 50);
}

TEST(AntiqueSword, TreasureSenseInHandOrInSuperStevesSphere) {
    {
        expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
        EXPECT_FALSE(expanded::TreasureSenseActive());
    }
    {
        expanded::test::Wielding sword(ITEM_WEAPON_ANTIQUE_SWORD);
        EXPECT_TRUE(expanded::TreasureSenseActive());
    }
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_ANTIQUE_SWORD);
    EXPECT_TRUE(expanded::TreasureSenseActive());
}

TEST(AntiqueSword, OffWithTheWeaponAbilities) {
    expanded::test::Wielding    sword(ITEM_WEAPON_ANTIQUE_SWORD);
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_ANTIQUE_SWORD, 1, 750), 750);
    EXPECT_FALSE(expanded::TreasureSenseActive());
}
