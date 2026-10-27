#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "expanded/weapons/zeus_shot.hpp"
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
        expanded::ZeusChargeReset();
        expanded::ZeusShotReset();
    }

    ~Floor() {
        expanded::test::SolarReset();
        expanded::ZeusChargeReset();
        expanded::ZeusShotReset();
        BtActStatus = {};
        NowColData = saved;
    }
};

} // namespace

TEST(SwordOfZeus, ItsChargeIsAlwaysTheLunge) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_TOAN;
    {
        expanded::test::Wielding sword(ITEM_WEAPON_SWORD_OF_ZEUS);
        EXPECT_FALSE(expanded::WhirlwindAllowed());
    }
    expanded::test::Wielding sword(ITEM_WEAPON_BIG_BANG);
    EXPECT_TRUE(expanded::WhirlwindAllowed());
}

TEST(SwordOfZeus, HeldThreeSecondsPastTheLungeItJumpsHigher) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_TOAN;
    expanded::test::Wielding sword(ITEM_WEAPON_SWORD_OF_ZEUS);
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 0xE;
    BtActStatus.charge_time = 1.5f;
    for (int i = 0; i < expanded::kChargeHighFrames - 1; i++) {
        expanded::ZeusChargeStep();
    }
    EXPECT_FLOAT_EQ(expanded::LungeGravity(0.1f), 0.1f);
    expanded::ZeusChargeStep();
    expanded::ZeusChargeStep();
    EXPECT_FLOAT_EQ(expanded::LungeGravity(0.1f), 0.1f * expanded::kChargeHigh);
    // Let go another way, it is spent.
    BtActStatus.action_on = 0;
    expanded::ZeusChargeStep();
    EXPECT_FLOAT_EQ(expanded::LungeGravity(0.1f), 0.1f);
}

TEST(SwordOfZeus, ItsSolarFlashHangsTheBladeOverTheLockedTarget) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_TOAN;
    expanded::test::Wielding sword(ITEM_WEAPON_SWORD_OF_ZEUS);
    s32 saved_flag = lockOnTargetFlag;
    s32 saved_no = lockOnTargetNo;
    lockOnTargetFlag = 1;
    lockOnTargetNo = 0;
    BtActStatus.guard_mode = 5;
    // The charge attack's step runs beside it every frame and leaves its blade be.
    for (int i = 0; i <= expanded::kSolarChargeFrames + expanded::kJudgementFadeFrames; i++) {
        expanded::test::SolarStep();
        expanded::ZeusChargeStep();
    }
    EXPECT_EQ(expanded::SolarFlashPhase(), expanded::SolarPhase::kPrimed);
    EXPECT_TRUE(expanded::JudgementHanging());
    EXPECT_GT(expanded::JudgementShown(), 0.0f);
    lockOnTargetFlag = saved_flag;
    lockOnTargetNo = saved_no;
}

TEST(ZeusShot, PrimedHerShotNotLockedOnCallsItsBoltWhereItLands) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SWORD_OF_ZEUS);
    EXPECT_TRUE(expanded::ZeusShotActive());
    BtActStatus.guard_mode = 5;
    for (int i = 0; i <= expanded::kZeusGuardFrames; i++) {
        expanded::ZeusShotStep();
    }
    BtActStatus.guard_mode = 0;
    static CSHOT shots;
    shots.size[0] = 1.0f;
    shots.damage[0] = 10;
    expanded::ZeusShotFired(shots, 0);
    EXPECT_EQ(shots.damage[0], 0);        // the bolt is its hit
    EXPECT_FLOAT_EQ(shots.size[0], 1.0f); // it flies
    EXPECT_FALSE(expanded::PelletHurts(shots, 0)); // it lands nothing of its own
    // The charge went with it: her next shot is a plain one.
    static CSHOT more;
    more.damage[0] = 10;
    expanded::ZeusShotFired(more, 0);
    EXPECT_EQ(more.damage[0], 10);
}

TEST(ZeusShot, ItsChargeCountsFromTheDraw) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SWORD_OF_ZEUS);
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 11; // drawing the slingshot back
    for (int i = 0; i < expanded::kZeusChargedFrames / 2; i++) {
        expanded::ZeusShotStep();
    }
    BtActStatus.action_no = 12; // holding it drawn
    for (int i = 0; i < expanded::kZeusChargedFrames - expanded::kZeusChargedFrames / 2; i++) {
        expanded::ZeusShotStep();
    }
    BtActStatus.action_on = 0;
    expanded::ZeusShotStep();
    static CSHOT shots;
    shots.size[0] = 1.0f;
    shots.damage[0] = 10;
    expanded::ZeusShotFired(shots, 0);
    EXPECT_EQ(shots.damage[0], 0); // charged: its bolt is its hit
    BtActStatus = {};
}
