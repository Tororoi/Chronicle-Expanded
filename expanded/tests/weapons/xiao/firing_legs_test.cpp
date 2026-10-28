#include "mathconst.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/motion_layer.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/firing_legs.hpp"
#include "itemdata.hpp"
#include "motion_rig.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::FiringLegs;
using expanded::FiringLegsKey;
using expanded::GameLegMove;
using expanded::kLegFadeFrames;
using expanded::test::MatrixGap;
using expanded::test::MotionRig;
using expanded::test::OptionScope;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// Xiao's KEYs as her dungeon cfg (c04b base.cfg) has them: first frame, last, rate.
struct Key {
    int   key;
    u32   start, end;
    float rate;
};

constexpr Key kKeys[] = {
    {1,  70,  90,  0.7f},
    {2,  30,  50,  0.7f},
    {11, 240, 251, 0.7f},
    {12, 250, 250, 0.0f},
    {13, 251, 255, 0.7f},
    {18, 100, 110, 0.3f},
    {19, 180, 200, 0.5f},
    {20, 210, 230, 0.5f},
    {21, 120, 140, 0.5f},
    {22, 150, 170, 0.5f},
};

// Her rig as the layer reads it, model and shadow: the hips, the chest seam and the joint under it, an arm, a thigh that
// steps 180-200 (the right strafe) and stands still in the shot.
MotionRig::Joint const kModel[] = {
    {"dummyRoot", -1, 0, 0,  0},
    {"g_ctr",     0,  0, 0,  0},
    {"c_ctr",     1,  0, 8,  0},
    {"jnt1_1",    2,  0, 0,  0},
    {"chn2",      3,  0, 1,  0},
    {"jnt2_1",    4,  0, 1,  0},
    {"eff2",      5,  0, 2,  0},
    {"arm",       6,  1, 0,  0},
    {"thigh",     3,  1, -1, 0}
};
constexpr int kThigh = 8;

void Steps(MotionRig &rig) {
    rig.Turn(kThigh, 0, {
                            {180,  0.4f },
                            {190,  -0.4f},
                            {200,  0.4f },
                            {240,  0.0f },
                            {1000, 0.0f }
    });
    for (const Key &k : kKeys) {
        rig.Info()[k.key] = {k.start, k.end, k.rate, 0};
    }
}

// Xiao out with Super Steve and the Star Breaker's sphere, locked on, facing +z, drawing a shot (KEY 11 at 240).
class FiringLegsTest : public ::testing::Test {
protected:
    void SetUp() override {
        BtActStatus = {};
        BtActStatus.action_on = 1;
        BtActStatus.action_no = expanded::kXiaoDraw;
        BtActStatus.can_act = 1;
        floor_.status().cur_chara = CHARA_XIAO;
        lockOnTargetFlag = 1;
        CharaMainHandViewFlag = 0;
        model_.Attach(chara_.motion_type);
        shadow_.Attach(chara_.shadow_motion_type);
        Steps(model_);
        Steps(shadow_);
        Steps(walk_);
        chara_.frame = model_.Root();
        chara_.shadow_frame = shadow_.Root();
        chara_.motion[0] = &chara_.motion_type;
        chara_.shadow_motion[0] = &chara_.shadow_motion_type;
        chara_.motion_start[0] = 0;
        chara_.motion_end[0] = 64;
        chara_.motion_no = expanded::kXiaoDraw;
        chara_.motion_flags = 0;
        chara_.SetRotation(0.0f, 0.0f, 0.0f);
        chara_.shadow_motion_type.state = {};
        MOTION_STATE &state = chara_.motion_type.state;
        state = {};
        state.motion_no = state.playing_no = expanded::kXiaoDraw;
        state.time = 240.0f;
        state.blend_step = 0.1f;
    }

    void TearDown() override {
        expanded::FiringLegsStep(chara_, CHARA_TOAN); // she leaves: the legs go
        BtActStatus = {};
        lockOnTargetFlag = saved_lock_;
        CharaMainHandViewFlag = saved_view_;
    }

    // Her stick's move, in the world (she faces +z, her right is -x).
    void Move(float x, float z) {
        BtActStatus.move_x = x;
        BtActStatus.move_z = z;
        BtActStatus.stick_strength = std::hypot(x, z);
    }

    void Step(int steps = 1) {
        for (int i = 0; i < steps; i++) {
            expanded::FiringLegsStep(chara_, CHARA_XIAO);
        }
    }

    float Weight() { return FiringLegs() != nullptr ? FiringLegs()->weight : 0.0f; }

    float Time() { return FiringLegs() != nullptr ? FiringLegs()->time : -1.0f; }

    TestFloor  floor_;
    Wielding   wielding_{ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_STAR_BREAKER};
    CCharacter chara_;
    MotionRig  model_{kModel[0], kModel[1], kModel[2], kModel[3], kModel[4], kModel[5], kModel[6], kModel[7], kModel[8]};
    MotionRig  shadow_{kModel[0], kModel[1], kModel[2], kModel[3], kModel[4], kModel[5], kModel[6], kModel[7], kModel[8]};
    MotionRig  walk_{kModel[0], kModel[1], kModel[2], kModel[3], kModel[4], kModel[5], kModel[6], kModel[7], kModel[8]};
    s32        saved_lock_ = lockOnTargetFlag;
    s32        saved_view_ = CharaMainHandViewFlag;
};

constexpr float kFade = 1.0f / kLegFadeFrames;

} // namespace

// The leg clip follows the way she moves from her facing, as MoveChara picks her move: locked on, the four strafes.
TEST(FiringLegs, TheMoveFollowsHerDirection) {
    EXPECT_EQ(GameLegMove(0.0f, 0.0f, 1.0f, 1.0f, true, 0, 0, 0.7f).key, expanded::kStrafeForward);
    EXPECT_EQ(GameLegMove(0.0f, 0.0f, -1.0f, 1.0f, true, 0, 0, 0.7f).key, expanded::kStrafeBack);
    EXPECT_EQ(GameLegMove(0.0f, -1.0f, 0.0f, 1.0f, true, 0, 0, 0.7f).key, expanded::kStrafeRight);
    EXPECT_EQ(GameLegMove(0.0f, 1.0f, 0.0f, 1.0f, true, 0, 0, 0.7f).key, expanded::kStrafeLeft);
    // Turned, the same move is another way from her facing; the angle wraps as MoveChara's does.
    EXPECT_EQ(GameLegMove(HALF_PI_SHORT, 1.0f, 0.0f, 1.0f, true, 0, 0, 0.7f).key, expanded::kStrafeForward);
    EXPECT_EQ(GameLegMove(3.0f, std::sin(-3.0f), std::cos(-3.0f), 1.0f, true, 0, 0, 0.7f).key, expanded::kStrafeForward);
    // Unlocked and facing her move, keyCtrl's walk or run; aside or back, the strafes still.
    EXPECT_EQ(GameLegMove(0.0f, 0.0f, 0.4f, 0.4f, false, 0.0f, -0.4f, 0.7f).key, expanded::kMoveWalk);
    EXPECT_EQ(GameLegMove(0.0f, 0.0f, 1.0f, 1.0f, false, 0.0f, -1.0f, 0.7f).key, expanded::kMoveRun);
    EXPECT_EQ(GameLegMove(0.0f, -1.0f, 0.0f, 1.0f, false, 1.0f, 0.0f, 0.7f).key, expanded::kStrafeRight);
}

// At the game's own rate: the strafes 0.2 on the stick's strength up to 1, keyCtrl's walk on the stick's tilt up to 0.7,
// the run its KEY's.
TEST(FiringLegs, TheGamesRate) {
    EXPECT_FLOAT_EQ(GameLegMove(0.0f, -0.3f, 0.0f, 0.3f, true, 0, 0, 0.7f).rate, 0.5f);
    EXPECT_FLOAT_EQ(GameLegMove(0.0f, -1.0f, 0.0f, 1.0f, true, 0, 0, 0.7f).rate, 1.0f);
    EXPECT_FLOAT_EQ(GameLegMove(0.0f, 0.0f, 0.3f, 0.3f, false, 0.0f, -0.3f, 0.7f).rate, 0.8f * (0.1f + 0.3f));
    EXPECT_FLOAT_EQ(GameLegMove(0.0f, 0.0f, 0.8f, 0.8f, false, 0.0f, -0.8f, 0.7f).rate, 0.7f);
    EXPECT_FLOAT_EQ(GameLegMove(0.0f, 0.0f, 1.0f, 1.0f, false, 0.6f, -0.6f, 0.55f).rate, 0.55f);
}

// Standing still she fires exactly as the game has her: no legs laid.
TEST_F(FiringLegsTest, StandingStillLeavesTheShotAlone) {
    Move(0.0f, 0.0f);
    Step(5);
    EXPECT_EQ(FiringLegs(), nullptr);
}

// Moving right as she fires, her legs take the right strafe from its first frame, fading in over kLegFadeFrames, the
// clip at the game's rate (1 on a full stick).
TEST_F(FiringLegsTest, MovingSheWalksUnderTheShot) {
    Move(-1.0f, 0.0f);
    Step();
    ASSERT_NE(FiringLegs(), nullptr);
    EXPECT_EQ(FiringLegsKey(), expanded::kStrafeRight);
    EXPECT_FLOAT_EQ(Weight(), kFade);
    EXPECT_FLOAT_EQ(Time(), 180.0f);
    Step(kLegFadeFrames - 1);
    EXPECT_NEAR(Weight(), 1.0f, 1e-5f);
    EXPECT_FLOAT_EQ(Time(), 180.0f + (kLegFadeFrames - 1));
    // the clip wraps at its end
    Step(20);
    EXPECT_NEAR(Time(), 180.0f + (kLegFadeFrames - 1), 1e-3f);
}

TEST_F(FiringLegsTest, ALighterStickStepsSlower) {
    Move(-0.3f, 0.0f);
    Step(3);
    EXPECT_FLOAT_EQ(Time(), 181.0f);
}

// A step under way when the shot starts carries on under it: the body was playing this strafe.
TEST_F(FiringLegsTest, AStepUnderWayCarriesOn) {
    MOTION_STATE &state = chara_.motion_type.state;
    state.playing_no = expanded::kStrafeRight;
    state.time = 187.25f;
    Move(-1.0f, 0.0f);
    Step();
    EXPECT_FLOAT_EQ(Weight(), 1.0f);
    EXPECT_FLOAT_EQ(Time(), 187.25f);
}

// Turning to go left, the legs cross-fade from the right strafe, held where it was, into the left from its start.
TEST_F(FiringLegsTest, TurningCrossFadesTheLegs) {
    Move(-1.0f, 0.0f);
    Step(4);
    const float held = Time();
    Move(1.0f, 0.0f);
    Step();
    EXPECT_EQ(FiringLegsKey(), expanded::kStrafeLeft);
    EXPECT_FLOAT_EQ(FiringLegs()->from_time, held);
    EXPECT_FLOAT_EQ(FiringLegs()->from_weight, kFade);
    EXPECT_FLOAT_EQ(Time(), 211.0f);
    Step(kLegFadeFrames - 1);
    EXPECT_LT(FiringLegs()->from_time, 0.0f);
}

// She stops: the legs fade back to the shot's own over kLegFadeFrames and are lifted.
TEST_F(FiringLegsTest, StoppingFadesThemOut) {
    Move(-1.0f, 0.0f);
    Step(kLegFadeFrames);
    Move(0.0f, 0.0f);
    Step();
    EXPECT_NEAR(Weight(), 1.0f - kFade, 1e-5f);
    Step(kLegFadeFrames);
    EXPECT_EQ(FiringLegs(), nullptr);
}

// The shot over and her stick still on, the body asks for the strafe: its cross-fade arrives on the legs' frame, then it
// plays on alone from the legs' step.
TEST_F(FiringLegsTest, TheShotsEndHandsTheStepOn) {
    Move(-1.0f, 0.0f);
    Step(kLegFadeFrames);
    BtActStatus.action_on = 0;
    chara_.motion_no = expanded::kStrafeRight;
    Step();
    ASSERT_NE(FiringLegs(), nullptr);
    EXPECT_EQ(FiringLegs()->carry_key, expanded::kStrafeRight);
    EXPECT_NEAR(FiringLegs()->carry_frame, Time() + 9.0f, 1e-3f); // ten blend steps of 0.1, the tenth lands
    EXPECT_FLOAT_EQ(Weight(), 1.0f);
    const float   legs = Time();
    MOTION_STATE &state = chara_.motion_type.state;
    state.playing_no = expanded::kStrafeRight;
    state.blending = 0;
    Step();
    EXPECT_EQ(FiringLegs(), nullptr);
    EXPECT_FLOAT_EQ(state.time, legs + 1.0f);
}

// The shot over and the body asking for anything else (she stopped), the legs fade out under it.
TEST_F(FiringLegsTest, AnotherMoveAfterTheShotFadesThem) {
    Move(-1.0f, 0.0f);
    Step(kLegFadeFrames);
    BtActStatus.action_on = 0;
    chara_.motion_no = expanded::kLockOnStance;
    Step();
    EXPECT_EQ(FiringLegs()->carry_key, -1);
    EXPECT_NEAR(Weight(), 1.0f - kFade, 1e-5f);
}

// Held (a paused or stopped step), nothing moves on.
TEST_F(FiringLegsTest, AHeldStepHoldsTheLegs) {
    Move(-1.0f, 0.0f);
    Step(3);
    chara_.motion_flags = 1;
    const float time = Time(), weight = Weight();
    Step(5);
    EXPECT_FLOAT_EQ(Time(), time);
    EXPECT_FLOAT_EQ(Weight(), weight);
}

// Her shadow's legs walk as her model's do.
TEST_F(FiringLegsTest, HerShadowWalksToo) {
    Move(-1.0f, 0.0f);
    Step(kLegFadeFrames + 3);
    walk_.Pose(Time());
    shadow_.Pose(245.0f);
    EXPECT_LT(MatrixGap(shadow_.Local(kThigh), walk_.Local(kThigh)), 1e-4f);
}

// None for anyone else, another sphere, no shot, first person or with the Weapon Abilities off.
TEST_F(FiringLegsTest, OnlyWhenHerMoveIsLeftToHer) {
    Move(-1.0f, 0.0f);
    BtActStatus.action_on = 0;
    Step();
    EXPECT_EQ(FiringLegs(), nullptr);
    BtActStatus.action_on = 1;

    CharaMainHandViewFlag = 1;
    Step();
    EXPECT_EQ(FiringLegs(), nullptr);
    CharaMainHandViewFlag = 0;

    expanded::FiringLegsStep(chara_, CHARA_TOAN);
    EXPECT_EQ(FiringLegs(), nullptr);
    {
        Wielding other(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DIVINEBEASTTITLE);
        Step();
        EXPECT_EQ(FiringLegs(), nullptr);
    }
    {
        OptionScope options;
        ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
        Step();
        EXPECT_EQ(FiringLegs(), nullptr);
    }
    Step();
    EXPECT_NE(FiringLegs(), nullptr);
}

// Another member out drops the legs at once; her model reloaded, they start over on the new one.
TEST_F(FiringLegsTest, LeavingDropsThem) {
    Move(-1.0f, 0.0f);
    Step(3);
    expanded::FiringLegsStep(chara_, CHARA_TOAN);
    EXPECT_EQ(FiringLegs(), nullptr);
    Step(kLegFadeFrames);
    ASSERT_NEAR(Weight(), 1.0f, 1e-5f);
    chara_.frame = walk_.Root();
    Step();
    EXPECT_FLOAT_EQ(Weight(), kFade);
    EXPECT_FLOAT_EQ(Time(), 180.0f);
}

// The step's foot sounds: the frames a step of the legs' clock passes, by CCharacter::Step's rule (a step of a whole
// frame or more passes none, so a full stick's strafe at 1 is as quiet as the game's).
TEST_F(FiringLegsTest, FootSoundsFromTheLegsClock) {
    chara_.event[0] = {188, 0, 0, 0, 0};
    chara_.event[1] = {198, 0, 1, 0, 0};
    chara_.event[2] = {188, 0, 0, 1, 0}; // another set's
    chara_.foot_sound[0] = {43, 0, 0, 0, 0};
    EXPECT_EQ(expanded::SoundFramesPassed(chara_, 0, 187.5f, 188.4f), std::vector<int>{188});
    EXPECT_EQ(expanded::SoundFramesPassed(chara_, 0, 188.0f, 188.7f), std::vector<int>{188});
    EXPECT_TRUE(expanded::SoundFramesPassed(chara_, 0, 188.5f, 189.4f).empty());
    EXPECT_TRUE(expanded::SoundFramesPassed(chara_, 0, 199.5f, 180.5f).empty()); // the loop back
    EXPECT_TRUE(expanded::SoundFramesPassed(chara_, 0, 187.5f, 188.5f).empty());
    EXPECT_EQ(expanded::SoundFramesPassed(chara_, 0, 42.6f, 43.1f), std::vector<int>{43});
}
