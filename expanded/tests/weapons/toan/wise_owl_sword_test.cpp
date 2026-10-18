#include <gtest/gtest.h>

#include "expanded/weapons/toan/wise_owl_sword.hpp"
#include "itemdata.hpp"
#include "sysmes.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// A floor of three monsters at 600, 300 and 100 from the player, none carrying a key.
struct Floor {
    expanded::test::TestFloor floor;

    Floor() {
        floor.SetUpModel(0, Monster::CANNIBAL_PLANT);
        float distances[] = {600.0f, 300.0f, 100.0f};
        for (int monster = 0; monster < 3; monster++) {
            floor.Spawn(monster, 0);
            floor.unit().monster[monster].player_distance = distances[monster];
            floor.unit().monster[monster].drop_item = -1;
        }
    }

    MONSTOR &Monster(int monster) { return floor.unit().monster[monster]; }
};

} // namespace

TEST(WiseOwlSword, FindsTheNearestKeyHolderInRange) {
    Floor floor;
    EXPECT_EQ(expanded::WiseOwlKeyHolder(floor.floor.unit()), -1);

    floor.Monster(0).drop_item = ITEM_SHINY_STONE; // out of range
    EXPECT_EQ(expanded::WiseOwlKeyHolder(floor.floor.unit()), -1);
    floor.Monster(1).drop_item = ITEM_RED_BERRY;
    EXPECT_EQ(expanded::WiseOwlKeyHolder(floor.floor.unit()), 1);
    floor.Monster(2).drop_item = ITEM_POINTY_CHESTNUT;
    EXPECT_EQ(expanded::WiseOwlKeyHolder(floor.floor.unit()), 2);

    // Not a key, beaten, or gone.
    floor.Monster(2).drop_item = ITEM_ATTACH_FIRE;
    EXPECT_EQ(expanded::WiseOwlKeyHolder(floor.floor.unit()), 1);
    floor.Monster(1).hp = 0;
    EXPECT_EQ(expanded::WiseOwlKeyHolder(floor.floor.unit()), -1);
    floor.Monster(1).hp = 10;
    floor.Monster(1).state = -1;
    EXPECT_EQ(expanded::WiseOwlKeyHolder(floor.floor.unit()), -1);
}

TEST(WiseOwlSword, NamesTheKeyOnceAndOnlyWhileTheSwordIsOwned) {
    Floor                    floor;
    expanded::test::TestSave save;
    floor.Monster(1).drop_item = ITEM_RED_BERRY;
    ClearSystemMes();

    expanded::WiseOwlStep(floor.floor.unit());
    EXPECT_FALSE(SystemMesCheck()); // no sword

    save.status().chara_weapons[0][0].item_no = ITEM_WEAPON_WISE_OWL_SWORD;
    expanded::WiseOwlStep(floor.floor.unit());
    EXPECT_TRUE(SystemMesCheck());

    ClearSystemMes();
    expanded::WiseOwlStep(floor.floor.unit()); // the same holder: once
    EXPECT_FALSE(SystemMesCheck());

    floor.Monster(1).player_distance = 700.0f; // out of range, then back
    expanded::WiseOwlStep(floor.floor.unit());
    floor.Monster(1).player_distance = 200.0f;
    expanded::WiseOwlStep(floor.floor.unit());
    EXPECT_TRUE(SystemMesCheck());
    ClearSystemMes();
}
