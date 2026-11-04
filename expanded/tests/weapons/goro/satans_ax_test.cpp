#include <gtest/gtest.h>

#include <array>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// Goro out with weapon_no at the origin, smashing (his full charge let go), the dungeon playing; monsters of their kinds in play.
class UnderworldPull : public ::testing::Test {
protected:
    explicit UnderworldPull(int weapon_no = ITEM_WEAPON_SATAN_S_AX) : wielding_(weapon_no) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
        }
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        Smash(true);
        expanded::UnderworldReset();
    }

    void TearDown() override {
        expanded::UnderworldReset();
        CharaMain.SetPosition(saved_pos_[0], saved_pos_[1], saved_pos_[2]);
        BtActStatus = {};
    }

    void Smash(bool on) {
        BtActStatus.action_on = on ? 1 : 0;
        BtActStatus.action_no = on ? 14 : 0;
    }

    // A monster of kind in slot at (x, z), standing still; its weight its kind's (SetupViewMonstor's).
    MONSTOR &Place(int slot, Monster kind, float x, float z) {
        floor_.SetUpModel(slot, kind);
        floor_.Spawn(slot, slot);
        MONSTOR &live = floor_.unit().monster[slot];
        live.state = 2;
        live.revealed = -1;
        live.knockback[2] = floor_.unit().model[slot].knockback_scale;
        live.movement_speed = 0.0f;
        floor_.unit().chara[slot][0].SetPosition(x, 0.0f, z);
        return live;
    }

    // Its step this frame (its movement times its speed).
    std::array<float, 3> Step(int slot) {
        expanded::UnderworldMonsterMove(floor_.unit(), slot);
        const MONSTOR &live = floor_.unit().monster[slot];
        return {live.movement[0] * live.movement_speed, live.movement[1] * live.movement_speed, live.movement[2] * live.movement_speed};
    }

    expanded::test::TestFloor     floor_;
    expanded::test::Wielding      wielding_;
    expanded::test::StepsUnpaused unpaused_;
    float                         saved_pos_[3] = {};
};

class UnderworldWithAnotherAx : public UnderworldPull {
protected:
    UnderworldWithAnotherAx() : UnderworldPull(ITEM_WEAPON_BATTLE_AX) {}
};

} // namespace

TEST_F(UnderworldPull, HisSmashDrawsAMonsterToward) {
    Place(0, Monster::SKELETON_SOLDIER, 100.0f, 0.0f);
    ASSERT_FLOAT_EQ(floor_.unit().monster[0].knockback[2], 1.0f);
    auto step = Step(0);
    EXPECT_NEAR(step[0], -expanded::UnderworldPullAt(100.0f), 1e-5f);
    EXPECT_NEAR(step[2], 0.0f, 1e-5f);
}

TEST_F(UnderworldPull, NearerIsDrawnHarderAndBeyondTheReachNotAtAll) {
    EXPECT_FLOAT_EQ(expanded::UnderworldPullAt(0.0f), expanded::kUnderworldPullNear);
    EXPECT_NEAR(expanded::UnderworldPullAt(10.0f), 4.0f, 0.1f);
    EXPECT_NEAR(expanded::UnderworldPullAt(40.0f), 0.2f, 0.01f);
    // Exponential: each kUnderworldFalloff nearer pulls e times harder.
    for (float distance : {10.0f, 40.0f, 70.0f}) {
        EXPECT_NEAR(expanded::UnderworldPullAt(distance) / expanded::UnderworldPullAt(distance + expanded::kUnderworldFalloff), std::exp(1.0f), 1e-4f)
            << distance;
    }
    EXPECT_GT(expanded::UnderworldPullAt(expanded::kUnderworldReach), 0.0f);
    EXPECT_FLOAT_EQ(expanded::UnderworldPullAt(expanded::kUnderworldReach + 0.1f), 0.0f);
    Place(0, Monster::SKELETON_SOLDIER, 50.0f, 0.0f);
    EXPECT_NEAR(Step(0)[0], -expanded::UnderworldPullAt(50.0f), 1e-5f);
    Place(1, Monster::SKELETON_SOLDIER, 0.0f, 101.0f);
    EXPECT_FLOAT_EQ(Step(1)[2], 0.0f);
}

TEST_F(UnderworldPull, AHeavierMonsterIsDrawnLessAndOparNotAtAll) {
    MONSTOR &golem = Place(0, Monster::GOLEM, 0.0f, 100.0f);
    ASSERT_GT(golem.knockback[2], 0.0f);
    ASSERT_LT(golem.knockback[2], 1.0f);
    EXPECT_NEAR(Step(0)[2], -expanded::UnderworldPullAt(100.0f) * golem.knockback[2], 1e-5f);
    MONSTOR &opar = Place(1, Monster::OPAR, 100.0f, 0.0f);
    ASSERT_FLOAT_EQ(opar.knockback[2], 0.0f);
    EXPECT_FLOAT_EQ(Step(1)[0], 0.0f);
}

TEST_F(UnderworldPull, ThePullAddsToItsOwnStep) {
    MONSTOR &live = Place(0, Monster::SKELETON_SOLDIER, 100.0f, 0.0f);
    live.movement[0] = 0.0f, live.movement[1] = 0.0f, live.movement[2] = 1.0f;
    live.movement_speed = 0.8f;
    auto step = Step(0);
    EXPECT_NEAR(step[0], -expanded::UnderworldPullAt(100.0f), 1e-5f);
    EXPECT_NEAR(step[2], 0.8f, 1e-5f);
}

TEST_F(UnderworldPull, ItStopsWhereItsBodyMeetsHis) {
    // No sphere posed: its body is kBodyFallback about its root.
    float meets = expanded::kBodyFallback + expanded::kColossusBodyRadius;
    Place(0, Monster::SKELETON_SOLDIER, meets + 0.1f, 0.0f);
    EXPECT_NEAR(Step(0)[0], -0.1f, 1e-4f);
    Place(1, Monster::SKELETON_SOLDIER, meets - 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(Step(1)[0], 0.0f);
}

TEST_F(UnderworldPull, AQuarterAsStrongWhileHeHoldsTheCharge) {
    Place(0, Monster::SKELETON_SOLDIER, 30.0f, 0.0f);
    BtActStatus.action_no = 13; // still holding the charge
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), expanded::kUnderworldChargeShare);
    EXPECT_NEAR(Step(0)[0], -expanded::kUnderworldChargeShare * expanded::UnderworldPullAt(30.0f), 1e-5f);
    Smash(true);
    EXPECT_NEAR(Step(0)[0], -expanded::UnderworldPullAt(30.0f), 1e-5f);
}

TEST_F(UnderworldPull, OnlyWhileHisChargeOrSmashLasts) {
    Place(0, Monster::SKELETON_SOLDIER, 100.0f, 0.0f);
    BtActStatus.action_no = 0x24; // his combo
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 0.0f);
    EXPECT_FLOAT_EQ(Step(0)[0], 0.0f);
    Smash(true);
    BtActStatus.action_on = 0; // the smash over (GoroKey_Play leaves action_no at 14)
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 0.0f);
    floor_.status().cur_chara = CHARA_XIAO;
    Smash(true);
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 0.0f); // Goro's alone
}

TEST_F(UnderworldWithAnotherAx, NoOtherAxPulls) {
    Place(0, Monster::SKELETON_SOLDIER, 100.0f, 0.0f);
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 0.0f);
    EXPECT_FLOAT_EQ(Step(0)[0], 0.0f);
}

TEST_F(UnderworldPull, ItDrawsOnWithoutGatheringSpeed) {
    MONSTOR &live = Place(0, Monster::SKELETON_SOLDIER, 100.0f, 0.0f);
    for (int frame = 0; frame < 3; frame++) {
        EXPECT_NEAR(Step(0)[0], -expanded::UnderworldPullAt(100.0f), 1e-5f) << frame; // its script set no new step
    }
    live.movement[0] = 0.0f, live.movement[1] = 0.0f, live.movement[2] = 1.0f; // its script's new step
    live.movement_speed = 0.5f;
    auto step = Step(0);
    EXPECT_NEAR(step[0], -expanded::UnderworldPullAt(100.0f), 1e-5f);
    EXPECT_NEAR(step[2], 0.5f, 1e-5f);
    Smash(false); // the smash over: the pull it took comes off
    step = Step(0);
    EXPECT_NEAR(step[0], 0.0f, 1e-5f);
    EXPECT_NEAR(step[2], 0.5f, 1e-5f);
}

// A light monster standing beyond kUnderworldSafe, drawn by the pull alone, is still out of the smash's reach (no nearer
// than kUnderworldSafe - 20) when the smash ends; one nearer is drawn in to meet him.
TEST(UnderworldPullStrength, ALightMonsterBeyondTheSafeDistanceIsNotDrawnIntoTheSmash) {
    auto drawn = [](float distance) {
        for (int frame = 0; frame < expanded::kUnderworldSmashFrames; frame++) {
            distance -= expanded::UnderworldPullAt(distance);
        }
        return distance;
    };
    EXPECT_GT(drawn(expanded::kUnderworldSafe), expanded::kUnderworldSafe - 20.0f);
    EXPECT_LT(drawn(40.0f), 15.0f);
}
