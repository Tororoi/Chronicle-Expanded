#include <gtest/gtest.h>

#include <memory>

#include "dungeonmap.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/antique_sword.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

TEST(AntiqueSword, CostsTwelveThousandAndSellsForThree) {
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_ANTIQUE_SWORD, 1, 200), 3000);
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_ANTIQUE_SWORD, 0, 800), 12000);
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_DAGGER, 1, 50), 50);
}

TEST(AntiqueSword, ItsSynthSphereSellsForThreeThousand) {
    EXPECT_EQ(expanded::SphereSellPrice(ITEM_ATTACH_SYNTHESIS_SPHERE, ITEM_WEAPON_ANTIQUE_SWORD, 1), 3000);
    EXPECT_EQ(expanded::SphereSellPrice(ITEM_ATTACH_SYNTHESIS_SPHERE, ITEM_WEAPON_DAGGER, 1), 1);
    EXPECT_EQ(expanded::SphereSellPrice(ITEM_ATTACH_FIRE, ITEM_WEAPON_ANTIQUE_SWORD, 25), 25); // not a sphere
}

// A real chest in slot 0, a mimic's of either size in slots 1 and 2.
TEST(AntiqueSword, TellsRealChestsFromMimics) {
    auto map = std::make_unique<CDungeonMap>();
    map->boxes[0] = {.used = 1, .closed = 1, .kind = TREASURE_BOX_SMALL};
    for (int box : {1, 2}) {
        map->boxes[box] = {.used = 1, .closed = 1, .kind = box == 1 ? TREASURE_BOX_SMALL : TREASURE_BOX_LARGE};
        map->events[box].kind = DNG_EVENT_MIMIC;
        map->events[box].index = box;
    }
    map->events[0].kind = DNG_EVENT_TREASURE_BOX;
    map->events[0].index = 0;
    EXPECT_TRUE(expanded::RealChest(*map, 0));
    EXPECT_FALSE(expanded::RealChest(*map, 1));
    EXPECT_FALSE(expanded::RealChest(*map, 2));
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
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_ANTIQUE_SWORD, 1, 200), 200);
    EXPECT_EQ(expanded::AntiqueSwordPrice(ITEM_WEAPON_ANTIQUE_SWORD, 0, 800), 800);
    EXPECT_EQ(expanded::SphereSellPrice(ITEM_ATTACH_SYNTHESIS_SPHERE, ITEM_WEAPON_ANTIQUE_SWORD, 1), 1);
    EXPECT_FALSE(expanded::TreasureSenseActive());
}
