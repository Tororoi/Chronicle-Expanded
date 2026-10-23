#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/status/blind.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

namespace {

using expanded::Monster;

// A floor of monsters of kind in play at slots 0.., each with the motions a blinding plays.
struct Floor {
    expanded::test::TestFloor floor;
    MOTION_INFO               motions[12] = {};

    explicit Floor(Monster kind, int monsters = 1) {
        floor.SetUpModel(0, kind);
        for (int i = 0; i < 12; i++) {
            motions[i] = {i * 20, i * 20 + 10, 0.5f, 0}; // 20 frames each at their own speed
        }
        for (int monster = 0; monster < monsters; monster++) {
            floor.Spawn(monster, 0);
            unit().monster[monster].revealed = -1;
            unit().chara[monster][0].motion_type.motion_info = motions;
        }
    }

    ~Floor() { expanded::BlindEnd(); }

    CMonstorUnit &unit() { return floor.unit(); }
};

int Requested(CMonstorUnit &unit, int monster) {
    return unit.monster[monster].requested_motion;
}

} // namespace

TEST(Blind, MostKindsHoldTheirGuardAndTheRestTheirOwn) {
    expanded::BlindClips clips;
    ASSERT_TRUE(expanded::BlindClipsOf(expanded::No(Monster::MASTER_JACKET), &clips));
    EXPECT_EQ(clips.hold, 6);
    EXPECT_EQ(clips.lower, 7);
    EXPECT_EQ(clips.stagger, 8);
    ASSERT_TRUE(expanded::BlindClipsOf(expanded::No(Monster::CANNIBAL_PLANT), &clips));
    EXPECT_EQ(clips.hold, 0);
    EXPECT_EQ(clips.lower, -1);
    EXPECT_EQ(clips.stagger, 1);
    EXPECT_FALSE(expanded::BlindClipsOf(expanded::No(Monster::BOMB_GEMRON), &clips));
}

TEST(Blind, HoldsTheGuardLowersItBeforeTheEndAndGivesTheScriptBack) {
    Floor         floor(Monster::MASTER_JACKET);
    CMonstorUnit &unit = floor.unit();
    unit.script_state[0] = 1;
    unit.monster[0].movement_speed = 2.0f;

    expanded::Blind(unit, 100);
    EXPECT_TRUE(expanded::Blinded(0));
    EXPECT_EQ(unit.script_state[0], 0);
    EXPECT_EQ(Requested(unit, 0), 6);
    EXPECT_FALSE(expanded::MonsterGuardBlocks(unit, 0, COLLISION_HIT{}));
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.monster[0].movement_speed, 0.0f);

    // The lowering clip runs 20 frames, and a beat after it before the end.
    const int lower_at = 20 + expanded::kBlindIdleBeat;
    while (expanded::BlindFramesLeft() > lower_at) {
        expanded::MonsterStepTaken(unit, 0);
        EXPECT_EQ(Requested(unit, 0), 6);
        expanded::BlindTick();
    }
    expanded::MonsterStepTaken(unit, 0);
    EXPECT_EQ(Requested(unit, 0), 7);

    while (expanded::Blinding()) {
        expanded::BlindTick();
    }
    EXPECT_FALSE(expanded::Blinded(0));
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_TRUE(expanded::MonsterGuardBlocks(unit, 0, COLLISION_HIT{}));
}

TEST(Blind, AHitStaggersThenTheGuardComesBack) {
    Floor         floor(Monster::MASTER_JACKET);
    CMonstorUnit &unit = floor.unit();
    expanded::Blind(unit, 200);

    unit.script_state[0] = 1; // CheckDmg ran the hit reaction
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 0);
    EXPECT_EQ(Requested(unit, 0), 8);
    for (int i = 1; i < expanded::kBlindStaggerFrames; i++) {
        expanded::MonsterStepTaken(unit, 0);
    }
    EXPECT_EQ(Requested(unit, 0), 6);
}

TEST(Blind, AKillGoesToItsDeathScript) {
    Floor         floor(Monster::MASTER_JACKET);
    CMonstorUnit &unit = floor.unit();
    expanded::Blind(unit, 200);
    unit.monster[0].hp = 0;
    unit.script_state[0] = 1;
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 1);
}

TEST(Blind, ANewFlashRaisesTheGuardAgain) {
    Floor         floor(Monster::MASTER_JACKET);
    CMonstorUnit &unit = floor.unit();
    expanded::Blind(unit, 10);
    expanded::MonsterStepTaken(unit, 0);
    EXPECT_EQ(Requested(unit, 0), 7);
    expanded::Blind(unit, 200);
    EXPECT_EQ(Requested(unit, 0), 6);
    EXPECT_EQ(expanded::BlindFramesLeft(), 200);
}

TEST(Blind, LeavesBossesAndChestsAlone) {
    Floor         floor(Monster::MASTER_JACKET, 2);
    CMonstorUnit &unit = floor.unit();
    unit.monster[1].revealed = 0;
    expanded::Blind(unit, 100);
    EXPECT_TRUE(expanded::Blinded(0));
    EXPECT_FALSE(expanded::Blinded(1));

    expanded::BlindEnd();
    unit.model[0].kind = MONSTER_KIND_NO_LOCK_ON;
    expanded::Blind(unit, 100);
    EXPECT_FALSE(expanded::Blinded(0));
}
