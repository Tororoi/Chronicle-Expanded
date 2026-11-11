#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
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
    explicit UnderworldSparks(int weapon_no = ITEM_WEAPON_SATAN_S_AX) : wielding_(weapon_no) {}

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

    // Steps until the first spark starts.
    void FirstSpark() {
        for (int step = 0; expanded::UnderworldSparksLive() == 0; step++) {
            ASSERT_LT(step, 20);
            Steps(1);
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

    // Every spark, where it is, the first started first.
    std::vector<std::array<float, 3>> All() {
        std::vector<std::array<float, 3>> all;
        float                             at[3];
        for (int i = 0; expanded::UnderworldSparkAt(i, at); i++) {
            all.push_back({at[0], at[1], at[2]});
        }
        return all;
    }

    // Each spark where it was first seen, over steps steps of step (one spark per slot, none gone in that time).
    template <typename Step>
    std::vector<std::array<float, 3>> FirstSeen(int steps, Step step) {
        std::vector<std::array<float, 3>> first;
        for (int i = 0; i < steps; i++) {
            step();
            std::vector<std::array<float, 3>> all = All();
            first.insert(first.end(), all.begin() + static_cast<std::ptrdiff_t>(first.size()), all.end());
        }
        return first;
    }

    // Each spark off the ground in start heading straight in (along the line from where it was) for the height on him
    // its own maps to.
    void ExpectHeadingForWhereItsOwnMaps(const std::vector<std::array<float, 3>> &start, const std::vector<std::array<float, 3>> &now) {
        const float top = CharaHeight(UserStatus);
        ASSERT_GE(now.size(), start.size()); // none gone, the new ones after
        int risen = 0;
        for (std::size_t i = 0; i < start.size(); i++) {
            if (start[i][1] <= expanded::kSparkLift + 0.5f) {
                continue; // on the ground
            }
            risen++;
            float was = std::hypot(start[i][0], start[i][2]), across = std::hypot(now[i][0], now[i][2]);
            float to = expanded::InfernoSparkHeight(start[i][1], top);
            EXPECT_LT(across, was);
            EXPECT_NEAR((now[i][1] - to) / across, (start[i][1] - to) / was, 2e-3f);
        }
        EXPECT_GT(risen, 0);
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

class UnderworldSparksWithInferno : public UnderworldSparks {
protected:
    UnderworldSparksWithInferno() : UnderworldSparks(ITEM_WEAPON_INFERNO) {}
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

TEST_F(UnderworldSparks, ItComesStraightAtHim) {
    Acting(14);
    FirstSpark();
    float start[3], at[3];
    ASSERT_TRUE(expanded::UnderworldSparkAt(0, start));
    Steps(5);
    ASSERT_TRUE(expanded::UnderworldSparkAt(0, at));
    EXPECT_NEAR(at[0] * start[2] - at[2] * start[0], 0.0f, 1e-2f);
}

TEST_F(UnderworldSparks, ItGathersSpeedSteadily) {
    // The first spark started: its steps in grow by kSparkAccel each, times the pull's strength.
    for (auto [action, strength] : {
             std::pair{14, 1.0f                            },
             std::pair{13, expanded::kUnderworldChargeShare}
    }) {
        expanded::UnderworldSparksReset();
        Acting(action);
        std::vector<float> was;
        for (int step = 0; was.size() < 4; step++) {
            Steps(1);
            float at[3];
            if (expanded::UnderworldSparkAt(0, at)) {
                was.push_back(std::hypot(at[0], at[2]));
            }
            ASSERT_LT(step, 20);
        }
        EXPECT_NEAR(was[0] - was[1], expanded::kSparkCreep + expanded::kSparkAccel * strength, 1e-3f) << action;
        for (int k = 1; k + 1 < 4; k++) {
            EXPECT_NEAR((was[k] - was[k + 1]) - (was[k - 1] - was[k]), expanded::kSparkAccel * strength, 1e-3f) << action;
        }
    }
}

TEST_F(UnderworldSparks, ItFadesInAsItIsDrawnIn) {
    Acting(14);
    // The first spark has moved one step: faint, not yet at full.
    FirstSpark();
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
    PulledSteps(7); // kBodySparkPerFrame a frame
    std::vector<std::array<float, 3>> risen = Risen();
    ASSERT_EQ(static_cast<int>(risen.size()), static_cast<int>(8 * expanded::kBodySparkPerFrame));
    for (const auto &at : risen) {
        // Within the sphere, drawn in up to seven steps since.
        EXPECT_LT(std::hypot(at[0] - 40.0f, at[1] - 10.0f, at[2]), 4.0f + 7.0f * expanded::kSparkCreep + 21.0f * expanded::kBodySparkAccel);
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
    Place(55.0f, 0.0f, 40.0f, 1.0f);
    Acting(14);
    PulledSteps(4);
    floor_.unit().monster[0].knockback[2] = 0.0f; // no more from it
    float                             top = CharaHeight(UserStatus);
    std::vector<std::array<float, 3>> start = Risen();
    ASSERT_FALSE(start.empty());
    PulledSteps(6);
    std::vector<std::array<float, 3>> now = Risen();
    ASSERT_EQ(now.size(), start.size());
    for (std::size_t i = 0; i < now.size(); i++) {
        float was = std::hypot(start[i][0], start[i][2]), across = std::hypot(now[i][0], now[i][2]);
        EXPECT_LT(across, was - 5.0f);
        EXPECT_LT(now[i][1], start[i][1] - 2.0f);
        // Its height above his top shrinks with its distance: a straight line.
        EXPECT_NEAR((now[i][1] - top) / across, (start[i][1] - top) / was, 1e-3f);
        // Heading straight at him.
        EXPECT_NEAR(now[i][0] * start[i][2] - now[i][2] * start[i][0], 0.0f, 1e-2f);
    }
}

TEST_F(UnderworldSparks, ABodysSparkWithinHisHeightKeepsItsOwn) {
    Place(55.0f, 0.0f, 9.0f, 0.5f);
    Acting(14);
    PulledSteps(4);
    floor_.unit().monster[0].knockback[2] = 0.0f;
    PulledSteps(3); // still on its way: the pull brings it to him in about 10
    std::vector<std::array<float, 3>> risen = Risen();
    ASSERT_FALSE(risen.empty());
    for (const auto &at : risen) {
        EXPECT_NEAR(at[1], 9.0f, 0.5f + 1e-3f);
        EXPECT_LT(std::hypot(at[0], at[2]), 55.0f);
    }
}

TEST_F(UnderworldSparks, ABodysSparkGathersSpeedFasterThanAGroundSpark) {
    EXPECT_GT(expanded::kBodySparkAccel, expanded::kSparkAccel);
    Place(55.0f, 0.0f, 9.0f, 0.5f);
    Acting(14);
    PulledSteps(4);
    floor_.unit().monster[0].knockback[2] = 0.0f; // no more from it
    std::vector<std::vector<std::array<float, 3>>> at;
    for (int step = 0; step < 3; step++) {
        at.push_back(Risen());
        ASSERT_FALSE(at.back().empty());
        PulledSteps(1);
    }
    at.push_back(Risen());
    for (std::size_t i = 0; i < at[0].size(); i++) {
        float d[4];
        for (int k = 0; k < 4; k++) {
            ASSERT_EQ(at[k].size(), at[0].size());
            d[k] = std::hypot(at[k][i][0], at[k][i][2]);
        }
        EXPECT_NEAR((d[1] - d[2]) - (d[0] - d[1]), expanded::kBodySparkAccel, 1e-3f);
        EXPECT_NEAR((d[2] - d[3]) - (d[1] - d[2]), expanded::kBodySparkAccel, 1e-3f);
    }
}

TEST_F(UnderworldSparks, SatansAxSparksAllStartOnTheGround) {
    Acting(14);
    Steps(16);
    ASSERT_GT(expanded::UnderworldSparksLive(), 0);
    EXPECT_TRUE(Risen().empty());
}

TEST_F(UnderworldSparksWithInferno, HalfItsGroundSparksStartUpToTwiceHisHeight) {
    Acting(14);
    Steps(16);
    const int                         live = expanded::UnderworldSparksLive();
    std::vector<std::array<float, 3>> risen = Risen();
    ASSERT_EQ(live, static_cast<int>(16 * expanded::kSparkSmashPerFrame));
    EXPECT_EQ(static_cast<int>(risen.size()), live / 2);
    const float top = CharaHeight(UserStatus);
    float       lowest = 2.0f * top, highest = 0.0f;
    for (const auto &at : risen) {
        EXPECT_LE(at[1], 2.0f * top);
        lowest = std::min(lowest, at[1]);
        highest = std::max(highest, at[1]);
    }
    EXPECT_LT(lowest, top);
    EXPECT_GT(highest, top);
}

TEST(InfernoSparkHeight, ThreeQuartersOfItsOwnWithinHisHeightAndUpToHisTopAtTwice) {
    const float top = 20.0f;
    EXPECT_FLOAT_EQ(expanded::InfernoSparkHeight(2.0f * top, top), top);
    EXPECT_FLOAT_EQ(expanded::InfernoSparkHeight(top, top), 0.75f * top);
    EXPECT_FLOAT_EQ(expanded::InfernoSparkHeight(0.75f * top, top), 0.5625f * top);
    EXPECT_FLOAT_EQ(expanded::InfernoSparkHeight(1.5f * top, top), 0.875f * top);
    EXPECT_FLOAT_EQ(expanded::InfernoSparkHeight(3.0f * top, top), top);
    EXPECT_FLOAT_EQ(expanded::InfernoSparkHeight(0.0f, top), 0.0f);
}

TEST_F(UnderworldSparksWithInferno, ARisenSparkHeadsStraightForTheHeightItsOwnMapsTo) {
    Acting(14);
    std::vector<std::array<float, 3>> start = FirstSeen(16, [this] { Steps(1); });
    Steps(6);
    ExpectHeadingForWhereItsOwnMaps(start, All());
}

TEST_F(UnderworldSparksWithInferno, ABodysSparkHeadsForTheHeightItsOwnMapsTo) {
    // Its body at his height: it comes down toward three quarters of it.
    Place(55.0f, 0.0f, CharaHeight(UserStatus), 0.25f);
    Acting(14);
    std::vector<std::array<float, 3>> start = FirstSeen(4, [this] { PulledSteps(1); });
    floor_.unit().monster[0].knockback[2] = 0.0f;
    PulledSteps(3);
    ExpectHeadingForWhereItsOwnMaps(start, All());
}
