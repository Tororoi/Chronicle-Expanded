#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/goro/last_judgement.hpp"
#include "expanded/weapons/player_state.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

// Goro out with weapon_no in his hand.
class JudgementHammer : public ::testing::Test {
protected:
    explicit JudgementHammer(int weapon_no = ITEM_WEAPON_LAST_JUDGEMENT) : wielding_(weapon_no) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        saved_weapon_ = NowWeapon;
        NowWeapon = &MainWeapon;
        expanded::JudgementHammerReset();
    }

    void TearDown() override {
        expanded::JudgementHammerReset();
        NowWeapon = saved_weapon_;
        BtActStatus = {};
    }

    // frames with the guard up (or down).
    void Guard(bool up, int frames) {
        BtActStatus.guard_mode = up ? expanded::kGuardUp : 0;
        for (int i = 0; i < frames; i++) {
            expanded::JudgementHammerStep();
        }
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  wielding_;
    CCharacter               *saved_weapon_ = nullptr;
};

class JudgementHammerWithInferno : public JudgementHammer {
protected:
    JudgementHammerWithInferno() : JudgementHammer(ITEM_WEAPON_INFERNO) {}
};

class JudgementHammerWithAnother : public JudgementHammer {
protected:
    JudgementHammerWithAnother() : JudgementHammer(ITEM_WEAPON_TALL_HAMMER) {}
};

constexpr int kGrow = expanded::kJudgementGrowFrames;

} // namespace

TEST_F(JudgementHammer, HoldingTheGuardGrowsItEvenlyToHalfAgainItsSizeOverThreeSeconds) {
    EXPECT_FLOAT_EQ(expanded::JudgementHammerScale(), 1.0f);
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::JudgementHammerScale(), 1.25f, 1e-4f);
    Guard(true, kGrow / 2);
    EXPECT_FLOAT_EQ(expanded::JudgementHammerScale(), 1.5f);
}

TEST_F(JudgementHammer, LetGoEarlyItShrinksBackAsLongAsItGrew) {
    Guard(true, kGrow / 3);
    float grown = expanded::JudgementHammerScale();
    EXPECT_GT(grown, 1.0f);
    Guard(false, kGrow / 3 - 1);
    EXPECT_GT(expanded::JudgementHammerScale(), 1.0f);
    Guard(false, 1);
    EXPECT_NEAR(expanded::JudgementHammerScale(), 1.0f, 1e-4f);
}

TEST_F(JudgementHammer, FullItKeepsItsSizeFifteenSecondsThenShrinksAndWaitsForAFreshHold) {
    Guard(true, kGrow);
    Guard(false, expanded::kJudgementFullFrames - 1);
    EXPECT_FLOAT_EQ(expanded::JudgementHammerScale(), expanded::kJudgementGrownScale);
    Guard(true, 2); // still held as it runs out: it shrinks all the same
    EXPECT_LT(expanded::JudgementHammerScale(), expanded::kJudgementGrownScale);
    Guard(true, kGrow);
    EXPECT_NEAR(expanded::JudgementHammerScale(), 1.0f, 1e-4f);
    Guard(false, 1);
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::JudgementHammerScale(), 1.25f, 1e-4f);
}

TEST_F(JudgementHammer, ItsSizeWidensItsHitsAndRaisesItsAttackToHalfAgain) {
    EXPECT_EQ(expanded::JudgementHammerAttack(100), 100);
    Guard(true, kGrow / 2);
    EXPECT_EQ(expanded::JudgementHammerAttack(100), 125);
    EXPECT_NEAR(expanded::JudgementHammerHitRadius(5.0f), 6.25f, 1e-3f);
    Guard(true, kGrow / 2);
    EXPECT_EQ(expanded::JudgementHammerAttack(100), 150);
    EXPECT_FLOAT_EQ(expanded::JudgementHammerHitRadius(5.0f), 7.5f);
}

TEST_F(JudgementHammer, ItsImpactIsItsSizeAndReachesAsFar) {
    Guard(true, kGrow);
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->active[2] = 1;
    expanded::JudgementHammerImpact(*effect, 2);
    EXPECT_FLOAT_EQ(effect->chara[2].scale[0], expanded::kJudgementGrownScale);
    COLLISION_HIT hit{};
    hit.radius = 15.0f;
    expanded::JudgementHammerHitSet(*effect, 2, hit);
    EXPECT_FLOAT_EQ(hit.radius, 22.5f);
    COLLISION_HIT other{};
    other.radius = 15.0f;
    expanded::JudgementHammerHitSet(*effect, 3, other);
    EXPECT_FLOAT_EQ(other.radius, 15.0f);
}

TEST_F(JudgementHammerWithAnother, OnlyLastJudgementGrows) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::JudgementHammerScale(), 1.0f);
    EXPECT_EQ(expanded::JudgementHammerAttack(100), 100);
}

TEST_F(JudgementHammerWithInferno, TheInfernoGrowsToo) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::JudgementHammerScale(), expanded::kJudgementGrownScale);
    EXPECT_EQ(expanded::JudgementHammerAttack(100), 150);
}

TEST_F(JudgementHammerWithInferno, ItsHiddenImpactStaysHiddenButReachesItsSizeAsFar) {
    Guard(true, kGrow);
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->active[2] = 1;
    effect->chara[2].SetScale(0.0f, 0.0f, 0.0f); // the Inferno's impact, behind its blast
    expanded::JudgementHammerImpact(*effect, 2);
    EXPECT_FLOAT_EQ(effect->chara[2].scale[0], 0.0f);
    COLLISION_HIT hit{};
    hit.radius = 20.0f;
    expanded::JudgementHammerHitSet(*effect, 2, hit);
    EXPECT_FLOAT_EQ(hit.radius, 20.0f * expanded::kJudgementGrownScale);
}
