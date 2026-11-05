#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "charaheight.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_sparks.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

// Goro out with Satan's Ax at the origin, the sparks cleared.
class UnderworldSparks : public ::testing::Test {
protected:
    UnderworldSparks() : wielding_(ITEM_WEAPON_SATAN_S_AX) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
        }
        saved_time_ = CharaMain.motion_type.state.time;
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        expanded::SeedModRandom(7);
        expanded::UnderworldSparksReset();
    }

    void TearDown() override {
        expanded::UnderworldSparksReset();
        CharaMain.SetPosition(saved_pos_[0], saved_pos_[1], saved_pos_[2]);
        CharaMain.motion_type.state.time = saved_time_;
        BtActStatus = {};
    }

    // action 13 his charge held, 14 his smash, anything else neither.
    void Acting(int action) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = action;
        CharaMain.motion_type.state.time = expanded::kUnderworldSmashLands - 20.0f; // short of the ground
    }

    void Steps(int count) {
        for (int i = 0; i < count; i++) {
            expanded::UnderworldSparksStep();
        }
    }

    // A monster at (x, 0, z) the pull can draw, its one hurt sphere radius across at height y; its model has no frames,
    // so its sparks rise from inside that sphere.
    void Place(float x, float z, float y, float radius) {
        floor_.SetUpModel(0, expanded::Monster::SKELETON_SOLDIER);
        floor_.Spawn(0, 0);
        CMonstorUnit &unit = floor_.unit();
        MONSTOR      &live = unit.monster[0];
        live.state = 2;
        live.revealed = -1;
        live.knockback[2] = 1.0f;
        live.movement_speed = 0.0f;
        unit.chara[0][0].frame = nullptr;
        unit.chara[0][0].SetPosition(x, 0.0f, z);
        unit.effect[0].timer[0] = 1;
        unit.effect[0].radius[0] = radius;
        unit.effect[0].position[0][0] = x;
        unit.effect[0].position[0][1] = y;
        unit.effect[0].position[0][2] = z;
    }

    // A step of the monster's pull, then of the sparks.
    void PulledSteps(int count) {
        for (int i = 0; i < count; i++) {
            expanded::UnderworldMonsterMove(floor_.unit(), 0);
            expanded::UnderworldSparksStep();
        }
    }

    // The sparks off the ground (the body's), where they are.
    std::vector<std::array<float, 3>> Risen() {
        std::vector<std::array<float, 3>> risen;
        float                             at[3];
        for (int i = 0; expanded::UnderworldSparkAt(i, at); i++) {
            if (at[1] > expanded::kSparkLift + 0.5f) {
                risen.push_back({at[0], at[1], at[2]});
            }
        }
        return risen;
    }

    std::vector<float> Distances() {
        std::vector<float> distances;
        float              at[3];
        for (int i = 0; expanded::UnderworldSparkAt(i, at); i++) {
            distances.push_back(std::hypot(at[0], at[2]));
        }
        return distances;
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  wielding_;
    float                     saved_pos_[3] = {};
    float                     saved_time_ = 0.0f;
};

} // namespace

TEST_F(UnderworldSparks, NoneWithoutThePull) {
    Acting(0x24);
    Steps(30);
    EXPECT_EQ(expanded::UnderworldSparksLive(), 0);
}

TEST_F(UnderworldSparks, TheSmashRaisesEightTimesWhatTheHoldDoes) {
    Acting(14);
    Steps(20);
    EXPECT_EQ(expanded::UnderworldSparksLive(), static_cast<int>(20 * expanded::kSparkSmashPerFrame));
    EXPECT_EQ(expanded::kSparkSmashPerFrame, 8.0f * expanded::kSparkHoldPerFrame);
    expanded::UnderworldSparksReset();
    Acting(13);
    Steps(20);
    EXPECT_EQ(expanded::UnderworldSparksLive(), static_cast<int>(20 * expanded::kSparkHoldPerFrame));
}

TEST_F(UnderworldSparks, ItLiesAroundHimWithinItsRing) {
    Acting(14);
    Steps(2);
    std::vector<float> distances = Distances();
    ASSERT_FALSE(distances.empty());
    for (float distance : distances) {
        EXPECT_GE(distance, expanded::kSparkNear - 1.0f);
        EXPECT_LE(distance, expanded::kSparkFar + 1.0f);
    }
}

TEST_F(UnderworldSparks, ItIsDrawnInAsAMonsterIsOnTopOfItsCreep) {
    Acting(14);
    Steps(4);
    std::vector<float> before = Distances();
    Steps(1);
    std::vector<float> after = Distances();
    ASSERT_GE(after.size(), before.size());
    for (std::size_t i = 0; i < before.size(); i++) {
        float drawn = expanded::UnderworldPullAt(before[i]) + expanded::kSparkCreep;
        // Its sway across its path moves it off its line by a hair.
        EXPECT_NEAR(before[i] - after[i], std::min(drawn, before[i]), 0.02f) << before[i];
    }
}

TEST_F(UnderworldSparks, ItFadesInAsItIsDrawnIn) {
    Acting(14);
    // The first spark has moved one step: faint, not yet at full.
    Steps(1);
    std::vector<float> start = Distances();
    ASSERT_FALSE(start.empty());
    int first = expanded::UnderworldSparkAlpha(0);
    EXPECT_GE(first, 0);
    EXPECT_LT(first, expanded::kSparkAlpha / 2);
    // Brighter each step it comes nearer, never all at once.
    int last = first;
    for (int step = 0; step < 30; step++) {
        Steps(1);
        int   alpha = expanded::UnderworldSparkAlpha(0);
        float at[3];
        ASSERT_TRUE(expanded::UnderworldSparkAt(0, at));
        if (std::hypot(at[0], at[2]) < expanded::kSparkNear) {
            break;
        }
        EXPECT_GE(alpha, last);
        EXPECT_LE(alpha - last, expanded::kSparkAlpha / 3);
        last = alpha;
    }
}

TEST_F(UnderworldSparks, ItFadesOnceThePullEnds) {
    Acting(14);
    Steps(10);
    ASSERT_GT(expanded::UnderworldSparksLive(), 0);
    Acting(0x24);
    Steps(expanded::kSparkFadeOut - 1);
    EXPECT_GT(expanded::UnderworldSparksLive(), 0);
    Steps(1);
    EXPECT_EQ(expanded::UnderworldSparksLive(), 0);
}

TEST_F(UnderworldSparks, NoSparkOutlivesItsLife) {
    Acting(14);
    Steps(expanded::kSparkLife * 2);
    EXPECT_LE(expanded::UnderworldSparksLive(), expanded::kSparkCount);
    EXPECT_GT(expanded::UnderworldSparksLive(), 0);
}

TEST_F(UnderworldSparks, APulledMonsterGivesOffSparksFromItsBody) {
    Place(40.0f, 0.0f, 10.0f, 4.0f);
    Acting(14);
    PulledSteps(1);
    ASSERT_TRUE(expanded::UnderworldPulling(0));
    PulledSteps(3); // kBodySparkPerFrame a frame
    std::vector<std::array<float, 3>> risen = Risen();
    ASSERT_EQ(static_cast<int>(risen.size()), static_cast<int>(4 * expanded::kBodySparkPerFrame));
    for (const auto &at : risen) {
        // Within the sphere, drawn in a step or two since.
        EXPECT_LT(std::hypot(at[0] - 40.0f, at[1] - 10.0f, at[2]), 4.0f + 1.0f);
    }
}

TEST_F(UnderworldSparks, NoneFromAMonsterThePullLeaves) {
    Place(40.0f, 0.0f, 10.0f, 4.0f);
    Acting(14);
    floor_.unit().monster[0].knockback[2] = 0.0f; // one hits never move
    PulledSteps(10);
    EXPECT_FALSE(expanded::UnderworldPulling(0));
    EXPECT_TRUE(Risen().empty());
}

TEST_F(UnderworldSparks, ABodysSparkHeadsForTheHeightOnHimNearestItsOwn) {
    // Its body above his head: it comes down toward the top of him as it comes in, along the line between.
    Place(30.0f, 0.0f, 40.0f, 1.0f);
    Acting(14);
    PulledSteps(2);
    floor_.unit().monster[0].knockback[2] = 0.0f; // no more from it
    float                             top = CharaHeight(UserStatus);
    std::vector<std::array<float, 3>> start = Risen();
    ASSERT_FALSE(start.empty());
    PulledSteps(8);
    std::vector<std::array<float, 3>> now = Risen();
    ASSERT_EQ(now.size(), start.size());
    for (std::size_t i = 0; i < now.size(); i++) {
        float was = std::hypot(start[i][0], start[i][2]), across = std::hypot(now[i][0], now[i][2]);
        EXPECT_LT(across, was - 5.0f);
        EXPECT_LT(now[i][1], start[i][1] - 3.0f);
        // Its height above his top shrinks with its distance (its sway bends the line a little).
        EXPECT_NEAR((now[i][1] - top) / across, (start[i][1] - top) / was, 0.05f);
    }
}

TEST_F(UnderworldSparks, ABodysSparkWithinHisHeightKeepsItsOwn) {
    Place(30.0f, 0.0f, 9.0f, 0.5f);
    Acting(14);
    PulledSteps(2);
    floor_.unit().monster[0].knockback[2] = 0.0f;
    PulledSteps(8); // still on its way: the pull brings it to him in about 15
    std::vector<std::array<float, 3>> risen = Risen();
    ASSERT_FALSE(risen.empty());
    for (const auto &at : risen) {
        EXPECT_NEAR(at[1], 9.0f, 0.5f + 1e-3f);
        EXPECT_LT(std::hypot(at[0], at[2]), 30.0f);
    }
}
