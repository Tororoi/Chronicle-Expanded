#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/goro/growing_hammer.hpp"
#include "expanded/weapons/player_state.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

// Goro out with weapon_no in his hand.
class GrowingHammer : public ::testing::Test {
protected:
    explicit GrowingHammer(int weapon_no = ITEM_WEAPON_LAST_JUDGEMENT) : wielding_(weapon_no) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        saved_weapon_ = NowWeapon;
        NowWeapon = &MainWeapon;
        expanded::GrowingHammerReset();
    }

    void TearDown() override {
        expanded::GrowingHammerReset();
        NowWeapon = saved_weapon_;
        BtActStatus = {};
    }

    // frames with the guard up (or down).
    void Guard(bool up, int frames) {
        BtActStatus.guard_mode = up ? expanded::kGuardUp : 0;
        for (int i = 0; i < frames; i++) {
            expanded::GrowingHammerStep();
        }
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  wielding_;
    CCharacter               *saved_weapon_ = nullptr;
};

class GrowingHammerWithInferno : public GrowingHammer {
protected:
    GrowingHammerWithInferno() : GrowingHammer(ITEM_WEAPON_INFERNO) {}
};

class GrowingHammerWithBigBucks : public GrowingHammer {
protected:
    GrowingHammerWithBigBucks() : GrowingHammer(ITEM_WEAPON_BIG_BUCKS_HAMMER) {}
};

class GrowingHammerWithMagical : public GrowingHammer {
protected:
    GrowingHammerWithMagical() : GrowingHammer(ITEM_WEAPON_MAGICAL_HAMMER) {}
};

class GrowingHammerWithGaia : public GrowingHammer {
protected:
    GrowingHammerWithGaia() : GrowingHammer(ITEM_WEAPON_GAIA_HAMMER) {}
};

class GrowingHammerWithAnother : public GrowingHammer {
protected:
    GrowingHammerWithAnother() : GrowingHammer(ITEM_WEAPON_TALL_HAMMER) {}
};

constexpr int kGrow = expanded::kGrowingHammerGrowFrames;

} // namespace

TEST_F(GrowingHammer, HoldingTheGuardGrowsLastJudgementEvenlyToHalfAgainItsSizeOverTwoSeconds) {
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.0f);
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::GrowingHammerScale(), 1.25f, 1e-4f);
    Guard(true, kGrow / 2);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.5f);
}

TEST_F(GrowingHammer, LetGoEarlyItShrinksBackAsLongAsItGrew) {
    Guard(true, kGrow / 3);
    float grown = expanded::GrowingHammerScale();
    EXPECT_GT(grown, 1.0f);
    Guard(false, kGrow / 3 - 1);
    EXPECT_GT(expanded::GrowingHammerScale(), 1.0f);
    Guard(false, 1);
    EXPECT_NEAR(expanded::GrowingHammerScale(), 1.0f, 1e-4f);
}

TEST_F(GrowingHammer, FullItKeepsItsSizeFifteenSecondsThenShrinksAndWaitsForAFreshHold) {
    Guard(true, kGrow);
    Guard(false, expanded::kGrowingHammerFullFrames - 1);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.5f);
    Guard(true, 2); // still held as it runs out: it shrinks all the same
    EXPECT_LT(expanded::GrowingHammerScale(), 1.5f);
    Guard(true, kGrow);
    EXPECT_NEAR(expanded::GrowingHammerScale(), 1.0f, 1e-4f);
    Guard(false, 1);
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::GrowingHammerScale(), 1.25f, 1e-4f);
}

TEST_F(GrowingHammer, ItsSizeWidensItsHitsAndRaisesItsAttackToHalfAgain) {
    EXPECT_EQ(expanded::GrowingHammerAttack(100), 100);
    Guard(true, kGrow / 2);
    EXPECT_EQ(expanded::GrowingHammerAttack(100), 125);
    EXPECT_NEAR(expanded::GrowingHammerHitRadius(5.0f), 6.25f, 1e-3f);
    Guard(true, kGrow / 2);
    EXPECT_EQ(expanded::GrowingHammerAttack(100), 150);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerHitRadius(5.0f), 7.5f);
}

TEST_F(GrowingHammer, ItsImpactIsItsSizeAndReachesAsFar) {
    Guard(true, kGrow);
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->active[2] = 1;
    expanded::GrowingHammerImpact(*effect, 2);
    EXPECT_FLOAT_EQ(effect->chara[2].scale[0], 1.5f);
    COLLISION_HIT hit{};
    hit.radius = 15.0f;
    expanded::GrowingHammerHitSet(*effect, 2, hit);
    EXPECT_FLOAT_EQ(hit.radius, 22.5f);
    COLLISION_HIT other{};
    other.radius = 15.0f;
    expanded::GrowingHammerHitSet(*effect, 3, other);
    EXPECT_FLOAT_EQ(other.radius, 15.0f);
}

TEST_F(GrowingHammerWithAnother, AHammerOutsideTheLineDoesNotGrow) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.0f);
    EXPECT_EQ(expanded::GrowingHammerAttack(100), 100);
}

TEST_F(GrowingHammerWithInferno, TheInfernoGrowsToo) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.5f);
    EXPECT_EQ(expanded::GrowingHammerAttack(100), 150);
}

TEST_F(GrowingHammerWithInferno, ItsHiddenImpactStaysHiddenButReachesItsSizeAsFar) {
    Guard(true, kGrow);
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->active[2] = 1;
    effect->chara[2].SetScale(0.0f, 0.0f, 0.0f); // the Inferno's impact, behind its blast
    expanded::GrowingHammerImpact(*effect, 2);
    EXPECT_FLOAT_EQ(effect->chara[2].scale[0], 0.0f);
    COLLISION_HIT hit{};
    hit.radius = 20.0f;
    expanded::GrowingHammerHitSet(*effect, 2, hit);
    EXPECT_FLOAT_EQ(hit.radius, 20.0f * 1.5f);
}

TEST_F(GrowingHammerWithBigBucks, BigBucksHammerGrowsToThreeTenthsMoreItsStrengthWithIt) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.3f);
    EXPECT_EQ(expanded::GrowingHammerAttack(100), 130);
    Guard(false, kGrow);
    EXPECT_NEAR(expanded::GrowingHammerScale(), 1.3f, 1e-4f); // full: kept
}

TEST_F(GrowingHammerWithMagical, MagicalHammerGrowsToFourTenthsMore) {
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::GrowingHammerScale(), 1.2f, 1e-4f);
    Guard(true, kGrow / 2);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.4f);
    EXPECT_EQ(expanded::GrowingHammerAttack(100), 140);
}

TEST_F(GrowingHammerWithGaia, GaiaHammerGrowsToHalfAgain) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::GrowingHammerScale(), 1.5f);
}
