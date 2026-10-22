#include <gtest/gtest.h>

#include "expanded/core/random.hpp"
#include "expanded/weapons/toan/sun_sword.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

namespace {

// How many of n kills drop a Sun attachment.
int Harvested(int n, int drop_item = -1) {
    CMonstorUnit *unit = nullptr;
    int           suns = 0;
    for (int i = 0; i < n; i++) {
        suns += expanded::SolarHarvestDrop(*unit, 0, drop_item) == ITEM_ATTACH_SUN ? 1 : 0;
    }
    return suns;
}

} // namespace

TEST(SunSword, SolarHarvestDropsASunOncePerHundredKills) {
    expanded::SeedModRandom(7);
    for (int sword : {ITEM_WEAPON_SUN_SWORD, ITEM_WEAPON_BIG_BANG, ITEM_WEAPON_SWORD_OF_ZEUS}) {
        expanded::test::Wielding wielding(sword);
        const int                suns = Harvested(20000);
        EXPECT_GT(suns, 140) << sword;
        EXPECT_LT(suns, 260) << sword;
    }
}

TEST(SunSword, SolarHarvestNeedsOneOfTheSwords) {
    expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
    EXPECT_EQ(Harvested(5000), 0);
}

TEST(SunSword, SolarHarvestWorksThroughSuperSteve) {
    expanded::SeedModRandom(11);
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BIG_BANG);
    EXPECT_GT(Harvested(20000), 140);
}

TEST(SunSword, SolarHarvestLeavesASetDropAlone) {
    expanded::SeedModRandom(3);
    expanded::test::Wielding sun(ITEM_WEAPON_SUN_SWORD);
    EXPECT_EQ(Harvested(5000, ITEM_ATTACH_FIRE), 0);
}
