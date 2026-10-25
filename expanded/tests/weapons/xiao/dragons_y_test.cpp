#include <gtest/gtest.h>

#include <cstring>
#include <memory>

#include "bt_shot_effect.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/xiao/dragons_y.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::TestFloor;
using expanded::test::Wielding;

// Xiao in her stance's strafe (motion 19, table speed 0.5), its speed the game's own.
struct Strafing {
    TestFloor   floor;
    MOTION_INFO info[40] = {};
    CCharacter  chara;
    s32         saved_lock = lockOnTargetFlag;

    Strafing() {
        floor.status().cur_chara = CHARA_XIAO;
        info[19].speed = 0.5f;
        info[2].speed = 0.5f;
        chara.motion_type.motion_info = info;
        chara.motion_no = 19;
        chara.motion_speed = -1.0f;
        lockOnTargetFlag = 1;
    }

    ~Strafing() { lockOnTargetFlag = saved_lock; }

    float Speed() {
        chara.motion_speed = -1.0f;
        expanded::LockOnSpeedMotion(chara, CHARA_XIAO);
        return chara.motion_speed;
    }
};

} // namespace

TEST(DragonsY, LockedOnXiaoMovesFasterDownTheLine) {
    for (int weapon : {ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_ANGE_GEAR}) {
        Strafing strafing;
        {
            Wielding slingshot(weapon);
            EXPECT_FLOAT_EQ(strafing.Speed(), 0.5f * expanded::kLockOnSpeed) << weapon;
            EXPECT_FLOAT_EQ(expanded::LockOnMoveScale(), expanded::kLockOnSpeed) << weapon;
        }
        Wielding steve(ITEM_WEAPON_SUPER_STEVE, weapon);
        EXPECT_FLOAT_EQ(strafing.Speed(), 0.5f * expanded::kLockOnSpeed) << weapon;
    }
}

TEST(DragonsY, OnlyLockedOnAndOnlyHerStrafesAndGuardSteps) {
    Strafing strafing;
    Wielding slingshot(ITEM_WEAPON_DRAGON_S_Y);

    lockOnTargetFlag = 0;
    EXPECT_FLOAT_EQ(strafing.Speed(), -1.0f);
    lockOnTargetFlag = 1;

    strafing.chara.motion_no = 2; // her walk
    EXPECT_FLOAT_EQ(strafing.Speed(), -1.0f);
    strafing.chara.motion_no = 19;

    // A speed the game set itself stands.
    strafing.chara.motion_speed = 0.8f;
    expanded::LockOnSpeedMotion(strafing.chara, CHARA_XIAO);
    EXPECT_FLOAT_EQ(strafing.chara.motion_speed, 0.8f);

    Wielding other(ITEM_WEAPON_MATADOR);
    EXPECT_FLOAT_EQ(strafing.Speed(), -1.0f);
    EXPECT_FLOAT_EQ(expanded::LockOnMoveScale(), 1.0f);
}

TEST(DragonsY, EachElementFiresItsGemronsBallTheBlackDragonsWithNone) {
    const char *names[] = {"f_boll_3", "i_boll", "t_boll", "e114a_ex", "e115a_ex"};
    for (int element = 0; element < 5; element++) {
        const BT_SHOT_EFFECT *ball = expanded::DragonShot(element);
        EXPECT_STREQ(ball->model_name, names[element]);
        EXPECT_EQ(ball->target, 2) << element;
        EXPECT_EQ(ball->hit_flags & ~0x1F, 0) << element;
    }
    EXPECT_STREQ(expanded::DragonShot(-1)->model_name, "b_boll");
    // The Black Dragon's freezes; the ball Xiao fires does not.
    EXPECT_NE(BtEntryEffectTbl[22]->hit_flags & ~0x1F, 0);
    EXPECT_EQ(expanded::DragonShot(-1)->hit_flags & ~0x1F, 0);
}

TEST(DragonsY, ItsHitThrowsFromBehindTheBall) {
    auto effect = std::make_unique<CSHOT_EFFECT>();
    effect->effect_data = const_cast<BT_SHOT_EFFECT *>(expanded::DragonShot(0));
    effect->velocity[2][0] = 0.0f;
    effect->velocity[2][2] = 3.0f;
    effect->chara[2].pos[0] = 10.0f;
    effect->chara[2].pos[1] = 5.0f;
    effect->chara[2].pos[2] = 40.0f;
    COLLISION_HIT hit = {};
    expanded::ShotEffectHitSet(*effect, 2, hit);

    EXPECT_EQ(hit.knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit.knockback_speed, expanded::kDragonShotKickSpeed);
    EXPECT_FLOAT_EQ(hit.knockback_decay, expanded::kDragonShotKickDecay);
    EXPECT_FLOAT_EQ(hit.knockback_origin[0], 10.0f);
    EXPECT_FLOAT_EQ(hit.knockback_origin[2], 40.0f - expanded::kDragonShotBehind);

    // Another shot's hit is left alone.
    BT_SHOT_EFFECT other = *BtEntryEffectTbl[5];
    effect->effect_data = &other;
    COLLISION_HIT plain = {};
    expanded::ShotEffectHitSet(*effect, 2, plain);
    EXPECT_EQ(plain.knockback_mode, 0);
}
