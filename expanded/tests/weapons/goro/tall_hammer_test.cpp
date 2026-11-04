#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

using expanded::test::OptionScope;

namespace {

// Goro out with weapon_no (attack 100) at the origin, the dungeon playing.
class Colossus : public ::testing::Test {
protected:
    explicit Colossus(int weapon_no = ITEM_WEAPON_TALL_HAMMER) : wielding_(weapon_no) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_GORO;
        wielding_.weapon().attack = 100;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        saved_data_ = NowColData;
        NowColData = data_.get();
        expanded::ColossusReset();
    }

    void TearDown() override {
        expanded::ColossusReset();
        NowColData = saved_data_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        BtActStatus = {};
    }

    // The charge held full (GoroKey_Play's action 13 past step 45), or let go.
    void Charge(bool held) {
        BtActStatus.action_on = held ? 1 : 0;
        BtActStatus.action_no = held ? 13 : 0;
        BtActStatus.action_step = held ? 46 : 0;
    }

    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::ColossusStep();
        }
    }

    void Grow(int frames) {
        Charge(true);
        Steps(frames);
        Charge(false);
    }

    int Hits() {
        int hits = 0;
        for (int active : NowColData->active) {
            hits += active != 0 ? 1 : 0;
        }
        return hits;
    }

    expanded::test::TestFloor       floor_;
    expanded::test::Wielding        wielding_;
    expanded::test::StepsUnpaused   unpaused_;
    std::unique_ptr<CCollisionData> data_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_data_ = nullptr;
    float                           saved_pos_[3] = {};
};

class ColossusWithAnotherHammer : public Colossus {
protected:
    ColossusWithAnotherHammer() : Colossus(ITEM_WEAPON_GAIA_HAMMER) {}
};

} // namespace

TEST_F(Colossus, AFullChargeGrowsHimToTwiceHisSizeInFiveSeconds) {
    Steps(10);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), 1.0f); // not charging
    BtActStatus.action_on = 1, BtActStatus.action_no = 13, BtActStatus.action_step = 30;
    Steps(10);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), 1.0f); // charging, not yet full
    Grow(expanded::kColossusGrowFrames / 2);
    EXPECT_NEAR(expanded::ColossusScale(), 1.5f, 1e-4f);
    EXPECT_FLOAT_EQ(CharaMain.scale[1], expanded::ColossusScale());
    Steps(60);
    EXPECT_NEAR(expanded::ColossusScale(), 1.5f, 1e-4f); // let go, he keeps it
    Grow(expanded::kColossusGrowFrames);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), expanded::kColossusMost);
}

TEST_F(Colossus, HeShrinksThirtySecondsAfterHeFirstGrewThenPopsBack) {
    Grow(60);
    Steps(expanded::kColossusFrames - 60 - 1);
    EXPECT_GT(expanded::ColossusScale(), 1.0f);
    Steps(1 + expanded::kColossusShrinkFrames / 2);
    float halfway = expanded::ColossusScale();
    EXPECT_LT(halfway, 1.2f);
    EXPECT_GT(halfway, expanded::kColossusLeast);
    Steps(expanded::kColossusShrinkFrames);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), expanded::kColossusLeast);
    // Spent: charging grows nothing until the pop.
    Grow(300);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), expanded::kColossusLeast);
    Steps(expanded::kColossusSpentFrames - expanded::kColossusShrinkFrames / 2 - expanded::kColossusShrinkFrames - 300);
    // The pop: past his size, then settled on it.
    float most = 0.0f;
    for (int frame = 0; frame < 30; frame++) {
        Steps(1);
        most = std::max(most, expanded::ColossusScale());
    }
    EXPECT_NEAR(most, expanded::kColossusPopPeak, 0.02f);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), 1.0f);
    Grow(30); // and he may grow again
    EXPECT_GT(expanded::ColossusScale(), 1.0f);
}

TEST_F(Colossus, PausedFramesDoNotCount) {
    Grow(30);
    float scale = expanded::ColossusScale();
    BtEventMode = 1;
    Charge(true);
    Steps(expanded::kColossusFrames * 2);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), scale);
}

TEST_F(Colossus, HisSizeScalesHisAttackReachThrowAndBody) {
    Grow(expanded::kColossusGrowFrames);
    EXPECT_EQ(expanded::ColossusAttack(100), 200);
    EXPECT_FLOAT_EQ(expanded::ColossusHitRadius(5.0f), 10.0f);
    // Twice the throw: speed^2 / (2 decay) carries twice as far.
    float speed = expanded::ColossusKickSpeed(2.5f);
    EXPECT_FLOAT_EQ(speed * speed, 2.0f * 2.5f * 2.5f);
    EXPECT_FLOAT_EQ(expanded::PlayerHurtHeight(16.0f), 32.0f);
    EXPECT_FLOAT_EQ(expanded::PlayerHurtReach(), expanded::kColossusBodyRadius);
    EXPECT_FLOAT_EQ(expanded::MonsterBlockRadius(6.0f), 12.0f);
    float point[4] = {0.0f, 16.0f, 0.0f, 1.0f};
    expanded::ShotPlayerPoint(point);
    EXPECT_FLOAT_EQ(point[1], 32.0f);
}

TEST_F(Colossus, DefenseRisesFifteenAtTwiceAndFallsFifteenAtHalf) {
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 30), 30);
    Grow(expanded::kColossusGrowFrames / 2);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 30), 38); // 1.5 times: half of 15, rounded
    Grow(expanded::kColossusGrowFrames);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 30), 45);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_TOAN, 30), 30);
    Steps(expanded::kColossusFrames + expanded::kColossusShrinkFrames);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 30), 15);
    EXPECT_EQ(expanded::ColossusAttack(100), 50);
}

TEST_F(Colossus, AtFullSizeAMonsterHeWalksIntoTakesAHammerHit) {
    MONSTOR &monster = floor_.unit().monster[0];
    monster.state = 2;
    monster.revealed = -1;
    monster.hp = monster.max_hp = 100;
    monster.body_radius = 4.0f;
    floor_.unit().chara[0][0].pos[2] = 15.0f; // within 12 + 4 + 2
    Grow(expanded::kColossusGrowFrames / 2);
    CharaMain.pos[2] += 1.0f;
    Steps(1);
    EXPECT_EQ(Hits(), 0); // only at full size
    Grow(expanded::kColossusGrowFrames);
    Steps(1);
    EXPECT_EQ(Hits(), 0); // standing still
    CharaMain.pos[2] += 1.0f;
    Steps(1);
    ASSERT_EQ(Hits(), 1);
    for (int i = 0; i < 96; i++) {
        if (NowColData->active[i] != 0) {
            EXPECT_EQ(NowColData->hit[i].damage, 50); // a quarter of a hammer hit at twice his attack
            EXPECT_FALSE(expanded::HitWearsWeapon(NowColData->hit[i]));
        }
    }
    CharaMain.pos[2] -= 1.0f;
    Steps(1);
    EXPECT_EQ(Hits(), 1); // once every kColossusTrampleFrames
}

TEST_F(Colossus, AtOneAndAHalfHeSetsOffHammerSwitchesHeStandsOn) {
    float under[3] = {3.0f, 0.0f, 0.0f};
    Grow(expanded::kColossusGrowFrames / 2 - 5);
    EXPECT_FALSE(expanded::HitSwitchSteppedOn(CHARA_GORO, under, 5.0f));
    Grow(10);
    EXPECT_TRUE(expanded::HitSwitchSteppedOn(CHARA_GORO, under, 5.0f));
    EXPECT_FALSE(expanded::HitSwitchSteppedOn(CHARA_XIAO, under, 5.0f));
    float away[3] = {8.0f, 0.0f, 0.0f};
    EXPECT_FALSE(expanded::HitSwitchSteppedOn(CHARA_GORO, away, 5.0f));
}

TEST_F(ColossusWithAnotherHammer, NoOtherHammerGrowsHim) {
    Grow(expanded::kColossusGrowFrames);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), 1.0f);
}

TEST_F(Colossus, OffWithTheWeaponAbilities) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Grow(expanded::kColossusGrowFrames);
    EXPECT_FLOAT_EQ(expanded::ColossusScale(), 1.0f);
}

TEST_F(Colossus, AnotherMemberIsDrawnAtHisOwnSize) {
    Grow(expanded::kColossusGrowFrames);
    floor_.status().cur_chara = CHARA_TOAN;
    Steps(1);
    EXPECT_FLOAT_EQ(CharaMain.scale[1], 1.0f);
    EXPECT_FLOAT_EQ(expanded::PlayerHurtHeight(16.0f), 16.0f);
    floor_.status().cur_chara = CHARA_GORO;
    Steps(1);
    EXPECT_FLOAT_EQ(CharaMain.scale[1], expanded::kColossusMost);
}

TEST_F(Colossus, AtTwiceHisSizeMonstersHitsLandATierLighter) {
    COLLISION_HIT heavy = {}, regular = {}, light = {};
    heavy.kind = expanded::kGuardBreakingHit;
    regular.kind = expanded::kKnockbackHit;
    light.kind = expanded::kLightHit;
    Grow(expanded::kColossusGrowFrames - 1);
    for (const COLLISION_HIT *hit : {&heavy, &regular, &light}) {
        EXPECT_EQ(expanded::PlayerHitKind(*hit), hit->kind); // not yet at his full size
        EXPECT_FALSE(expanded::PlayerHitShrugged(*hit));
    }
    Grow(1);
    EXPECT_EQ(expanded::PlayerHitKind(heavy), expanded::kKnockbackHit);
    EXPECT_EQ(expanded::PlayerHitKind(regular), expanded::kLightHit);
    EXPECT_FALSE(expanded::PlayerHitShrugged(heavy));
    EXPECT_FALSE(expanded::PlayerHitShrugged(regular));
    EXPECT_TRUE(expanded::PlayerHitShrugged(light));
    floor_.status().cur_chara = CHARA_TOAN;
    EXPECT_EQ(expanded::PlayerHitKind(heavy), expanded::kGuardBreakingHit);
    EXPECT_FALSE(expanded::PlayerHitShrugged(light));
}

TEST_F(Colossus, AShruggedOffHitLeavesHimInWhatHeWasDoing) {
    Grow(expanded::kColossusGrowFrames);
    Charge(true);
    COLLISION_HIT light = {};
    light.kind = expanded::kLightHit;
    ASSERT_TRUE(expanded::PlayerHitShrugged(light));
    BtActStatus.action_on = 0; // the game takes him out of his action
    BtActStatus.action_gauge = 100.0f;
    expanded::PlayerHitReacted(true, light);
    EXPECT_EQ(BtActStatus.action_on, 1);
    EXPECT_EQ(BtActStatus.action_no, 13);
    EXPECT_EQ(BtActStatus.action_step, 46);
    // A hit that lands takes him out of it as the game does.
    COLLISION_HIT regular = {};
    regular.kind = expanded::kKnockbackHit;
    ASSERT_FALSE(expanded::PlayerHitShrugged(regular));
    BtActStatus.action_on = 0;
    expanded::PlayerHitReacted(false, regular);
    EXPECT_EQ(BtActStatus.action_on, 0);
}

TEST_F(Colossus, AtFullSizeAMonstersAttackHitsHimOnce) {
    Grow(expanded::kColossusGrowFrames);
    COLLISION_HIT &bite = NowColData->hit[3];
    bite = {};
    bite.kind = expanded::kGuardBreakingHit;
    bite.owner = 200 + 2 * 5; // monster 2's
    EXPECT_FALSE(expanded::PlayerHitPassesBy(bite));
    expanded::PlayerHitReacted(false, bite);
    // The same monster's next hits pass him by for as long as the knockdown would have kept him from them.
    COLLISION_HIT &again = NowColData->hit[7];
    again = bite;
    EXPECT_TRUE(expanded::PlayerHitPassesBy(again));
    COLLISION_HIT &other = NowColData->hit[8];
    other = bite;
    other.owner = 200 + 3 * 5;
    EXPECT_FALSE(expanded::PlayerHitPassesBy(other));
    Steps(0xA0 - 1);
    EXPECT_TRUE(expanded::PlayerHitPassesBy(again));
    Steps(1);
    EXPECT_FALSE(expanded::PlayerHitPassesBy(again));
}

TEST_F(Colossus, AShotThatHitsHimAtFullSizeStopsAsAtAWall) {
    Grow(expanded::kColossusGrowFrames);
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->phase[1] = 1; // in flight
    effect->life_time[1] = -1;
    COLLISION_HIT &hit = NowColData->hit[4];
    hit = {};
    hit.kind = expanded::kKnockbackHit;
    expanded::ShotEffectHitSet(*effect, 1, hit);
    expanded::PlayerHitReacted(false, hit);
    EXPECT_EQ(effect->life_time[1], 0); // it hits nothing more
    EXPECT_TRUE(expanded::PlayerHitPassesBy(hit));
    EXPECT_EQ(expanded::ColossusShotCollision(*effect, 1, SHOT_COLLISION_NONE), SHOT_COLLISION_MAP);
    EXPECT_EQ(expanded::ColossusShotCollision(*effect, 1, SHOT_COLLISION_NONE), SHOT_COLLISION_NONE); // once
}

TEST_F(Colossus, BelowFullSizeHitsLandAsTheyDo) {
    Grow(expanded::kColossusGrowFrames - 1);
    COLLISION_HIT &bite = NowColData->hit[3];
    bite = {};
    bite.kind = expanded::kGuardBreakingHit;
    bite.owner = 200;
    expanded::PlayerHitReacted(false, bite);
    EXPECT_FALSE(expanded::PlayerHitPassesBy(bite));
}
