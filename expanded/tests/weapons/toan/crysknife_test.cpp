#include <gtest/gtest.h>

#include "expanded/weapons/toan/crysknife.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

TEST(Crysknife, ThrownGemsDealTwiceTheirDamage) {
    {
        expanded::test::Wielding knife(ITEM_WEAPON_CRYSKNIFE);
        EXPECT_EQ(expanded::CrystalAffinityDamage(30), 60);
        EXPECT_EQ(expanded::CrystalAffinityDamage(210), 420);
    }
    {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CRYSKNIFE);
        EXPECT_EQ(expanded::CrystalAffinityDamage(30), 60);
    }
    {
        expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
        EXPECT_EQ(expanded::CrystalAffinityDamage(30), 30);
    }
}
