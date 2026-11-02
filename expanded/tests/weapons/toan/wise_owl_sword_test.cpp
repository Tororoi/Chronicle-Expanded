#include <gtest/gtest.h>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/door_keys.hpp"
#include "expanded/weapons/toan/wise_owl_sword.hpp"
#include "itemdata.hpp"
#include "runscript.hpp"
#include "sysmes.hpp"
#include "test_support.hpp"

using expanded::Monster;
using expanded::test::Wielding;

namespace {

// The window the locked room's gate asks for in Wise Owl Forest: the Moustache Key alone.
const int kMoustacheKeyWindow[] = {ITEM_MOUSTACHE_KEY, -1};

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

TEST(WiseOwlSword, InHandItOpensTheMoustacheKeyDoor) {
    Wielding     wielding(ITEM_WEAPON_WISE_OWL_SWORD);
    RS_STACKDATA result = {};
    BtEventInfo.item_select_result = 1;
    EXPECT_TRUE(expanded::DoorKeyAnswers(kMoustacheKeyWindow, &result));
    EXPECT_EQ(result.i, ITEM_MOUSTACHE_KEY);
    EXPECT_EQ(BtEventInfo.item_select_result, 0);

    const int    bone_key[] = {ITEM_BONE_KEY, -1}; // the Bone Rapier's door, not the owl's
    RS_STACKDATA other = {};
    EXPECT_FALSE(expanded::DoorKeyAnswers(bone_key, &other));
    EXPECT_EQ(other.i, 0);
}

TEST(WiseOwlSword, OwnedOrAnotherWeaponTheDoorAsksForTheKey) {
    Wielding     dagger(ITEM_WEAPON_DAGGER);
    RS_STACKDATA result = {};
    EXPECT_FALSE(expanded::WiseOwlsKeyActive());
    EXPECT_FALSE(expanded::DoorKeyAnswers(kMoustacheKeyWindow, &result));
    EXPECT_EQ(result.i, 0);
}

TEST(WiseOwlSword, SuperSteveCarryingItsSphereHoldsTheKey) {
    {
        Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_WISE_OWL_SWORD);
        EXPECT_TRUE(expanded::WiseOwlsKeyActive());
    }
    Wielding plain(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
    EXPECT_FALSE(expanded::WiseOwlsKeyActive());
}
