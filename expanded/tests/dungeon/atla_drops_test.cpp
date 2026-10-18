#include <gtest/gtest.h>

#include <cmath>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "itemdata.hpp"
#include "savedata.hpp"
#include "userstatus.hpp"

namespace {

alignas(64) CSaveData save;
alignas(64) CDungeonMap map;
int model;

class AtlaDrops : public ::testing::Test {
protected:
    void SetUp() override {
        saved_status = UserStatus;
        saved_map = NowDngMap;
        save = {};
        UserStatus = save.GetDngStatus();
        map.atra_num = 0;
        for (ATRA_BOLL &atla : map.atra) {
            atla = {};
        }
        for (DUNGEON_EVENT &event : map.events) {
            event = {};
            event.kind = DNG_EVENT_NONE;
        }
        map.atra_model = reinterpret_cast<CFrame *>(&model); // there, never drawn here
        NowDngMap = &map;
        expanded::AtlaDropsFloorLoaded();
    }

    void TearDown() override {
        expanded::AtlaDropsFloorLoaded();
        UserStatus = saved_status;
        NowDngMap = saved_map;
    }

    const DUNGEON_EVENT *Event(int atla) const {
        for (const DUNGEON_EVENT &event : map.events) {
            if (event.kind == DNG_EVENT_ATRA && event.index == atla) {
                return &event;
            }
        }
        return nullptr;
    }

    CUserStatus *saved_status = nullptr;
    CDungeonMap *saved_map = nullptr;
};

const ATTACH_LIST *Sphere() {
    CDngStatusData *status = static_cast<CDngStatusData *>(UserStatus);
    for (DNG_CONSUMABLE &item : status->consumable_items) {
        if (item.id == ITEM_ATTACH_SYNTHESIS_SPHERE) {
            return reinterpret_cast<const ATTACH_LIST *>(&item);
        }
    }
    return nullptr;
}

} // namespace

TEST_F(AtlaDrops, ANewAtlaCarriesItsDropAndNoPart) {
    float at[4] = {10.0f, 0.0f, 20.0f, 1.0f};
    EXPECT_EQ(expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_DAGGER, true}), 0);
    EXPECT_EQ(map.atra_num, 1);
    EXPECT_EQ(map.atra[0].used, 1);
    EXPECT_EQ(map.atra[0].atra_no, -1);
    const DUNGEON_EVENT *event = Event(0);
    ASSERT_NE(event, nullptr);
    EXPECT_FLOAT_EQ(event->radius, 13.0f);
    EXPECT_FLOAT_EQ(event->pos[2], 20.0f);
}

TEST_F(AtlaDrops, AFullFloorReusesACollectedPlace) {
    map.atra_num = 8;
    for (int i = 0; i < 8; i++) {
        map.atra[i].used = i != 3;
    }
    float at[4] = {1.0f, 0.0f, 2.0f, 1.0f};
    EXPECT_EQ(expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_DAGGER, true}), 3);
    EXPECT_EQ(map.atra[3].used, 1);
    EXPECT_EQ(map.atra[3].atra_no, -1);
    EXPECT_NE(Event(3), nullptr);
}

TEST_F(AtlaDrops, WithEveryPlaceHoldingOneTheFarthestComesInstead) {
    map.atra_num = 8;
    for (int i = 0; i < 8; i++) {
        map.atra[i].used = 1;
        map.atra[i].atra_no = i;
        map.atra[i].pos[0] = CharaMain.pos[0] + (i == 5 ? 500.0f : 10.0f * i);
        map.atra[i].pos[2] = CharaMain.pos[2];
    }
    float at[4] = {1.0f, 0.0f, 2.0f, 1.0f};
    EXPECT_EQ(expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_DAGGER, true}), -1);
    EXPECT_EQ(expanded::TakeOverAtla(at), 5);
    EXPECT_FLOAT_EQ(map.atra[5].pos[0], 1.0f);
    EXPECT_EQ(map.atra[5].atra_no, 5); // its own content
    EXPECT_FALSE(expanded::AtlaDropTaken(5));
}

TEST_F(AtlaDrops, NoAtlaWithoutAModel) {
    map.atra_model = nullptr;
    float at[4] = {};
    EXPECT_EQ(expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_DAGGER, true}), -1);
}

TEST_F(AtlaDrops, TakingItHandsOverASixthOfTheWeaponAsASphere) {
    float at[4] = {};
    int   atla = expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_SUN_SWORD, true});
    EXPECT_TRUE(expanded::AtlaDropTaken(atla));
    const ATTACH_LIST *sphere = Sphere();
    ASSERT_NE(sphere, nullptr);
    const WEAPON_DATA *data = GetWeaponData(ITEM_WEAPON_SUN_SWORD);
    EXPECT_EQ(sphere->sphere_weapon_no, ITEM_WEAPON_SUN_SWORD);
    EXPECT_EQ(sphere->status[0], static_cast<s16>(std::ceil(data->attack / 6.0)));
    EXPECT_EQ(sphere->status[3], static_cast<s16>(std::ceil(data->magic / 6.0)));
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(sphere->elem[i], static_cast<s8>(std::ceil(data->elem[i] / 6.0))) << i;
    }
    EXPECT_FALSE(expanded::AtlaDropTaken(atla)); // once
}

TEST_F(AtlaDrops, AWeaponGoesToItsOwner) {
    float at[4] = {};
    int   atla = expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_ATLAMILLIA_SWORD, false});
    EXPECT_TRUE(expanded::AtlaDropTaken(atla));
    bool held = false;
    for (const WEAPON_HAVE &weapon : UserStatus->chara_weapons[0]) {
        held |= weapon.item_no == ITEM_WEAPON_ATLAMILLIA_SWORD;
    }
    EXPECT_TRUE(held);
    EXPECT_EQ(Sphere(), nullptr);
}

TEST_F(AtlaDrops, ARetailAtlaIsLeftToTheGame) {
    float at[4] = {};
    map.SetAtraBoll(at, 4);
    EXPECT_FALSE(expanded::AtlaDropTaken(0));
}

TEST_F(AtlaDrops, AMovingAtlaCannotBePickedUpUntilItSettles) {
    float              at[4] = {};
    int                atla = expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_DAGGER, true});
    expanded::AtlaPath path = {
        {1, 0, 0, 0},
        {0, 0, 0, 1},
        0.5f,
        {0, 4, 0},
        {5, 0, 6}
    };
    expanded::MoveAtla(atla, path, -1.0f);
    EXPECT_FLOAT_EQ(Event(atla)->radius, 0.0f);
    EXPECT_FLOAT_EQ(map.atra[atla].pos[0], 5.0f);
    EXPECT_TRUE(expanded::AtlaDrawn(map, atla)); // out of the draw
    expanded::MoveAtla(atla, path, 1.0f);
    EXPECT_FLOAT_EQ(Event(atla)->radius, 13.0f);
    EXPECT_FALSE(expanded::AtlaDrawn(map, atla));
}

TEST_F(AtlaDrops, ANewFloorForgetsThem) {
    float at[4] = {};
    int   atla = expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_DAGGER, true});
    expanded::AtlaDropsFloorLoaded();
    EXPECT_FALSE(expanded::AtlaDropTaken(atla));
}

TEST_F(AtlaDrops, OneDrawnOverTheShadowsLeavesTheShadowPassDraw) {
    float at[4] = {};
    int   atla = expanded::PlaceAtlaDrop(at, {ITEM_WEAPON_DAGGER, true});
    EXPECT_FALSE(expanded::AtlaDrawn(map, atla));
    expanded::DrawAtlaOverShadows(atla, true);
    EXPECT_TRUE(expanded::AtlaDrawn(map, atla)); // drawn after the shadow pass instead
    expanded::DrawAtlaOverShadows(atla, false);
    EXPECT_FALSE(expanded::AtlaDrawn(map, atla));
}
