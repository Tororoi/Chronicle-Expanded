#include <gtest/gtest.h>

#include <memory>

#include "dungeonmap.hpp"
#include "expanded/weapons/toan/sax.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::test::Wielding;

namespace {

// A floor whose box 0 holds item_no.
struct Chests {
    std::unique_ptr<CDungeonMap> map = std::make_unique<CDungeonMap>();

    explicit Chests(int item_no) { map->boxes[0].item_no = item_no; }

    int Opened(int box_no = 0) const { return expanded::FineFareItem(*map, box_no); }
};

} // namespace

TEST(Sax, FareUpgrades) {
    Wielding wielding(ITEM_WEAPON_SAX);
    EXPECT_EQ(Chests(ITEM_REGULAR_WATER).Opened(), ITEM_PREMIUM_WATER);
    EXPECT_EQ(Chests(ITEM_TASTY_WATER).Opened(), ITEM_PREMIUM_WATER);
    EXPECT_EQ(Chests(ITEM_BREAD).Opened(), ITEM_PREMIUM_CHICKEN);
    EXPECT_EQ(Chests(ITEM_CHEESE).Opened(), ITEM_PREMIUM_CHICKEN);
    EXPECT_EQ(Chests(ITEM_TREASURE_KEY).Opened(), ITEM_TREASURE_KEY);
    EXPECT_EQ(Chests(ITEM_ANTIDOTE_DRINK).Opened(), ITEM_ANTIDOTE_DRINK);
}

TEST(Sax, HeirsAddTheirTier) {
    {
        Wielding wielding(ITEM_WEAPON_DUSACK);
        EXPECT_EQ(Chests(ITEM_BREAD).Opened(), ITEM_PREMIUM_CHICKEN);
        EXPECT_EQ(Chests(ITEM_TREASURE_KEY).Opened(), ITEM_GOLD_BULLION);
        EXPECT_EQ(expanded::FineFareTier(), 1);
    }

    for (int weapon : {ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_ATLAMILLIA_SWORD, ITEM_WEAPON_CHRONICLE_SWORD}) {
        Wielding wielding(weapon);
        EXPECT_EQ(Chests(ITEM_TREASURE_KEY).Opened(), ITEM_GOLD_BULLION);
        EXPECT_EQ(expanded::FineFareTier(), 2);
    }
}

TEST(Sax, RepairPowderRollsOncePerChest) {
    Wielding     wielding(ITEM_WEAPON_7BRANCH_SWORD);
    auto         owned = std::make_unique<CDungeonMap>();
    CDungeonMap &map = *owned;
    int          upgraded = 0;
    for (int box = 0; box < 24; box++) {
        map.boxes[box].item_no = ITEM_REPAIR_POWDER;
    }

    for (int seed = 0; seed < 50; seed++) {
        map.map_seed = seed * 7919;
        for (int box = 0; box < 24; box++) {
            int item_no = expanded::FineFareItem(map, box);
            ASSERT_TRUE(item_no == ITEM_REPAIR_POWDER || item_no == ITEM_AUTO_REPAIR_POWDER);
            EXPECT_EQ(expanded::FineFareItem(map, box), item_no);
            upgraded += item_no == ITEM_AUTO_REPAIR_POWDER;
        }
    }

    // About one in four of 1200.
    EXPECT_GT(upgraded, 200);
    EXPECT_LT(upgraded, 400);

    // Below the 7 Branch Sword's tier it never upgrades.
    Wielding dusack(ITEM_WEAPON_DUSACK);
    for (int box = 0; box < 24; box++) {
        EXPECT_EQ(expanded::FineFareItem(map, box), ITEM_REPAIR_POWDER);
    }
}

TEST(Sax, SphereGivesItsSwordsTier) {
    {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DUSACK);
        EXPECT_EQ(Chests(ITEM_TREASURE_KEY).Opened(), ITEM_GOLD_BULLION);
    }
    {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SAX);
        EXPECT_EQ(Chests(ITEM_REGULAR_WATER).Opened(), ITEM_PREMIUM_WATER);
        EXPECT_EQ(Chests(ITEM_TREASURE_KEY).Opened(), ITEM_TREASURE_KEY);
    }

    Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
    EXPECT_EQ(Chests(ITEM_REGULAR_WATER).Opened(), ITEM_REGULAR_WATER);
    EXPECT_EQ(expanded::FineFareTier(), -1);
}
