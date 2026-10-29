#include <gtest/gtest.h>

#include <cstring>
#include <memory>

#include "clsmes.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/weapons/xiao/bandit_slingshot.hpp"
#include "itemdata.hpp"
#include "main.hpp"
#include "menu_misc.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "test_support.hpp"
#include "text/gametext.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern s32 mode;
} // namespace dc::game

namespace {

using expanded::Monster;
using expanded::No;
using expanded::Species;
using expanded::test::TestFloor;
using expanded::test::Wielding;

bool FakeLoad(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    return true;
}

// A dungeon floor with Xiao holding weapon_no (carrying sphere when given), an empty shot pack with the disc load
// faked and the monsters' names in their bank.
class StealShot : public ::testing::Test {
protected:
    explicit StealShot(int weapon_no = ITEM_WEAPON_BANDIT_SLINGSHOT, int sphere = 0) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        saved_pack_ = NowShotEffect;
        saved_mode_ = mode;
        saved_names_ = MonsterNameMessages();
        for (CSHOT_EFFECT &effect : pack_->effect) {
            effect.Initialize();
        }
        NowShotEffect = pack_.get();
        mode = GAME_MODE_DUNGEON;
        expanded::SetShotEffectLoader(FakeLoad);
        expanded::ShotEffectsClear();
        floor_.status().cur_chara = CHARA_XIAO;
        NewFloor();
        names_.Set(Species(No(Monster::MR_BLARE))->name_no + 3000, "Mr. Blare");
        names_.Set(Species(No(Monster::HALLOWEEN))->name_no + 3000, "Halloween");
        mes_.SetBuff(names_.Data());
        MonsterNameMessages() = &mes_;
    }

    void TearDown() override {
        NewFloor();
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        NowShotEffect = saved_pack_;
        mode = saved_mode_;
        MonsterNameMessages() = saved_names_;
    }

    // The floor's kinds starting to be set up.
    void NewFloor() { floor_.SetUpModel(0, Monster::MASTER_JACKET); }

    // monster a kind, its steal landing.
    void Steal(Monster kind, int monster = 0, int item = ITEM_BOMB) {
        floor_.SetUpModel(monster, kind);
        floor_.Spawn(monster, monster);
        expanded::StealShotTaken(floor_.unit(), monster, item);
    }

    TestFloor                          floor_;
    Wielding                           wielding_;
    std::unique_ptr<CSHOT_EFFECT_PACK> pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
    GameTextFile                       names_;
    ClsMes                             mes_;
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    s32                                saved_mode_ = 0;
    ClsMes                            *saved_names_ = nullptr;
};

class StealShotWithAnotherWeapon : public StealShot {
protected:
    StealShotWithAnotherWeapon() : StealShot(ITEM_WEAPON_WOODEN_SLINGSHOT) {}
};

class StealShotBySuperSteve : public StealShot {
protected:
    StealShotBySuperSteve() : StealShot(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BANDIT_SLINGSHOT) {}
};

class StealShotByTheBanditsRingsSphere : public StealShot {
protected:
    StealShotByTheBanditsRingsSphere() : StealShot(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BANDIT_S_RING) {}
};

} // namespace

TEST_F(StealShot, TakesTheKindsShotTurnedOnTheMonsters) {
    Steal(Monster::HALLOWEEN);

    const BT_SHOT_EFFECT *held = expanded::StealShotHeld();
    ASSERT_NE(held, nullptr);
    EXPECT_STREQ(held->model_name, "pump_bom");
    EXPECT_EQ(held->target, 2);
    EXPECT_GE(expanded::ShotEffectIndex(held), expanded::kRetailShotEffects);
    EXPECT_EQ(expanded::ShotEffectAt(expanded::ShotEffectIndex(held))->slot_count, expanded::kStealShots);
}

TEST_F(StealShot, PassesASelfBurstForTheSecondShot) {
    Steal(Monster::MR_BLARE);
    ASSERT_NE(expanded::StealShotHeld(), nullptr);
    EXPECT_STREQ(expanded::StealShotHeld()->model_name, "f_boll_3");
}

TEST_F(StealShot, AKindWithOnlyASelfBurstGivesTheItemAlone) {
    Steal(Monster::BOMBER_HEAD);
    EXPECT_EQ(expanded::StealShotHeld(), nullptr);
}

TEST_F(StealShot, IsGoneWithTheFloor) {
    Steal(Monster::HALLOWEEN);
    NewFloor();
    EXPECT_EQ(expanded::StealShotHeld(), nullptr);
}

TEST_F(StealShotWithAnotherWeapon, TakesNothing) {
    Steal(Monster::HALLOWEEN);
    EXPECT_FALSE(expanded::StealShotActive());
    EXPECT_EQ(expanded::StealShotHeld(), nullptr);
}

TEST_F(StealShotBySuperSteve, TakesWithTheSlingshotsSphere) {
    Steal(Monster::HALLOWEEN);
    EXPECT_NE(expanded::StealShotHeld(), nullptr);
}

TEST_F(StealShotByTheBanditsRingsSphere, TakesNothing) {
    Steal(Monster::HALLOWEEN);
    EXPECT_FALSE(expanded::StealShotActive());
    EXPECT_EQ(expanded::StealShotHeld(), nullptr);
}

TEST_F(StealShot, APelletKeepsFlyingWithNothingHeld) {
    CSHOT shots;
    shots.used[3] = 1;
    expanded::StealShotFired(shots, 3);
    EXPECT_EQ(shots.used[3], 1);
}

TEST_F(StealShot, TheStolenItemsNoticeNamesTheMonster) {
    Steal(Monster::MR_BLARE, 0, ITEM_FIRE_GEM);

    EXPECT_EQ(expanded::StealShotNoticeText(30), std::nullopt);
    expanded::StealShotNotice(true, ITEM_FIRE_GEM);
    EXPECT_EQ(expanded::StealShotNoticeText(30), "{icon-jar}\"{insert1}\" acquired.\nMr. Blare's projectile is now yours");
    EXPECT_EQ(expanded::StealShotNoticeText(10), "{icon-sword}\"{insert1}{value1}\" acquired.\nMr. Blare's projectile is now yours");
    expanded::StealShotNotice(false, ITEM_FIRE_GEM);
    EXPECT_EQ(expanded::StealShotNoticeText(30), std::nullopt);

    // Only once, and not for another item.
    expanded::StealShotNotice(true, ITEM_FIRE_GEM);
    EXPECT_EQ(expanded::StealShotNoticeText(30), std::nullopt);
}
