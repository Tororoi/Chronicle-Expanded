#include <gtest/gtest.h>

#include "expanded/weapons/toan/small_sword.hpp"
#include "expanded/weapons/toan/tsukikage.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::test::Wielding;

TEST(Tsukikage, ChargeBuildsTwiceAsFast) {
    for (int weapon : {ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_HEAVEN_S_CLOUD}) {
        Wielding wielding(weapon);
        EXPECT_FLOAT_EQ(expanded::MoonlitFocusCharge(1.0f / 60.0f), 2.0f / 60.0f);
        EXPECT_TRUE(expanded::QuickDrawSword());
    }

    Wielding small_sword(ITEM_WEAPON_SMALL_SWORD);
    EXPECT_FLOAT_EQ(expanded::MoonlitFocusCharge(1.0f / 60.0f), 1.0f / 60.0f);
}

TEST(Tsukikage, SpherePelletsFlyTwiceAsFast) {
    for (int sphere : {ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_HEAVEN_S_CLOUD}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        float    vector[4] = {3.0f, -1.0f, 4.0f, 1.0f};
        expanded::MoonlitFocusShot(vector);
        EXPECT_FLOAT_EQ(vector[0], 6.0f);
        EXPECT_FLOAT_EQ(vector[1], -2.0f);
        EXPECT_FLOAT_EQ(vector[2], 8.0f);
        EXPECT_FLOAT_EQ(vector[3], 1.0f);
        EXPECT_TRUE(expanded::QuickDrawSphere());
    }

    for (int sphere : {static_cast<int>(ITEM_WEAPON_SMALL_SWORD), 0}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        float    vector[4] = {3.0f, -1.0f, 4.0f, 1.0f};
        expanded::MoonlitFocusShot(vector);
        EXPECT_FLOAT_EQ(vector[0], 3.0f);
    }

    // The sword in Toan's hand leaves pellets alone.
    Wielding tsukikage(ITEM_WEAPON_TSUKIKAGE);
    float    vector[4] = {3.0f, -1.0f, 4.0f, 1.0f};
    expanded::MoonlitFocusShot(vector);
    EXPECT_FLOAT_EQ(vector[0], 3.0f);
}
