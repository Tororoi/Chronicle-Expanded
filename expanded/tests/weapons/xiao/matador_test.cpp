#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/xiao/matador.hpp"
#include "expanded/weapons/charge.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// Xiao holding weapon_no (carrying sphere when given); a pellet of 40 fired into slot 3 along +x.
class ChargingBull : public ::testing::Test {
protected:
    explicit ChargingBull(int weapon_no = ITEM_WEAPON_MATADOR, int sphere = 0) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_XIAO;
        *shots_ = CSHOT{};
        shots_->used[3] = 1;
        shots_->pos[3][0] = 100.0f;
        shots_->pos[3][1] = 10.0f;
        shots_->vector[3][0] = 5.0f;
        shots_->damage[3] = 40;
        floor_.SetUpModel(0, Monster::MASTER_JACKET); // a new floor: nothing held
    }

    void TearDown() override { BtActStatus = {}; }

    // The slingshot held drawn for frames, then let go into slot 3.
    void HoldAndFire(int frames) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 12;
        for (int frame = 0; frame < frames; frame++) {
            expanded::ChargingBullStep();
        }
        BtActStatus.action_on = 0;
        expanded::ChargingBullStep();
        expanded::PelletFired(*shots_, 3);
    }

    TestFloor              floor_;
    Wielding               wielding_;
    std::unique_ptr<CSHOT> shots_ = std::make_unique<CSHOT>();
};

class ChargingBullWithAnotherWeapon : public ChargingBull {
protected:
    ChargingBullWithAnotherWeapon() : ChargingBull(ITEM_WEAPON_FLAMINGO) {}
};

class ChargingBullBySuperSteve : public ChargingBull {
protected:
    ChargingBullBySuperSteve() : ChargingBull(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_MATADOR) {}
};

} // namespace

TEST(ShotCharge, IsReadyOnceHeldItsFrames) {
    BtActStatus = {};
    expanded::ShotCharge charge(10);
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 12;
    for (int frame = 0; frame < 9; frame++) {
        charge.Step(true);
    }
    EXPECT_FALSE(charge.Ready());
    charge.Step(true);
    EXPECT_TRUE(charge.Ready());
    BtActStatus.action_on = 0;
    charge.Step(true); // let go: the hold is kept for the shot
    EXPECT_TRUE(charge.TakeCharged());
    EXPECT_FALSE(charge.TakeCharged());
    BtActStatus = {};
}

TEST(ShotCharge, CountsFromTheDraw) {
    BtActStatus = {};
    expanded::ShotCharge charge(10);
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 11; // drawing it back
    for (int frame = 0; frame < 6; frame++) {
        charge.Step(true);
    }
    EXPECT_TRUE(charge.Holding());
    BtActStatus.action_no = 12; // holding it drawn
    for (int frame = 0; frame < 4; frame++) {
        charge.Step(true);
    }
    EXPECT_TRUE(charge.Ready());
    BtActStatus = {};
}

TEST(ShotCharge, AChargedShotWearsTwice) {
    BtActStatus = {};
    expanded::ShotCharge charge(10);
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 12;
    for (int frame = 0; frame < 10; frame++) {
        charge.Step(true);
    }
    BtActStatus.action_on = 0;
    charge.Step(true);
    ASSERT_TRUE(charge.TakeCharged());
    EXPECT_FLOAT_EQ(expanded::ChargedShotWear(1.0f), expanded::kChargedShotWear);
    EXPECT_FLOAT_EQ(expanded::ChargedShotWear(1.0f), 1.0f); // the next, ordinary
    EXPECT_FALSE(charge.TakeCharged());
    EXPECT_FLOAT_EQ(expanded::ChargedShotWear(1.0f), 1.0f);
    BtActStatus = {};
}

TEST_F(ChargingBull, AFullHoldFiresAChargedPellet) {
    HoldAndFire(expanded::kChargingBullFrames);

    EXPECT_TRUE(expanded::ChargingBullCharged(*shots_, 3));
    EXPECT_EQ(shots_->damage[3], 60);
}

TEST_F(ChargingBull, AShortHoldFiresAPlainOne) {
    HoldAndFire(expanded::kChargingBullFrames - 1);

    EXPECT_FALSE(expanded::ChargingBullCharged(*shots_, 3));
    EXPECT_EQ(shots_->damage[3], 40);
}

TEST_F(ChargingBull, ItsHitGoesThroughAGuardAndShovesAlongItsFlight) {
    HoldAndFire(expanded::kChargingBullFrames);
    COLLISION_HIT hit = {};
    COLLISION_HIT plain = {};
    expanded::PelletHitSet(*shots_, 3, hit);

    EXPECT_TRUE(expanded::ChargingBullHit(hit));
    EXPECT_FALSE(expanded::ChargingBullHit(plain));
    EXPECT_FALSE(expanded::MonsterGuardBlocks(floor_.unit(), 0, 0, hit));
    EXPECT_TRUE(expanded::MonsterGuardBlocks(floor_.unit(), 0, 0, plain));
    EXPECT_EQ(hit.knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit.knockback_speed, expanded::kChargingBullKickSpeed);
    EXPECT_FLOAT_EQ(hit.knockback_decay, expanded::kChargingBullKickDecay);
    EXPECT_FLOAT_EQ(hit.knockback_origin[0], 100.0f - expanded::kChargingBullBehind);
    EXPECT_FLOAT_EQ(hit.knockback_origin[1], 10.0f);
    EXPECT_FLOAT_EQ(hit.knockback_origin[2], 0.0f);
}

TEST_F(ChargingBull, TheChargeEndsWithThePellet) {
    HoldAndFire(expanded::kChargingBullFrames);
    float point[4] = {110.0f, 10.0f, 0.0f, 1.0f};
    expanded::PelletEnded(*shots_, 3, point, false);
    EXPECT_FALSE(expanded::ChargingBullCharged(*shots_, 3));

    // The next shot let go without a hold is plain.
    shots_->damage[3] = 40;
    HoldAndFire(5);
    EXPECT_EQ(shots_->damage[3], 40);
}

TEST_F(ChargingBullWithAnotherWeapon, NeverCharges) {
    HoldAndFire(expanded::kChargingBullFrames * 2);
    EXPECT_FALSE(expanded::ChargingBullCharged(*shots_, 3));
    EXPECT_EQ(shots_->damage[3], 40);
}

TEST_F(ChargingBullBySuperSteve, ChargesWithTheSphere) {
    HoldAndFire(expanded::kChargingBullFrames);
    EXPECT_TRUE(expanded::ChargingBullCharged(*shots_, 3));
    EXPECT_EQ(shots_->damage[3], 60);
}
