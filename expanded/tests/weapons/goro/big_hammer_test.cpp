#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/goro/big_hammer.hpp"
#include "expanded/weapons/player_state.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

// Goro out with weapon_no in his hand.
class BigHammer : public ::testing::Test {
protected:
    explicit BigHammer(int weapon_no = ITEM_WEAPON_LAST_JUDGEMENT) : wielding_(weapon_no) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        saved_weapon_ = NowWeapon;
        NowWeapon = &MainWeapon;
        expanded::BigHammerReset();
    }

    void TearDown() override {
        expanded::BigHammerReset();
        NowWeapon = saved_weapon_;
        BtActStatus = {};
    }

    // frames with the guard up (or down).
    void Guard(bool up, int frames) {
        BtActStatus.guard_mode = up ? expanded::kGuardUp : 0;
        for (int i = 0; i < frames; i++) {
            expanded::BigHammerStep();
        }
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  wielding_;
    CCharacter               *saved_weapon_ = nullptr;
};

class BigHammerWithInferno : public BigHammer {
protected:
    BigHammerWithInferno() : BigHammer(ITEM_WEAPON_INFERNO) {}
};

class BigHammerWithBigBucks : public BigHammer {
protected:
    BigHammerWithBigBucks() : BigHammer(ITEM_WEAPON_BIG_BUCKS_HAMMER) {}
};

class BigHammerWithMagical : public BigHammer {
protected:
    BigHammerWithMagical() : BigHammer(ITEM_WEAPON_MAGICAL_HAMMER) {}
};

class BigHammerWithGaia : public BigHammer {
protected:
    BigHammerWithGaia() : BigHammer(ITEM_WEAPON_GAIA_HAMMER) {}
};

class BigHammerWithAnother : public BigHammer {
protected:
    BigHammerWithAnother() : BigHammer(ITEM_WEAPON_TALL_HAMMER) {}
};

constexpr int kGrow = expanded::kBigHammerGrowFrames;

} // namespace

TEST_F(BigHammer, HoldingTheGuardGrowsLastJudgementEvenlyToHalfAgainItsSizeOverTwoSeconds) {
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.0f);
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::BigHammerScale(), 1.25f, 1e-4f);
    Guard(true, kGrow / 2);
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.5f);
}

TEST_F(BigHammer, LetGoEarlyItShrinksBackAsLongAsItGrew) {
    Guard(true, kGrow / 3);
    float grown = expanded::BigHammerScale();
    EXPECT_GT(grown, 1.0f);
    Guard(false, kGrow / 3 - 1);
    EXPECT_GT(expanded::BigHammerScale(), 1.0f);
    Guard(false, 1);
    EXPECT_NEAR(expanded::BigHammerScale(), 1.0f, 1e-4f);
}

TEST_F(BigHammer, FullItKeepsItsSizeFifteenSecondsThenShrinksAndWaitsForAFreshHold) {
    Guard(true, kGrow);
    Guard(false, expanded::kBigHammerFullFrames - 1);
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.5f);
    Guard(true, 2); // still held as it runs out: it shrinks all the same
    EXPECT_LT(expanded::BigHammerScale(), 1.5f);
    Guard(true, kGrow);
    EXPECT_NEAR(expanded::BigHammerScale(), 1.0f, 1e-4f);
    Guard(false, 1);
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::BigHammerScale(), 1.25f, 1e-4f);
}

TEST_F(BigHammer, ItsSizeWidensItsHitsAndRaisesItsAttackToHalfAgain) {
    EXPECT_EQ(expanded::BigHammerAttack(100), 100);
    Guard(true, kGrow / 2);
    EXPECT_EQ(expanded::BigHammerAttack(100), 125);
    EXPECT_NEAR(expanded::BigHammerHitRadius(5.0f), 6.25f, 1e-3f);
    Guard(true, kGrow / 2);
    EXPECT_EQ(expanded::BigHammerAttack(100), 150);
    EXPECT_FLOAT_EQ(expanded::BigHammerHitRadius(5.0f), 7.5f);
}

TEST_F(BigHammer, ItsImpactIsItsSizeAndReachesAsFar) {
    Guard(true, kGrow);
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->active[2] = 1;
    expanded::BigHammerImpact(*effect, 2);
    EXPECT_FLOAT_EQ(effect->chara[2].scale[0], 1.5f);
    COLLISION_HIT hit{};
    hit.radius = 15.0f;
    expanded::BigHammerHitSet(*effect, 2, hit);
    EXPECT_FLOAT_EQ(hit.radius, 22.5f);
    COLLISION_HIT other{};
    other.radius = 15.0f;
    expanded::BigHammerHitSet(*effect, 3, other);
    EXPECT_FLOAT_EQ(other.radius, 15.0f);
}

TEST_F(BigHammerWithAnother, AHammerOutsideTheLineDoesNotGrow) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.0f);
    EXPECT_EQ(expanded::BigHammerAttack(100), 100);
}

TEST_F(BigHammerWithInferno, TheInfernoGrowsToo) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.5f);
    EXPECT_EQ(expanded::BigHammerAttack(100), 150);
}

TEST_F(BigHammerWithInferno, ItsHiddenImpactStaysHiddenButReachesItsSizeAsFar) {
    Guard(true, kGrow);
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->active[2] = 1;
    effect->chara[2].SetScale(0.0f, 0.0f, 0.0f); // the Inferno's impact, behind its blast
    expanded::BigHammerImpact(*effect, 2);
    EXPECT_FLOAT_EQ(effect->chara[2].scale[0], 0.0f);
    COLLISION_HIT hit{};
    hit.radius = 20.0f;
    expanded::BigHammerHitSet(*effect, 2, hit);
    EXPECT_FLOAT_EQ(hit.radius, 20.0f * 1.5f);
}

TEST_F(BigHammerWithBigBucks, BigBucksHammerGrowsToThreeTenthsMoreItsStrengthWithIt) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.3f);
    EXPECT_EQ(expanded::BigHammerAttack(100), 130);
    Guard(false, kGrow);
    EXPECT_NEAR(expanded::BigHammerScale(), 1.3f, 1e-4f); // full: kept
}

TEST_F(BigHammerWithMagical, MagicalHammerGrowsToFourTenthsMore) {
    Guard(true, kGrow / 2);
    EXPECT_NEAR(expanded::BigHammerScale(), 1.2f, 1e-4f);
    Guard(true, kGrow / 2);
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.4f);
    EXPECT_EQ(expanded::BigHammerAttack(100), 140);
}

TEST_F(BigHammerWithGaia, GaiaHammerGrowsToHalfAgain) {
    Guard(true, kGrow);
    EXPECT_FLOAT_EQ(expanded::BigHammerScale(), 1.5f);
}

// The game's charge-complete flash playing, and every one played out.
static bool Flashing() {
    float colour[4];
    return unitAmbientAnime(colour) != 0;
}

static void FlashDone() {
    for (int i = 0; i < 1000 && Flashing(); i++) {
    }
}

TEST_F(BigHammer, ItFlashesAsTheChargeDoesWhenItReachesFullSize) {
    FlashDone();
    Guard(true, kGrow - 1);
    EXPECT_FALSE(Flashing());
    Guard(true, 1);
    EXPECT_TRUE(Flashing());
    FlashDone();
    Guard(true, 30); // full and kept: once
    EXPECT_FALSE(Flashing());
}
