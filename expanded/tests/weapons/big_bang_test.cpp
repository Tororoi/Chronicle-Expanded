#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/big_bang_shot.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/toan/big_bang.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"

// The lock-on target, as movechara.cpp keeps it.
extern s32 lockOnTargetNo;

using expanded::Monster;

namespace {

// A Master Jacket 100 units ahead, the hits recorded.
struct Floor {
    expanded::test::TestFloor       floor;
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved = NowColData;
    MOTION_INFO                     motions[12] = {};

    Floor() {
        NowColData = hits.get();
        floor.SetUpModel(0, Monster::MASTER_JACKET);
        floor.Spawn(0, 0);
        floor.unit().monster[0].revealed = -1;
        floor.unit().chara[0][0].SetPosition(0.0f, 0.0f, 100.0f);
        floor.unit().chara[0][0].body_height = 20.0f;
        floor.unit().chara[0][0].motion_type.motion_info = motions;
        BtActStatus = {};
        expanded::test::SolarReset();
        expanded::JudgementReset();
        expanded::BigBangShotReset();
    }

    ~Floor() {
        expanded::JudgementReset();
        expanded::BigBangShotReset();
        BtActStatus = {};
        NowColData = saved;
    }
};

} // namespace

TEST(JudgementBlade, HangsOverItsTargetThenFallsToItsFeet) {
    Floor floor;
    expanded::JudgementHover(floor.floor.unit(), 0);
    EXPECT_TRUE(expanded::JudgementHanging());
    for (int i = 0; i < expanded::kJudgementFadeFrames; i++) {
        expanded::JudgementStep();
    }
    EXPECT_FLOAT_EQ(expanded::JudgementShown(), 1.0f);
    float glow[4];
    ASSERT_TRUE(expanded::JudgementGlowPoint(glow));
    EXPECT_GT(glow[1], 20.0f); // over the Jacket's head
    EXPECT_TRUE(expanded::TargetNameHidden());

    ASSERT_TRUE(expanded::JudgementDrop());
    float landed[4];
    int   frames = 0;
    while (!expanded::JudgementLanded(landed) && frames++ < 120) {
        expanded::JudgementStep();
    }
    EXPECT_LT(frames, 60);
    EXPECT_FLOAT_EQ(landed[2], 100.0f);
    EXPECT_FLOAT_EQ(landed[1], 0.0f);
    EXPECT_FALSE(expanded::JudgementHanging());
}

TEST(JudgementBlade, FadesAwayWhenReleased) {
    Floor floor;
    expanded::JudgementHover(floor.floor.unit(), 0);
    expanded::JudgementStep();
    expanded::JudgementRelease();
    EXPECT_FALSE(expanded::JudgementDrop());
    for (int i = 0; i < expanded::kJudgementFadeFrames; i++) {
        expanded::JudgementStep();
    }
    EXPECT_FLOAT_EQ(expanded::JudgementShown(), 0.0f);
}

TEST(BigBang, TheWhirlwindIsTheBlastAndExplosionsPassItsWielderBy) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_TOAN;
    COLLISION_HIT bomb = {};
    bomb.kind = 3;
    bomb.ready_phase = 10;
    {
        expanded::test::Wielding sword(ITEM_WEAPON_SUN_SWORD);
        EXPECT_TRUE(expanded::WhirlSwordHits());
        EXPECT_FALSE(expanded::PlayerHitSkipped(bomb));
    }
    expanded::test::Wielding sword(ITEM_WEAPON_BIG_BANG);
    EXPECT_FALSE(expanded::WhirlSwordHits());
    EXPECT_TRUE(expanded::PlayerHitSkipped(bomb));
    COLLISION_HIT swing = {};
    swing.kind = 2;
    EXPECT_FALSE(expanded::PlayerHitSkipped(swing));
}

TEST(BigBang, ItsSolarFlashHangsTheBladeOverTheLockedTarget) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_TOAN;
    expanded::test::Wielding sword(ITEM_WEAPON_BIG_BANG);
    s32                      saved_flag = lockOnTargetFlag;
    lockOnTargetFlag = 1;
    s32 saved_no = lockOnTargetNo;
    lockOnTargetNo = 0;
    BtActStatus.guard_mode = 5;
    for (int i = 0; i <= expanded::kSolarChargeFrames + 1; i++) {
        expanded::test::SolarStep();
    }
    EXPECT_EQ(expanded::SolarFlashPhase(), expanded::SolarPhase::kPrimed);
    EXPECT_TRUE(expanded::JudgementHanging());
    lockOnTargetFlag = saved_flag;
    lockOnTargetNo = saved_no;
}

TEST(BigBangShot, SuperSteveGuardsThreeSecondsThenHerShotFlashes) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BIG_BANG);
    EXPECT_TRUE(expanded::BigBangShotActive());
    EXPECT_TRUE(expanded::DetonateActive());
    BtActStatus.guard_mode = 5;
    for (int i = 0; i <= expanded::kBombChargeFrames; i++) {
        expanded::BigBangShotStep();
    }
    BtActStatus.guard_mode = 0;
    expanded::BigBangShotStep();
    // Nothing is locked, so her next shot is the flash alone, after the room's plunge.
    static CSHOT shots;
    shots.size[0] = 1.0f;
    shots.damage[0] = 10;
    expanded::BigBangShotFired(shots, 0);
    EXPECT_EQ(shots.damage[0], 0);
    for (int i = 0; i <= expanded::kRampFrames; i++) {
        expanded::BigBangShotStep();
    }
    EXPECT_FALSE(expanded::BigBangShotHanging());
    EXPECT_TRUE(expanded::SceneFlashing());
    expanded::test::SolarReset();
}

TEST(BigBangShot, AGuardStopsTheChargedShotsBlast) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BIG_BANG);
    // Drawn and held drawn a second, then let go.
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 11; // Xiao drawing the slingshot back
    for (int i = 0; i < expanded::kChargedShotFrames / 2; i++) {
        expanded::BigBangShotStep();
    }
    BtActStatus.action_no = 12; // and holding it drawn
    for (int i = 0; i < expanded::kChargedShotFrames - expanded::kChargedShotFrames / 2; i++) {
        expanded::BigBangShotStep();
    }
    BtActStatus.action_on = 0;
    expanded::BigBangShotStep();
    static CSHOT shots;
    shots.size[0] = 1.0f;
    shots.damage[0] = 10;
    expanded::BigBangShotFired(shots, 0);
    EXPECT_EQ(shots.damage[0], 0);
    const float landed[4] = {0.0f, 0.0f, 95.0f, 1.0f};
    expanded::BigBangShotEnded(shots, 0, landed);
    const COLLISION_HIT &hit = floor.hits->hit[0];
    EXPECT_GT(hit.damage, 0);
    EXPECT_FALSE(expanded::BlastHit(hit));
    EXPECT_TRUE(expanded::MonsterGuardBlocks(floor.floor.unit(), 0, 0, hit));
}

TEST(BigBang, ALandingBlindsOnlyOnceItsThrowIsOver) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_TOAN;
    expanded::test::Wielding sword(ITEM_WEAPON_BIG_BANG);
    const float              at[4] = {0.0f, 0.0f, 100.0f, 1.0f};
    expanded::SolarBlastFlash(expanded::kBigBangFlash, at, expanded::kDetonateWhp);
    // The Jacket under the landing is the blast's alone: no light hit on it.
    EXPECT_EQ(floor.hits->hit[0].life, 0);
    for (int i = 1; i < expanded::kBlastThrowFrames; i++) {
        expanded::test::SolarStep();
    }
    EXPECT_FALSE(expanded::Blinding());
    expanded::test::SolarStep();
    EXPECT_TRUE(expanded::Blinding());
    expanded::test::SolarReset();
}
