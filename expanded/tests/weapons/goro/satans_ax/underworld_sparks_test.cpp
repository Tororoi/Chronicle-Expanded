#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_sparks.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

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
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        expanded::SeedModRandom(7);
        expanded::UnderworldSparksReset();
    }

    void TearDown() override {
        expanded::UnderworldSparksReset();
        CharaMain.SetPosition(saved_pos_[0], saved_pos_[1], saved_pos_[2]);
        BtActStatus = {};
    }

    // action 13 his charge held, 14 his smash, anything else neither.
    void Acting(int action) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = action;
    }

    void Steps(int count) {
        for (int i = 0; i < count; i++) {
            expanded::UnderworldSparksStep();
        }
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
