#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/double_impact.hpp"
#include "expanded/weapons/xiao/hardshooter.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::TestFloor;
using expanded::test::Wielding;

// A floor with Xiao holding weapon_no (carrying sphere when given) and a pellet of 40 just fired into slot 2 along +z.
class DoubleImpact : public ::testing::Test {
protected:
    explicit DoubleImpact(int weapon_no = ITEM_WEAPON_DOUBLE_IMPACT, int sphere = 0) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        floor_.status().cur_chara = CHARA_XIAO;
        *shots_ = CSHOT{};
        shots_->used[2] = 1;
        shots_->pos[2][1] = 10.0f;
        shots_->pos[2][3] = 1.0f;
        shots_->vector[2][2] = 4.0f;
        shots_->damage[2] = 40;
        shots_->size[2] = 2.5f;
        shots_->life[2] = 60;
    }

    int Fire() {
        expanded::RicochetShotFired(2);
        return expanded::DoubleImpactFired(*shots_, 2);
    }

    // monster standing with one hurt sphere of radius 6 at (x, 10, z).
    void Stand(int monster, float x, float z) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = 1;
        live.hp = 10;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.position[0][0] = x;
        effect.position[0][1] = 10.0f;
        effect.position[0][2] = z;
        effect.position[0][3] = 1.0f;
        effect.radius[0] = 6.0f;
        effect.timer[0] = 1;
    }

    TestFloor              floor_;
    Wielding               wielding_;
    std::unique_ptr<CSHOT> shots_ = std::make_unique<CSHOT>();
};

class DoubleImpactWithAnotherWeapon : public DoubleImpact {
protected:
    DoubleImpactWithAnotherWeapon() : DoubleImpact(ITEM_WEAPON_HARDSHOOTER) {}
};

class DoubleImpactBySuperSteve : public DoubleImpact {
protected:
    DoubleImpactBySuperSteve() : DoubleImpact(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DOUBLE_IMPACT) {}
};

} // namespace

TEST_F(DoubleImpact, EveryShotIsTwoStonesSideBySideAtThreeQuartersEach) {
    int twin = Fire();

    ASSERT_EQ(twin, 0);
    EXPECT_EQ(shots_->used[twin], 1);
    EXPECT_EQ(shots_->damage[2], 30);
    EXPECT_EQ(shots_->damage[twin], 30);
    EXPECT_EQ(shots_->life[twin], 60);
    EXPECT_FLOAT_EQ(shots_->size[twin], 2.5f);
    EXPECT_FLOAT_EQ(shots_->vector[twin][2], 4.0f);
    // Across the flight (x), the width apart, level with each other and abreast.
    EXPECT_FLOAT_EQ(shots_->pos[2][0] - shots_->pos[twin][0], expanded::kDoubleImpactWidth);
    EXPECT_FLOAT_EQ(shots_->pos[2][1], shots_->pos[twin][1]);
    EXPECT_FLOAT_EQ(shots_->pos[2][2], shots_->pos[twin][2]);
    EXPECT_EQ(expanded::PelletWeapon(), ITEM_WEAPON_STEEL_SLINGSHOT);
}

TEST_F(DoubleImpact, ThePairRicochetsAtDifferentMonsters) {
    Stand(0, 0.0f, 50.0f);
    Stand(1, 40.0f, 50.0f);  // nearest the first
    Stand(2, -60.0f, 50.0f); // next
    int twin = Fire();
    ASSERT_GE(twin, 0);

    shots_->pos[2][2] = shots_->pos[twin][2] = 44.0f;
    int first = expanded::RicochetShotEnded(*shots_, 2, floor_.unit().effect[0].position[0], true);
    ASSERT_GE(first, 0);
    EXPECT_GT(shots_->vector[first][0], 0.0f); // at monster 1
    int second = expanded::RicochetShotEnded(*shots_, twin, floor_.unit().effect[0].position[0], true);
    ASSERT_GE(second, 0);
    EXPECT_LT(shots_->vector[second][0], 0.0f); // at monster 2
}

TEST_F(DoubleImpactWithAnotherWeapon, FiresOne) {
    EXPECT_EQ(Fire(), -1);
    EXPECT_EQ(shots_->damage[2], 40);
    EXPECT_EQ(shots_->used[0], 0);
}

TEST_F(DoubleImpactBySuperSteve, FiresTwoWithTheSphere) {
    EXPECT_GE(Fire(), 0);
    EXPECT_EQ(shots_->damage[2], 30);
    EXPECT_EQ(expanded::PelletWeapon(), ITEM_WEAPON_STEEL_SLINGSHOT);
}
