#include <gtest/gtest.h>

#include <cmath>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/weapons/stats.hpp"
#include "expanded/weapons/toan/lambs_sword.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// A Werewolf (slot 0), a White Fang (1) and a Mummy (2) in play, the player at the origin, the Lamb's Sword in hand
// at whp of its 100.
class Pack : public ::testing::Test {
protected:
    void SetUp() override {
        for (int i = 0; i < 3; i++) {
            saved_player_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        expanded::ConfuseFloorReset(false);
        const Monster kinds[] = {Monster::WEREWOLF, Monster::WHITE_FANG, Monster::MUMMY};
        for (int slot = 0; slot < 3; slot++) {
            floor_.SetUpModel(slot, kinds[slot]);
            floor_.Spawn(slot, slot);
            MONSTOR &live = floor_.unit().monster[slot];
            live.state = 2;
            live.revealed = -1;
            live.hp = 100;
        }
        floor_.status().cur_chara = CHARA_TOAN;
        Place(0, 0.0f, 100.0f);
        Place(1, 50.0f, 100.0f);
        Place(2, 0.0f, 200.0f);
        Whp(40.0f);
    }

    void TearDown() override {
        expanded::ConfuseFloorReset(false);
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_player_[i];
        }
    }

    void Place(int slot, float x, float z) {
        float at[4] = {x, 0.0f, z, 1.0f};
        floor_.unit().chara[slot][0].SetPosition(at);
    }

    void Whp(float whp) {
        sword_.weapon().durability = 100;
        sword_.weapon().durability_f = whp;
    }

    // A blow of the monster in slot.
    static COLLISION_HIT Blow(int slot) {
        COLLISION_HIT hit = {};
        hit.owner = static_cast<s16>(200 + slot * 5);
        hit.monster_no = -1;
        return hit;
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  sword_{ITEM_WEAPON_LAMB_S_SWORD};
    float                     saved_player_[3] = {};
};

} // namespace

TEST_F(Pack, TheFormFollowsTheWhp) {
    EXPECT_TRUE(expanded::PackLeaderWolf());
    Whp(100.0f * expanded::kLambSwordThreshold);
    EXPECT_TRUE(expanded::PackLeaderWolf()); // at the threshold: wolf
    Whp(51.0f);
    EXPECT_TRUE(expanded::PackLeaderLamb());
    EXPECT_FALSE(expanded::PackWolf(floor_.unit(), 0));
}

TEST_F(Pack, WerewolvesAndWhiteFangsRunWithThePlayer) {
    EXPECT_TRUE(expanded::PackWolf(floor_.unit(), 0));
    EXPECT_TRUE(expanded::PackWolf(floor_.unit(), 1));
    EXPECT_FALSE(expanded::PackWolf(floor_.unit(), 2));
    EXPECT_TRUE(expanded::confuse::Turned(floor_.unit(), 0));
    EXPECT_FALSE(expanded::confuse::Confused(floor_.unit(), 0)); // no stars
    expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
    EXPECT_FALSE(expanded::PackWolf(floor_.unit(), 0));
}

TEST_F(Pack, AWolfGoesForTheNearestOtherMonsterNotAWolf) {
    float at[4];
    ASSERT_TRUE(expanded::PackTarget(floor_.unit(), 0, at));
    EXPECT_FLOAT_EQ(at[0], 0.0f); // the Mummy, though the White Fang is nearer
    EXPECT_FLOAT_EQ(at[2], 200.0f);
}

TEST_F(Pack, WithNoneInReachItsScriptWalksOnPastThePlayer) {
    Place(2, 0.0f, 100.0f + expanded::kPackReach + 10.0f);
    EXPECT_EQ(expanded::PackPrey(floor_.unit(), 0), -1);
    float at[4];
    ASSERT_TRUE(expanded::PackTarget(floor_.unit(), 0, at));
    EXPECT_FLOAT_EQ(at[0], 0.0f);
    EXPECT_FLOAT_EQ(at[2], 100.0f - expanded::kPackWalkOn * 100.0f); // far past the player on its way to him
    float mummy[4];
    EXPECT_FALSE(expanded::PackTarget(floor_.unit(), 2, mummy));
}

// With none in reach, each wolf has its own place behind the player that its script heads for, and waits there.
TEST_F(Pack, WithNoneInReachTheWolvesHeadForTheirOwnPlaces) {
    Place(2, 0.0f, 100.0f + expanded::kPackReach + 10.0f);
    expanded::PackStep(0);
    ASSERT_NE(expanded::PackPlace(0), nullptr);
    ASSERT_NE(expanded::PackPlace(1), nullptr);
    const float a[2] = {expanded::PackPlace(0)[0], expanded::PackPlace(0)[1]};
    const float b[2] = {expanded::PackPlace(1)[0], expanded::PackPlace(1)[1]};
    EXPECT_EQ(expanded::PackPlace(2), nullptr);
    EXPECT_NEAR(std::hypot(a[0], a[1]), expanded::kPackHeel, 0.01f);
    EXPECT_NEAR(std::hypot(b[0], b[1]), expanded::kPackHeel, 0.01f);
    EXPECT_GT(std::hypot(a[0] - b[0], a[1] - b[1]), 20.0f); // apart
    EXPECT_FALSE(expanded::PackParked(0));                  // still on its way: its script walks it
    float at[4];
    ASSERT_TRUE(expanded::PackTarget(floor_.unit(), 0, at));
    EXPECT_NEAR(at[0], 0.0f + expanded::kPackWalkOn * a[0], 0.01f); // far past its place
    EXPECT_NEAR(at[2], 100.0f + expanded::kPackWalkOn * (a[1] - 100.0f), 0.01f);

    // There, its script waits and it idles.
    Place(0, a[0], a[1]);
    floor_.unit().chara[0][0].motion_no = 1;
    expanded::PackStep(0);
    EXPECT_TRUE(expanded::PackParked(0));
    EXPECT_EQ(floor_.unit().chara[0][0].motion_no, 0);
    EXPECT_EQ(floor_.unit().monster[0].movement_speed, 0.0f);

    // The player moves on: its script goes on.
    CharaMain.pos[0] = expanded::kPackRestart + 20.0f;
    expanded::PackStep(0);
    EXPECT_FALSE(expanded::PackParked(0));

    // A monster in reach sets it going after it.
    CharaMain.pos[0] = 0.0f;
    Place(0, a[0], a[1]);
    expanded::PackStep(0);
    ASSERT_TRUE(expanded::PackParked(0));
    Place(2, 0.0f, 100.0f);
    expanded::PackStep(0);
    EXPECT_FALSE(expanded::PackParked(0));
    EXPECT_EQ(expanded::PackPlace(0), nullptr);
}

TEST_F(Pack, AWolfsBlowPassesThePlayerBy) {
    EXPECT_TRUE(expanded::PackHitPassesBy(Blow(0)));
    EXPECT_FALSE(expanded::PackHitPassesBy(Blow(2)));
    Whp(80.0f);
    EXPECT_FALSE(expanded::PackHitPassesBy(Blow(0))); // lamb form: the wolves are the floor's again
}

// Super Steve carrying a Lamb's Sword SynthSphere turns as the sword does, by his own WHP (Wolf Steve).
TEST(LambsSword, SuperSteveCarryingItsSphereTurnsByHisWhp) {
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_LAMB_S_SWORD);
    steve.weapon().durability = 60;
    steve.weapon().durability_f = 30.0f;
    EXPECT_TRUE(expanded::LambWeapon(steve.weapon()));
    EXPECT_TRUE(expanded::LambWolfForm(steve.weapon()));
    EXPECT_TRUE(expanded::PackLeaderWolf());
    steve.weapon().durability_f = 31.0f;
    EXPECT_FALSE(expanded::LambWolfForm(steve.weapon()));
    expanded::test::Wielding plain(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
    EXPECT_FALSE(expanded::LambWeapon(plain.weapon()));
}
