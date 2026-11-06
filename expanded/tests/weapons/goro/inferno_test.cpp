#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// Goro out with weapon_no (attack 300) at the origin, the dungeon playing; a monster of weight 1 may stand at (x, 0).
class BurningAura : public ::testing::Test {
protected:
    explicit BurningAura(int weapon_no = ITEM_WEAPON_INFERNO) : wielding_(weapon_no) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        wielding_.weapon().attack = 300;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
        }
        saved_time_ = CharaMain.motion_type.state.time;
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        saved_hits_ = NowColData;
        NowColData = hits_.get();
        *hits_ = CCollisionData{};
        expanded::UnderworldReset();
        expanded::BurningAuraReset();
    }

    void TearDown() override {
        expanded::UnderworldReset();
        expanded::BurningAuraReset();
        NowColData = saved_hits_;
        CharaMain.SetPosition(saved_pos_[0], saved_pos_[1], saved_pos_[2]);
        CharaMain.motion_type.state.time = saved_time_;
        BtActStatus = {};
    }

    // action 13 his charge held, 14 his smash (short of the ground), anything else neither.
    void Acting(int action) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = action;
        CharaMain.motion_type.state.time = expanded::kUnderworldSmashLands - 20.0f;
    }

    // A Skeleton Soldier in slot 0 at (x, 0, 0), standing still, of weight 1 (a new floor: placed before anything else).
    void Place(float x) {
        floor_.SetUpModel(0, Monster::SKELETON_SOLDIER);
        floor_.Spawn(0, 0);
        MONSTOR &live = floor_.unit().monster[0];
        live.state = 2;
        live.revealed = -1;
        live.knockback[2] = 1.0f;
        live.movement_speed = 0.0f;
        floor_.unit().chara[0][0].frame = nullptr;
        floor_.unit().chara[0][0].SetPosition(x, 0.0f, 0.0f);
    }

    // Steps of the pull on the monster (kept where it stands), then the aura.
    void Steps(int count) {
        for (int i = 0; i < count; i++) {
            expanded::UnderworldMonsterMove(floor_.unit(), 0);
            floor_.unit().monster[0].movement_speed = 0.0f;
            expanded::BurningAuraStep();
        }
    }

    // The aura's hits planted so far, their damage.
    std::vector<int> Burns() {
        std::vector<int> burns;
        for (int i = 0; i < kHitRecordsHere; i++) {
            const COLLISION_HIT &hit = NowColData->hit[i];
            if (NowColData->active[i] != 0 && (hit.weapon_flags & expanded::kBlastMark) != 0) {
                EXPECT_NE(hit.flags & expanded::kFireElementBit, 0);
                burns.push_back(hit.damage);
            }
        }
        return burns;
    }

    static constexpr int kHitRecordsHere = sizeof(CCollisionData::hit) / sizeof(CCollisionData::hit[0]);

    expanded::test::TestFloor       floor_;
    expanded::test::Wielding        wielding_;
    expanded::test::StepsUnpaused   unpaused_;
    std::unique_ptr<CCollisionData> hits_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits_ = nullptr;
    float                           saved_pos_[3] = {};
    float                           saved_time_ = 0.0f;
};

class BurningAuraWithSatansAx : public BurningAura {
protected:
    BurningAuraWithSatansAx() : BurningAura(ITEM_WEAPON_SATAN_S_AX) {}
};

COLLISION_HIT Hit(int flags) {
    COLLISION_HIT hit{};
    hit.flags = flags;
    hit.damage = 50;
    return hit;
}

} // namespace

TEST_F(BurningAura, TheInfernoPullsAsSatansAxDoes) {
    Place(50.0f);
    Acting(14);
    EXPECT_FLOAT_EQ(expanded::UnderworldStrength(), 1.0f);
    expanded::UnderworldMonsterMove(floor_.unit(), 0);
    EXPECT_TRUE(expanded::UnderworldPulling(0));
}

TEST_F(BurningAura, AFireHitPassesHimBy) {
    EXPECT_TRUE(expanded::InfernoFireproof(Hit(expanded::kFireElementBit)));
    EXPECT_FALSE(expanded::InfernoFireproof(Hit(0)));
    EXPECT_FALSE(expanded::InfernoFireproof(Hit(2))); // ice
}

TEST_F(BurningAuraWithSatansAx, OnlyTheInfernoIsFireproof) {
    EXPECT_FALSE(expanded::InfernoFireproof(Hit(expanded::kFireElementBit)));
}

TEST_F(BurningAura, EachSecondBurnsAPercentMoreThanTheLast) {
    Place(50.0f);
    Acting(13); // the hold counts too
    Steps(expanded::kBurningAuraFrames - 1);
    EXPECT_TRUE(Burns().empty());
    Steps(1);
    EXPECT_EQ(Burns(), std::vector<int>{3}); // 1% of 300
    *hits_ = CCollisionData{};
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_EQ(Burns(), std::vector<int>{6}); // 2%
    *hits_ = CCollisionData{};
    Steps(expanded::kBurningAuraFrames);
    EXPECT_EQ(Burns(), std::vector<int>{9}); // 3%: 6% in all over three seconds
    EXPECT_EQ(expanded::BurningAuraSeconds(), 3);
}

TEST_F(BurningAura, ItStartsOverWithTheNextPull) {
    Place(50.0f);
    Acting(14);
    Steps(2 * expanded::kBurningAuraFrames);
    Acting(0);
    Steps(1);
    EXPECT_EQ(expanded::BurningAuraSeconds(), 0);
    *hits_ = CCollisionData{};
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_EQ(Burns(), std::vector<int>{3});
}

TEST_F(BurningAura, AMonsterThePullDoesNotDrawIsNotBurned) {
    Place(expanded::kUnderworldReach + 20.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_FALSE(expanded::UnderworldPulling(0));
    EXPECT_TRUE(Burns().empty());
}

TEST_F(BurningAuraWithSatansAx, SatansAxDoesNotBurn) {
    Place(50.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFrames);
    EXPECT_TRUE(Burns().empty());
    EXPECT_FALSE(expanded::BurningAuraFeared(floor_.unit(), 0));
}

TEST_F(BurningAura, ADrawnMonsterIsAfraidUntilItComesNear) {
    Place(50.0f);
    Acting(14);
    Steps(1);
    EXPECT_TRUE(expanded::BurningAuraFeared(floor_.unit(), 0));
    floor_.unit().chara[0][0].SetPosition(expanded::kBurningAuraBrave - 1.0f, 0.0f, 0.0f);
    EXPECT_FALSE(expanded::BurningAuraFeared(floor_.unit(), 0));
    Acting(0);
    Steps(1);
    floor_.unit().chara[0][0].SetPosition(50.0f, 0.0f, 0.0f);
    EXPECT_FALSE(expanded::BurningAuraFeared(floor_.unit(), 0));
}

TEST_F(BurningAura, TheFogReddensOverThreeSecondsAndFadesAfter) {
    Place(50.0f);
    Acting(14);
    Steps(expanded::kBurningAuraFogFrames / 2);
    EXPECT_NEAR(expanded::BurningAuraRed(), 0.5f, 1e-3f);
    unsigned char fog[3] = {0x60, 0xA0, 0xEF};
    expanded::BurningAuraFog(fog);
    EXPECT_EQ(fog[0], (0x60 + 220) / 2);
    Steps(expanded::kBurningAuraFogFrames);
    EXPECT_FLOAT_EQ(expanded::BurningAuraRed(), 1.0f);
    unsigned char full[3] = {0x60, 0xA0, 0xEF};
    expanded::BurningAuraFog(full);
    EXPECT_EQ(full[0], 220);
    EXPECT_EQ(full[1], 40);
    EXPECT_EQ(full[2], 20);
    Acting(0);
    Steps(expanded::kBurningAuraFogFade - 1);
    EXPECT_GT(expanded::BurningAuraRed(), 0.0f);
    Steps(1);
    EXPECT_FLOAT_EQ(expanded::BurningAuraRed(), 0.0f);
}
