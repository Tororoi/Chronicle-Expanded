#include <gtest/gtest.h>

#include "dun/gameloop.hpp"
#include "expanded/status/confuse.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

namespace {

using expanded::test::TestFloor;

// Four monsters in play on a floor, the player at the origin; monster 0 confused for a minute.
class ConfuseTargets : public ::testing::Test {
protected:
    void SetUp() override {
        for (int i = 0; i < 3; i++) {
            saved_player_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        expanded::ConfuseFloorReset(false);
        for (int monster = 0; monster < 4; monster++) {
            MONSTOR &live = floor_.unit().monster[monster];
            live.state = 2;
            live.revealed = -1;
            live.hp = 100;
        }
        Place(0, 100.0f, 0.0f, 100.0f);
        Place(1, 100.0f, 40.0f, 130.0f); // 30 away across the ground, 40 above
        Place(2, 140.0f, 0.0f, 100.0f);  // 40 away
        Place(3, 100.0f, 0.0f, 200.0f);  // 100 away, nearer than the player (141)
        expanded::ConfuseMonster(floor_.unit(), 0, 3600);
    }

    void TearDown() override {
        expanded::ConfuseFloorReset(false);
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_player_[i];
        }
    }

    void Place(int monster, float x, float y, float z) {
        float at[4] = {x, y, z, 1.0f};
        floor_.unit().chara[monster][0].SetPosition(at);
    }

    // Where monster's script finds the player, and whether confusion set it.
    bool Target(int monster, float *at) { return expanded::ConfuseTarget(floor_.unit(), monster, at); }

    TestFloor floor_;
    float     saved_player_[3] = {};
};

} // namespace

TEST_F(ConfuseTargets, TheNearestAcrossTheGround) {
    float at[4];
    ASSERT_TRUE(Target(0, at));
    EXPECT_FLOAT_EQ(at[0], 100.0f); // monster 1, nearest across the ground though 40 above
    EXPECT_FLOAT_EQ(at[2], 130.0f);
    EXPECT_FLOAT_EQ(at[1], 40.0f);
}

TEST_F(ConfuseTargets, ATargetKilledIsPassedOverAtOnce) {
    float at[4];
    ASSERT_TRUE(Target(0, at));
    floor_.unit().monster[1].hp = 0; // killed, still playing its death
    ASSERT_TRUE(Target(0, at));
    EXPECT_FLOAT_EQ(at[0], 140.0f);      // the next nearest
    floor_.unit().monster[2].state = -1; // gone from the floor
    ASSERT_TRUE(Target(0, at));
    EXPECT_FLOAT_EQ(at[2], 200.0f);
}

TEST_F(ConfuseTargets, ThePlayerWhenNearerOrNoneIsLeft) {
    CharaMain.pos[0] = 100.0f, CharaMain.pos[2] = 90.0f; // 10 away
    float at[4];
    EXPECT_FALSE(Target(0, at)); // his own position
    CharaMain.pos[0] = 0.0f, CharaMain.pos[2] = 0.0f;
    for (int monster : {1, 2, 3}) {
        floor_.unit().monster[monster].hp = 0;
    }
    EXPECT_FALSE(Target(0, at));
}

TEST_F(ConfuseTargets, ALargeTargetIsMarkedWhereItsBodyIsReached) {
    // Monster 1 a large body: one sphere of radius 20 centred on it; the mark is 8 inside it, toward monster 0.
    MONSTOR_EFFECT_STATE &body = floor_.unit().effect[1];
    body.timer[0] = 1;
    body.position[0][0] = 100.0f, body.position[0][1] = 50.0f, body.position[0][2] = 130.0f, body.radius[0] = 20.0f;
    float at[4];
    ASSERT_TRUE(Target(0, at));
    EXPECT_FLOAT_EQ(at[0], 100.0f);
    EXPECT_FLOAT_EQ(at[2], 118.0f);
    EXPECT_FLOAT_EQ(at[1], 40.0f); // at its root's height
}

TEST_F(ConfuseTargets, AnUnconfusedMonsterFindsThePlayer) {
    float at[4];
    EXPECT_FALSE(Target(2, at));
}
