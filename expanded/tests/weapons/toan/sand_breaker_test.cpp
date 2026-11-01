#include <gtest/gtest.h>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/toan/sand_breaker.hpp"
#include "expanded/weapons/toan/small_sword.hpp"
#include "expanded/weapons/toan/swings.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

constexpr int kGuarding = 6;

// Toan guarding a Werewolf's blows (slot 0), the Sand Breaker in hand unless another weapon is given.
struct Guard {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding;

    explicit Guard(int weapon_no = ITEM_WEAPON_SAND_BREAKER) : wielding(weapon_no) {
        floor.SetUpModel(0, Monster::WEREWOLF);
        floor.Spawn(0, 0);
        floor.unit().monster[0].knockback[2] = 1.0f;
        floor.status().cur_chara = CHARA_TOAN;
        BtActStatus = {};
        BtActStatus.action_on = kGuarding;
        BtActStatus.action_no = 9;
        while (expanded::SandCounterReady() > 0 || expanded::SandCountering()) {
            BtActStatus.action_on = 0;
            expanded::SandCounterStep();
            BtActStatus.action_on = kGuarding;
        }
    }

    ~Guard() { BtActStatus = {}; }

    // The Werewolf's blow of kind meeting the guard as the game reacts to it: the kind it reaches Toan as.
    int Blow(int kind) {
        COLLISION_HIT hit = {};
        hit.kind = kind;
        hit.owner = 200;
        hit.monster_no = -1;
        // Any hit takes him out of his action before the game reacts to its kind.
        bool guarding = BtActStatus.action_on == kGuarding;
        BtActStatus.action_on = 0;
        expanded::PlayerHitReached(hit);
        expanded::PlayerHitReacted(guarding && (hit.kind == 2 || hit.kind == 4), hit);
        return hit.kind;
    }
};

} // namespace

TEST(SandBreaker, XInTheGuardCatchesTheNextBlowAndSwingsAtOnce) {
    Guard guard;
    expanded::SandCounterPress();
    EXPECT_EQ(expanded::SandCounterReady(), expanded::kCounterWindow);
    expanded::SandCounterStep();

    EXPECT_EQ(guard.Blow(3), 2); // a guard-breaking blow is blocked
    EXPECT_EQ(BtActStatus.action_on, 1);
    EXPECT_EQ(BtActStatus.action_no, expanded::kToanFirstSwing);
    CharaMain.motion_type.state.time = 821.0f; // past the step-in, as the swing's motion starts
    expanded::QuickDrawSwing(CharaMain);
    EXPECT_FLOAT_EQ(CharaMain.motion_type.state.time, expanded::kToanFirstSwingTrail);
    EXPECT_TRUE(expanded::SandCountering());
    EXPECT_EQ(expanded::SandCounterAttack(40), 80);
    EXPECT_EQ(expanded::SandCounterReady(), 0);

    const MONSTOR &wolf = guard.floor.unit().monster[0];
    EXPECT_FLOAT_EQ(wolf.knockback[0], expanded::kCounterKick.speed);
    EXPECT_FLOAT_EQ(wolf.knockback[1], expanded::kCounterKick.decay);

    // The swing over, his hits are his own again.
    BtActStatus.action_on = 0;
    expanded::SandCounterStep();
    EXPECT_FALSE(expanded::SandCountering());
    EXPECT_EQ(expanded::SandCounterAttack(40), 40);
}

TEST(SandBreaker, TheWindowIsATenthOfASecond) {
    Guard guard;
    expanded::SandCounterPress();
    for (int frame = 0; frame < expanded::kCounterWindow - 1; frame++) {
        expanded::SandCounterStep();
        EXPECT_EQ(BtActStatus.action_on, kGuarding);
    }
    // No blow caught: the guard let go for an ordinary swing, from its wind-up.
    expanded::SandCounterStep();
    EXPECT_EQ(BtActStatus.action_on, 1);
    EXPECT_EQ(BtActStatus.action_no, expanded::kToanFirstSwing);
    EXPECT_FALSE(expanded::SandCountering());
    EXPECT_EQ(expanded::SandCounterAttack(40), 40);
    EXPECT_EQ(guard.Blow(3), 3); // a blow now is the game's own: no counter readied
}

TEST(SandBreaker, OnlyInTheGuardAndInHand) {
    {
        Guard guard;
        BtActStatus.action_on = 0;
        expanded::SandCounterPress();
        EXPECT_EQ(expanded::SandCounterReady(), 0); // X out of the guard is the swing it always was
    }
    Guard guard(ITEM_WEAPON_DAGGER);
    expanded::SandCounterPress();
    EXPECT_EQ(expanded::SandCounterReady(), 0);
    EXPECT_EQ(guard.Blow(3), 3);
}

TEST(SandBreaker, SuperStevesCounterIsAShotAtOnce) {
    Guard                    guard(ITEM_WEAPON_SUPER_STEVE);
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SAND_BREAKER);
    guard.floor.status().cur_chara = CHARA_XIAO;
    expanded::SandCounterPress();
    EXPECT_EQ(expanded::SandCounterReady(), expanded::kCounterWindow);
    BtActStatus.action_gauge = 100.0f;
    EXPECT_EQ(guard.Blow(3), 2);
    EXPECT_EQ(BtActStatus.action_on, 1);
    EXPECT_EQ(BtActStatus.action_no, expanded::kXiaoDraw);
    EXPECT_TRUE(expanded::SandCountering());
    EXPECT_EQ(expanded::SandCounterAttack(30), 60);

    // Quick Draw's jump: the draw goes straight to the shot.
    expanded::QuickDrawShot(CharaMain, 0);
    EXPECT_EQ(BtActStatus.action_no, expanded::kXiaoShot);
    BtActStatus = {};
    expanded::SandCounterStep();
}
