#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "charaheight.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/fire_shot.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_shot.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_sparks.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_tint.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "motionmodel.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

using expanded::Monster;

namespace {

// Xiao at the origin with Super Steve carrying sphere; a pellet of 40 fired into slot 3 from (100, 10, 0) along +x.
class UnderworldShot : public ::testing::Test {
protected:
    explicit UnderworldShot(int sphere = ITEM_WEAPON_SATAN_S_AX) : wielding_(ITEM_WEAPON_SUPER_STEVE, sphere) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_XIAO;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
        }
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        *shots_ = CSHOT{};
        shots_->used[3] = 1;
        shots_->pos[3][0] = 100.0f;
        shots_->pos[3][1] = 10.0f;
        shots_->vector[3][0] = 5.0f;
        shots_->life[3] = 120;
        shots_->damage[3] = 40;
        shots_->size[3] = 1.0f;
        saved_hits_ = NowColData;
        NowColData = hits_.get();
        *hits_ = CCollisionData{};
        expanded::SeedModRandom(7);
        expanded::UnderworldShotReset();
        expanded::UnderworldReset();
        expanded::UnderworldSparksReset();
        expanded::BurningAuraReset();
    }

    void TearDown() override {
        expanded::UnderworldShotReset();
        expanded::UnderworldReset();
        expanded::UnderworldSparksReset();
        expanded::BurningAuraReset();
        NowColData = saved_hits_;
        CharaMain.SetPosition(saved_pos_[0], saved_pos_[1], saved_pos_[2]);
        BtActStatus = {};
    }

    // The slingshot held drawn for frames, then let go into slot 3.
    void HoldAndFire(int frames) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 12;
        for (int frame = 0; frame < frames; frame++) {
            expanded::UnderworldShotStep();
        }
        BtActStatus.action_on = 0;
        expanded::UnderworldShotStep();
        expanded::PelletFired(*shots_, 3);
    }

    // A Skeleton Soldier in slot at (x, z), standing still, of weight 1.
    void Place(int slot, float x, float z) {
        floor_.SetUpModel(slot, Monster::SKELETON_SOLDIER);
        floor_.Spawn(slot, slot);
        MONSTOR &live = floor_.unit().monster[slot];
        live.state = 2;
        live.revealed = -1;
        live.knockback[2] = 1.0f;
        live.movement_speed = 0.0f;
        floor_.unit().chara[slot][0].frame = nullptr;
        floor_.unit().chara[slot][0].SetPosition(x, 0.0f, z);
    }

    // Its step this frame, across the ground.
    std::array<float, 2> Step(int slot) {
        expanded::UnderworldMonsterMove(floor_.unit(), slot);
        const MONSTOR &live = floor_.unit().monster[slot];
        return {live.movement[0] * live.movement_speed, live.movement[2] * live.movement_speed};
    }

    // A hit the pellet in slot 3 plants, in record.
    COLLISION_HIT &Planted(int record) {
        COLLISION_HIT &hit = NowColData->hit[record];
        hit = {};
        expanded::PelletHitSet(*shots_, 3, hit);
        return hit;
    }

    expanded::test::TestFloor       floor_;
    expanded::test::Wielding        wielding_;
    expanded::test::StepsUnpaused   unpaused_;
    std::unique_ptr<CSHOT>          shots_ = std::make_unique<CSHOT>();
    std::unique_ptr<CCollisionData> hits_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits_ = nullptr;
    float                           saved_pos_[3] = {};
};

class UnderworldShotWithAnotherSphere : public UnderworldShot {
protected:
    UnderworldShotWithAnotherSphere() : UnderworldShot(ITEM_WEAPON_MATADOR) {}
};

class InfernoShot : public UnderworldShot {
protected:
    InfernoShot() : UnderworldShot(ITEM_WEAPON_INFERNO) {}

    // frames of the slingshot held drawn, a step of the shot and of the Inferno each.
    void Hold(int frames) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 12;
        for (int frame = 0; frame < frames; frame++) {
            expanded::UnderworldShotStep();
            expanded::BurningAuraStep();
        }
    }

    // Let go into slot 3.
    void Fire() {
        BtActStatus.action_on = 0;
        expanded::UnderworldShotStep();
        expanded::PelletFired(*shots_, 3);
    }

    // Steps of the Inferno with nothing drawn.
    void After(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::UnderworldShotStep();
            expanded::BurningAuraStep();
        }
    }
};

} // namespace

TEST_F(UnderworldShot, AChargedShotFliesTwiceAsLargeAtAQuarterSpeedForTwiceAsLong) {
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_FLOAT_EQ(shots_->vector[3][0], 5.0f * expanded::kUnderworldShotSpeed);
    EXPECT_EQ(shots_->life[3], static_cast<int>(120 * expanded::kUnderworldShotLife));
    EXPECT_FLOAT_EQ(shots_->size[3], expanded::kUnderworldShotSize);
    float at[3];
    ASSERT_TRUE(expanded::UnderworldShotAt(at));
    EXPECT_FLOAT_EQ(at[0], 100.0f);
    EXPECT_FLOAT_EQ(at[1], 10.0f);
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 1.0f);
}

TEST_F(UnderworldShot, AShotNotHeldLongEnoughIsItsOwn) {
    HoldAndFire(expanded::kUnderworldShotFrames - 10);
    EXPECT_FLOAT_EQ(shots_->vector[3][0], 5.0f);
    EXPECT_EQ(shots_->life[3], 120);
    EXPECT_FLOAT_EQ(shots_->size[3], 1.0f);
    EXPECT_FALSE(expanded::UnderworldShotAt(nullptr));
    EXPECT_FALSE(expanded::UnderworldShotPierces(*shots_, 3));
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 0.0f);
}

TEST_F(UnderworldShotWithAnotherSphere, OnlyWithSatansAxsSphere) {
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_FALSE(expanded::UnderworldShotAt(nullptr));
}

TEST_F(UnderworldShot, TheMonstersAreDrawnToThePelletNotToHer) {
    Place(0, 140.0f, 0.0f); // 40 beyond the pellet, 140 from her (a new floor: placed first)
    HoldAndFire(expanded::kUnderworldShotFrames);
    std::array<float, 2> step = Step(0);
    EXPECT_NEAR(step[0], -expanded::UnderworldPullAt(40.0f) * expanded::kUnderworldShotPull, 1e-4f);
    EXPECT_NEAR(step[1], 0.0f, 1e-4f);
    EXPECT_TRUE(expanded::UnderworldPulling(0));
}

TEST_F(UnderworldShot, AMonsterItHasGoneThroughIsDrawnThreeTimesAsHard) {
    Place(0, 140.0f, 0.0f); // 40 beyond the pellet
    Place(1, 140.0f, 30.0f);
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_FALSE(expanded::UnderworldShotStruck(0));
    EXPECT_FALSE(expanded::UnderworldShotRefused(0, Planted(5))); // it goes through the first
    EXPECT_TRUE(expanded::UnderworldShotStruck(0));
    EXPECT_FALSE(expanded::UnderworldShotStruck(1));
    std::array<float, 2> step = Step(0);
    EXPECT_NEAR(step[0], -expanded::UnderworldPullAt(40.0f) * expanded::kUnderworldShotPull * expanded::kUnderworldStruckPull, 1e-4f);
    float pulled = std::hypot(Step(1)[0], Step(1)[1]);
    EXPECT_LT(pulled, expanded::UnderworldPullAt(40.0f) * expanded::kUnderworldShotPull * 1.01f); // as ever
    // Until the pellet ends.
    float point[3] = {110.0f, 10.0f, 0.0f};
    expanded::PelletEnded(*shots_, 3, point, false);
    EXPECT_FALSE(expanded::UnderworldShotStruck(0));
}

TEST_F(UnderworldShot, ThePullEndsWithThePellet) {
    Place(0, 140.0f, 0.0f);
    HoldAndFire(expanded::kUnderworldShotFrames);
    float point[3] = {110.0f, 10.0f, 0.0f};
    expanded::PelletEnded(*shots_, 3, point, false);
    shots_->used[3] = 0;
    EXPECT_FALSE(expanded::UnderworldShotAt(nullptr));
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 0.0f);
    std::array<float, 2> step = Step(0);
    EXPECT_FLOAT_EQ(step[0], 0.0f);
}

TEST_F(UnderworldShot, ItGoesOnThroughAMonsterHittingEachOnce) {
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_TRUE(expanded::UnderworldShotPierces(*shots_, 3));
    // Its hit, planted again each frame it is inside them: each monster takes the first.
    EXPECT_FALSE(expanded::UnderworldShotRefused(0, Planted(5)));
    EXPECT_TRUE(expanded::UnderworldShotRefused(0, Planted(6)));
    EXPECT_FALSE(expanded::UnderworldShotRefused(1, Planted(7)));
    EXPECT_TRUE(expanded::UnderworldShotRefused(1, Planted(8)));
    EXPECT_TRUE(expanded::UnderworldShotRefused(0, Planted(9)));
}

TEST_F(UnderworldShot, APelletNotChargedEndsOnAMonster) {
    HoldAndFire(10);
    EXPECT_FALSE(expanded::UnderworldShotPierces(*shots_, 3));
    EXPECT_FALSE(expanded::UnderworldShotRefused(0, Planted(5)));
    EXPECT_FALSE(expanded::UnderworldShotRefused(0, Planted(6)));
}

TEST_F(UnderworldShot, TheNextPelletHitsThemAgain) {
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_FALSE(expanded::UnderworldShotRefused(0, Planted(5)));
    float point[3] = {110.0f, 10.0f, 0.0f};
    expanded::PelletEnded(*shots_, 3, point, false);
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_FALSE(expanded::UnderworldShotRefused(0, Planted(6)));
}

TEST_F(UnderworldShot, ItsSparksComeInToThePellet) {
    HoldAndFire(expanded::kUnderworldShotFrames);
    for (int step = 0; step < 4; step++) {
        expanded::UnderworldSparksStep();
    }
    ASSERT_GT(expanded::UnderworldSparksLive(), 0);
    float at[3];
    for (int i = 0; expanded::UnderworldSparkAt(i, at); i++) {
        float across = std::hypot(at[0] - 100.0f, at[2]);
        EXPECT_LE(across, expanded::kSparkFar + 1.0f);
        // Rising from the ground to the pellet as it comes in.
        EXPECT_GE(at[1], expanded::kSparkLift - 1e-3f);
        EXPECT_LE(at[1], 10.0f);
    }
    float before[3];
    ASSERT_TRUE(expanded::UnderworldSparkAt(0, before));
    for (int step = 0; step < 10; step++) {
        expanded::UnderworldSparksStep();
    }
    ASSERT_TRUE(expanded::UnderworldSparkAt(0, at));
    EXPECT_LT(std::hypot(at[0] - 100.0f, at[2]), std::hypot(before[0] - 100.0f, before[2]));
    EXPECT_GT(at[1], before[1]);
}

TEST_F(UnderworldShot, SuperSteveIsNotTinted) {
    expanded::UnderworldTintReset();
    HoldAndFire(expanded::kUnderworldShotFrames);
    for (int step = 0; step < expanded::kUnderworldTintFade; step++) {
        expanded::UnderworldTintStep();
    }
    EXPECT_FLOAT_EQ(expanded::UnderworldTint(), 0.0f);
}

TEST_F(InfernoShot, TheInfernosSpherePullsAQuarterAsStronglyWhileTheSlingshotIsDrawn) {
    EXPECT_TRUE(expanded::InfernoSphere());
    EXPECT_TRUE(expanded::InfernoBurning());
    EXPECT_FALSE(expanded::InfernoWielded());
    Hold(10);
    EXPECT_TRUE(expanded::UnderworldShotDrawn());
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), expanded::kUnderworldChargeShare);
    expanded::UnderworldCentre centre;
    ASSERT_TRUE(expanded::UnderworldCentreNow(centre));
    EXPECT_FLOAT_EQ(centre.x, 0.0f); // on her
    EXPECT_FLOAT_EQ(centre.high - centre.ground, CharaHeight(UserStatus));
}

TEST_F(UnderworldShot, SatansAxsSphereDoesNotPullWhileDrawn) {
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 12;
    for (int frame = 0; frame < 10; frame++) {
        expanded::UnderworldShotStep();
    }
    EXPECT_FALSE(expanded::UnderworldShotDrawn());
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 0.0f);
    EXPECT_FALSE(expanded::InfernoBurning());
}

TEST_F(InfernoShot, ItsChargedShotIsTheFireballInPlaceOfThePellet) {
    Hold(expanded::kUnderworldShotFrames);
    Fire();
    EXPECT_TRUE(expanded::UnderworldShotPierces(*shots_, 3));
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 1.0f);
    EXPECT_TRUE(expanded::PelletHidden(*shots_, 3));
    EXPECT_TRUE(expanded::FireShotsFlying());
}

TEST_F(UnderworldShot, SatansAxsChargedShotKeepsItsPellet) {
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_FALSE(expanded::PelletHidden(*shots_, 3));
}

TEST_F(InfernoShot, XiaoIsFireproof) {
    COLLISION_HIT fire{};
    fire.flags = expanded::kFireElementBit;
    EXPECT_TRUE(expanded::InfernoFireproof(fire));
    COLLISION_HIT plain{};
    EXPECT_FALSE(expanded::InfernoFireproof(plain));
}

TEST_F(InfernoShot, ItsEndBlastsEveryMonsterWithinTwentyUnitsForThePelletsDamage) {
    Place(0, 115.0f, 0.0f); // 15 from where it ends
    Place(1, 140.0f, 0.0f); // 40 from it
    Hold(expanded::kUnderworldShotFrames);
    Fire();
    const float end[3] = {100.0f, 0.0f, 0.0f};
    expanded::PelletEnded(*shots_, 3, end, false);
    int near = 0, far = 0;
    for (int i = 0; i < static_cast<int>(sizeof(NowColData->hit) / sizeof(NowColData->hit[0])); i++) {
        if (NowColData->active[i] == 0) {
            continue;
        }
        const COLLISION_HIT &hit = NowColData->hit[i];
        EXPECT_EQ(hit.damage, 40);
        float d = std::hypot(hit.pos[0] - 115.0f, hit.pos[2]);
        (d < 10.0f ? near : far)++;
    }
    EXPECT_GT(near, 0);
    EXPECT_EQ(far, 0);
    EXPECT_FALSE(expanded::FireShotsFlying());
}

TEST_F(InfernoShot, HerFlameRisesWhileDrawnThenFlaresAndDiesDownAsSheFires) {
    const float body = CharaHeight(UserStatus) / expanded::kRisingFlameHeight;
    Hold(expanded::kRisingFlameRise);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 1.0f);
    EXPECT_FLOAT_EQ(expanded::RisingFlameScale(), expanded::kRisingFlameScale * body);
    Fire();
    After(expanded::kRisingFlameFlare);
    EXPECT_FLOAT_EQ(expanded::RisingFlameScale(), expanded::kRisingFlameSmashScale * body);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 1.0f);
    After(expanded::kRisingFlameFade / 2);
    EXPECT_NEAR(expanded::RisingFlameRisen(), 0.5f, 0.02f);
    EXPECT_NEAR(expanded::RisingFlameScale(), 0.5f * expanded::kRisingFlameSmashScale * body, 0.02f);
    After(expanded::kRisingFlameFade / 2);
    EXPECT_FLOAT_EQ(expanded::RisingFlameRisen(), 0.0f);
}

TEST_F(InfernoShot, ItsEndBlastIsASecondHitOnAMonsterItWentThrough) {
    Place(0, 110.0f, 0.0f);
    Hold(expanded::kUnderworldShotFrames);
    Fire();
    EXPECT_FALSE(expanded::MonsterHitRefused(floor_.unit(), 0, Planted(5))); // it goes through: once
    EXPECT_TRUE(expanded::MonsterHitRefused(floor_.unit(), 0, Planted(6)));
    for (int i = 0; i < static_cast<int>(sizeof(NowColData->active) / sizeof(NowColData->active[0])); i++) {
        NowColData->active[i] = 0;
    }
    const float end[3] = {115.0f, 0.0f, 0.0f};
    expanded::PelletEnded(*shots_, 3, end, false);
    int blasts = 0;
    for (int i = 0; i < static_cast<int>(sizeof(NowColData->active) / sizeof(NowColData->active[0])); i++) {
        if (NowColData->active[i] != 0) {
            blasts++;
            EXPECT_FALSE(expanded::MonsterHitRefused(floor_.unit(), 0, NowColData->hit[i]));
        }
    }
    EXPECT_GT(blasts, 0);
}

TEST_F(InfernoShot, ItsChargedShotHitsWithFireItsOtherShotsWithTheSelectedElement) {
    Hold(10);
    Fire(); // not charged
    COLLISION_HIT &plain = Planted(5);
    EXPECT_EQ(plain.flags, 0); // left to the game's selected element
    shots_->used[3] = 1;
    Hold(expanded::kUnderworldShotFrames);
    Fire();
    EXPECT_EQ(Planted(6).flags, GetWeaponElementAttr(WEAPON_ELEMENT_FIRE));
}

TEST_F(UnderworldShot, SatansAxsChargedShotKeepsTheSelectedElement) {
    HoldAndFire(expanded::kUnderworldShotFrames);
    EXPECT_EQ(Planted(5).flags, 0);
}
