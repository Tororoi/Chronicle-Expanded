#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "itemdata.hpp"
#include "main.hpp"
#include "shot_effect_pack.hpp"
#include "userstatus.hpp"

extern s32 mode;

namespace {

int g_loads = 0;

bool FakeLoad(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *) {
    g_loads++;
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    return true;
}

BT_SHOT_EFFECT Described(const char *name) {
    BT_SHOT_EFFECT effect = {};
    std::snprintf(effect.model_name, sizeof(effect.model_name), "%s", name);
    return effect;
}

// A dungeon floor with an empty pack, a party carrying nothing and the disc load faked.
class ShotEffects : public ::testing::Test {
protected:
    void SetUp() override {
        saved_pack_ = NowShotEffect;
        saved_status_ = UserStatus;
        saved_mode_ = mode;
        pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
        for (CSHOT_EFFECT &effect : pack_->effect) {
            effect.Initialize();
        }
        status_ = std::make_unique<CDngStatusData>();
        for (auto &chara : status_->chara_weapons) {
            for (WEAPON_HAVE &weapon : chara) {
                weapon.item_no = 0;
            }
        }
        NowShotEffect = pack_.get();
        UserStatus = status_.get();
        mode = GAME_MODE_DUNGEON;
        expanded::SetShotEffectLoader(FakeLoad);
        expanded::ShotEffectsClear();
        g_loads = 0;
    }

    void TearDown() override {
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        NowShotEffect = saved_pack_;
        UserStatus = saved_status_;
        mode = saved_mode_;
    }

    std::unique_ptr<CSHOT_EFFECT_PACK> pack_;
    std::unique_ptr<CDngStatusData>    status_;
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    CUserStatus                       *saved_status_ = nullptr;
    s32                                saved_mode_ = 0;
};

TEST_F(ShotEffects, LoadsACustomDescriptionPastRetailsFive) {
    BT_SHOT_EFFECT custom = Described("custom");

    int index = expanded::EnterShotEffect(&custom, 3);

    EXPECT_EQ(index, expanded::kRetailShotEffects);
    EXPECT_EQ(expanded::ShotEffectAt(index)->effect_data, &custom);
    EXPECT_EQ(expanded::ShotEffectAt(index)->slot_count, 3);
    EXPECT_EQ(expanded::ShotEffectIndex(&custom), index);
    EXPECT_EQ(expanded::ShotEffectCount(), expanded::kRetailShotEffects + 1);
}

// Only an index in use has an effect: not -1, not one past the loaded ones, not retail's while no pack is up.
TEST_F(ShotEffects, OnlyAnIndexInUseIsLoaded) {
    BT_SHOT_EFFECT custom = Described("custom");
    int            index = expanded::EnterShotEffect(&custom, 3);

    EXPECT_EQ(expanded::LoadedShotEffect(index), expanded::ShotEffectAt(index));
    EXPECT_EQ(expanded::LoadedShotEffect(0), &pack_->effect[0]);
    EXPECT_EQ(expanded::LoadedShotEffect(-1), nullptr);
    EXPECT_EQ(expanded::LoadedShotEffect(index + 1), nullptr);
    NowShotEffect = nullptr;
    EXPECT_EQ(expanded::LoadedShotEffect(0), nullptr);
    EXPECT_EQ(expanded::LoadedShotEffect(index), expanded::ShotEffectAt(index));
}

TEST_F(ShotEffects, ALoadedDescriptionKeepsItsSlot) {
    BT_SHOT_EFFECT custom = Described("custom");
    BT_SHOT_EFFECT retail = Described("retail");
    pack_->effect[2].effect_data = &retail;

    int first = expanded::EnterShotEffect(&custom, 2);
    int again = expanded::EnterShotEffect(&custom, 6);

    EXPECT_EQ(first, again);
    EXPECT_EQ(expanded::EnterShotEffect(&retail, 2), 2);
    EXPECT_EQ(pack_->Entry(&custom, nullptr, 0x26, nullptr, 2), first);
    EXPECT_EQ(g_loads, 1);
}

TEST_F(ShotEffects, ASixthConfigGetsASlotFromThePack) {
    std::vector<BT_SHOT_EFFECT> configs(6);
    for (int i = 0; i < 5; i++) {
        pack_->effect[i].effect_data = &configs[i];
    }

    int index = pack_->Entry(&configs[5], nullptr, 0x26, nullptr, 2);

    EXPECT_EQ(index, expanded::kRetailShotEffects);
    EXPECT_EQ(expanded::ShotEffectAt(index)->effect_data, &configs[5]);
}

TEST_F(ShotEffects, GrowsPastTheTableWithoutMovingAnEffect) {
    std::vector<BT_SHOT_EFFECT> configs(60);
    expanded::EnterShotEffect(&configs[0], 2);
    CSHOT_EFFECT *first = expanded::ShotEffectAt(expanded::kRetailShotEffects);

    for (int i = 1; i < 60; i++) {
        EXPECT_EQ(expanded::EnterShotEffect(&configs[i], 2), expanded::kRetailShotEffects + i);
    }

    EXPECT_EQ(expanded::ShotEffectAt(expanded::kRetailShotEffects), first);
    EXPECT_EQ(first->effect_data, &configs[0]);
    EXPECT_EQ(expanded::ShotEffectCount(), expanded::kRetailShotEffects + 60);

    expanded::ShotEffectsClear();
    EXPECT_EQ(expanded::ShotEffectCount(), expanded::kRetailShotEffects);
    EXPECT_EQ(expanded::ShotEffectIndex(&configs[0]), -1);
    expanded::EnterShotEffect(&configs[1], 2);
    EXPECT_EQ(expanded::ShotEffectAt(expanded::kRetailShotEffects), first);
}

TEST_F(ShotEffects, AFloorLoadsTheEffectsOfCarriedWeaponsAndSuperStevesSphere) {
    static BT_SHOT_EFFECT carried = Described("carried");
    static BT_SHOT_EFFECT left = Described("left");
    static BT_SHOT_EFFECT sphere = Described("sphere");
    expanded::RegisterWeaponShotEffect(ITEM_WEAPON_PARTISAN, expanded::WeaponShotForm::Weapon, &carried, 2);
    expanded::RegisterWeaponShotEffect(ITEM_WEAPON_CHRONICLE_SWORD, expanded::WeaponShotForm::Weapon, &left, 2);
    expanded::RegisterWeaponShotEffect(ITEM_WEAPON_CHRONICLE_SWORD, expanded::WeaponShotForm::Sphere, &sphere, 2);
    status_->chara_weapons[3][4].item_no = ITEM_WEAPON_PARTISAN;
    WEAPON_HAVE &steve = status_->chara_weapons[1][7];
    steve.item_no = ITEM_WEAPON_SUPER_STEVE;
    steve.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    steve.attach[0].sphere_weapon_no = ITEM_WEAPON_CHRONICLE_SWORD;

    expanded::ShotEffectsWeaponsChanged();
    EXPECT_EQ(g_loads, 0);

    expanded::ShotEffectsFloorLoaded();

    EXPECT_GE(expanded::ShotEffectIndex(&carried), expanded::kRetailShotEffects);
    EXPECT_GE(expanded::ShotEffectIndex(&sphere), expanded::kRetailShotEffects);
    EXPECT_EQ(expanded::ShotEffectIndex(&left), -1);
    EXPECT_EQ(g_loads, 2);
}

TEST_F(ShotEffects, AWeaponPickedUpOrBuiltUpMidFloorIsLoaded) {
    static BT_SHOT_EFFECT picked = Described("picked");
    static BT_SHOT_EFFECT built = Described("built");
    expanded::RegisterWeaponShotEffect(ITEM_WEAPON_DRAGON_S_Y, expanded::WeaponShotForm::Weapon, &picked, 2);
    expanded::RegisterWeaponShotEffect(ITEM_WEAPON_ANGE_GEAR, expanded::WeaponShotForm::Weapon, &built, 2);
    expanded::ShotEffectsFloorLoaded();
    EXPECT_EQ(g_loads, 0);

    status_->GetItem(ITEM_WEAPON_DRAGON_S_Y, 0);
    EXPECT_GE(expanded::ShotEffectIndex(&picked), expanded::kRetailShotEffects);

    status_->chara_weapons[4][0].item_no = ITEM_WEAPON_ANGE_GEAR;
    expanded::ShotEffectsWeaponsChanged();
    EXPECT_GE(expanded::ShotEffectIndex(&built), expanded::kRetailShotEffects);
    EXPECT_EQ(g_loads, 2);
}

TEST_F(ShotEffects, NothingLoadsOutsideAFloor) {
    static BT_SHOT_EFFECT town = Described("town");
    expanded::RegisterWeaponShotEffect(ITEM_WEAPON_SAX, expanded::WeaponShotForm::Weapon, &town, 2);
    expanded::ShotEffectsFloorLoaded();
    mode = GAME_MODE_EDIT;

    status_->GetItem(ITEM_WEAPON_SAX, 0);

    EXPECT_EQ(expanded::ShotEffectIndex(&town), -1);
}

} // namespace
