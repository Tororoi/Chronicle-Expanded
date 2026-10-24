#include <gtest/gtest.h>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "dungeonparts.hpp"
#include "expanded/core/random.hpp"
#include "expanded/core/save.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/weapons/toan/atlamillia_sword.hpp"
#include "itemdata.hpp"
#include "savedata.hpp"
#include "userstatus.hpp"

namespace {

alignas(64) CSaveData save;
alignas(64) CDungeonMap map;
int model;

// Divine Beast Cave's floors, the save holding an Atlamillia Sword or not, and a floor map with room for an atla.
class Atlamillia : public ::testing::Test {
protected:
    void SetUp() override {
        saved = {UserStatus, SaveData, NowDngMap, selectMapNo};
        save = {};
        SaveData = &save;
        UserStatus = save.GetDngStatus();
        selectMapNo = DUNGEON_DIVINE_BEAST_CAVE;
        map.atra_num = 0;
        for (ATRA_BOLL &atla : map.atra) {
            atla = {};
        }
        for (DUNGEON_EVENT &event : map.events) {
            event = {};
            event.kind = DNG_EVENT_NONE;
        }
        for (MAPPARTS &cell : map.cells) {
            cell = {};
            cell.parts_no = ItemFreeAreaAll[DUNGEON_DIVINE_BEAST_CAVE][0].parts_no;
        }
        map.atra_model = reinterpret_cast<CFrame *>(&model);
        expanded::SaveClear();
        expanded::SeedModRandom(7);
        expanded::AtlaDropsFloorLoaded();
    }

    void TearDown() override {
        expanded::SaveClear();
        expanded::AtlaDropsFloorLoaded();
        UserStatus = saved.status;
        SaveData = saved.data;
        NowDngMap = saved.map;
        selectMapNo = saved.dungeon;
    }

    void OwnAtlamillia() { UserStatus->chara_weapons[CHARA_TOAN][3].item_no = ITEM_WEAPON_ATLAMILLIA_SWORD; }

    // The floor the insured atla went to, loaded in turn until one is placed: its index there, or -1.
    int FindAtla() {
        NowDngMap = &MainDungeonMap;
        for (int floor = 0; floor < 15; floor++) {
            UserStatus->cur_floor = floor;
            MainDungeonMap = map;
            expanded::AtlaDropsFloorLoaded();
            expanded::AtlamilliaFloorLoaded();
            expanded::AtlamilliaStep();
            if (MainDungeonMap.atra_num > 0) {
                return 0;
            }
        }
        return -1;
    }

    const ATTACH_LIST *Sphere() {
        for (DNG_CONSUMABLE &item : static_cast<CDngStatusData *>(UserStatus)->consumable_items) {
            if (item.id == ITEM_ATTACH_SYNTHESIS_SPHERE) {
                return reinterpret_cast<const ATTACH_LIST *>(&item);
            }
        }
        return nullptr;
    }

    struct Saved {
        CUserStatus *status;
        CSaveData   *data;
        CDungeonMap *map;
        s32          dungeon;
    } saved{};
};

WEAPON_HAVE Broken(int level) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = ITEM_WEAPON_SUN_SWORD;
    weapon.level = static_cast<s16>(level);
    weapon.attack = 100;
    weapon.endurance = 50;
    weapon.speed = 40;
    weapon.magic = 20;
    weapon.elem[2] = 30;
    weapon.vs_monster[1] = 10;
    return weapon;
}

} // namespace

TEST_F(Atlamillia, TheShareIsATenthALevelUpToAHalf) {
    EXPECT_FLOAT_EQ(expanded::InsuranceShare(Broken(0)), 0.1f);
    EXPECT_FLOAT_EQ(expanded::InsuranceShare(Broken(3)), 0.4f);
    EXPECT_FLOAT_EQ(expanded::InsuranceShare(Broken(4)), 0.5f);
    EXPECT_FLOAT_EQ(expanded::InsuranceShare(Broken(12)), 0.5f);
}

TEST_F(Atlamillia, OnlyWhileTheSaveOwnsOne) {
    expanded::AtlamilliaWeaponBroken(Broken(2));
    EXPECT_EQ(expanded::InsuredSpheres(), 0);
    OwnAtlamillia();
    expanded::AtlamilliaWeaponBroken(Broken(2));
    EXPECT_EQ(expanded::InsuredSpheres(), 1);
}

TEST_F(Atlamillia, ABrokenWeaponWaitsAsASphereOnAFloorOfItsDungeon) {
    OwnAtlamillia();
    expanded::AtlamilliaWeaponBroken(Broken(2)); // +2: 30%
    ASSERT_EQ(FindAtla(), 0);
    EXPECT_TRUE(expanded::AtlaDropTaken(0));
    const ATTACH_LIST *sphere = Sphere();
    ASSERT_NE(sphere, nullptr);
    EXPECT_EQ(sphere->sphere_weapon_no, ITEM_WEAPON_SUN_SWORD);
    EXPECT_EQ(sphere->sphere_level, 2);
    EXPECT_EQ(sphere->status[0], 30);
    EXPECT_EQ(sphere->status[1], 15);
    EXPECT_EQ(sphere->status[2], 12);
    EXPECT_EQ(sphere->status[3], 6);
    EXPECT_EQ(sphere->elem[2], 9);
    EXPECT_EQ(sphere->vs_monster[1], 3);
    // Picked up: it waits no more.
    EXPECT_EQ(expanded::InsuredSpheres(), 0);
}

TEST_F(Atlamillia, LeftOnTheFloorItComesBack) {
    OwnAtlamillia();
    expanded::AtlamilliaWeaponBroken(Broken(4));
    ASSERT_EQ(FindAtla(), 0);
    const int floor = UserStatus->cur_floor;
    // The floor built again: placed again.
    MainDungeonMap = map;
    expanded::AtlaDropsFloorLoaded();
    expanded::AtlamilliaFloorLoaded();
    expanded::AtlamilliaStep();
    EXPECT_EQ(MainDungeonMap.atra_num, 1);
    EXPECT_EQ(UserStatus->cur_floor, floor);
    EXPECT_EQ(expanded::InsuredSpheres(), 1);
}
