#include <gtest/gtest.h>

#include <cmath>

#include "expanded/core/motion_layer.hpp"
#include "motion_rig.hpp"
#include "test_support.hpp"

namespace {

using expanded::MotionLayer;
using expanded::test::MatrixGap;
using expanded::test::MotionRig;

enum : int {
    kRoot,
    kHips,
    kChest,
    kRestJoint,
    kArm,
    kThigh,
    kShin
};

// A body as the layer splits it: the hips (turned and raised by their tracks) carry the chest (jnt2_1), the joint under it
// that no track drives (eff2, turned at rest) and an arm on that, and a thigh and a shin. Frames 100-120 walk (the hips
// sway, the legs step, the chest and arm hang still); frames 240-260 shoot (the hips square, the chest twists, the arm
// draws). A key at 1000 closes every track.
struct Body {
    MotionRig rig{
        {"root",   -1, 0, 0,  0},
        {"hips",   0,  0, 8,  0},
        {"jnt2_1", 1,  0, 2,  0},
        {"eff2",   2,  0, 2,  0},
        {"arm",    3,  1, 0,  0},
        {"thigh",  1,  1, -1, 0},
        {"shin",   5,  0, -3, 0}
    };

    Body() {
        rig.Turn(kHips, 1, {
                               {100,  0.3f },
                               {110,  -0.3f},
                               {120,  0.3f },
                               {240,  0.0f },
                               {260,  0.0f },
                               {1000, 0.0f }
        });
        rig.Move(kHips, {
                            {100,  {0, 7.5f, 0}},
                            {110,  {0, 8.5f, 0}},
                            {120,  {0, 7.5f, 0}},
                            {240,  {0, 8, 0}   },
                            {1000, {0, 8, 0}   }
        });
        rig.Turn(kChest, 1, {
                                {100,  0.0f},
                                {120,  0.0f},
                                {240,  0.2f},
                                {250,  0.6f},
                                {260,  0.6f},
                                {1000, 0.0f}
        });
        rig.Turn(kArm, 2, {
                              {100,  0.1f},
                              {120,  0.1f},
                              {240,  0.5f},
                              {250,  1.0f},
                              {260,  1.0f},
                              {1000, 0.0f}
        });
        rig.Turn(kThigh, 0, {
                                {100,  0.5f },
                                {110,  -0.5f},
                                {120,  0.5f },
                                {240,  0.05f},
                                {260,  0.05f},
                                {1000, 0.0f }
        });
        rig.Turn(kShin, 0, {
                               {100,  0.2f},
                               {110,  0.8f},
                               {120,  0.2f},
                               {240,  0.1f},
                               {260,  0.1f},
                               {1000, 0.0f}
        });
        rig.Rest(kRestJoint, 0, 0.35f);
    }

    ~Body() { expanded::LiftMotion(&rig.Motion(), false); }

    // The block posed at time with layer laid over it (none for nullptr).
    void Pose(float time, MotionLayer *layer) {
        if (layer != nullptr) {
            expanded::LayMotion(&rig.Motion(), layer);
        } else {
            expanded::LiftMotion(&rig.Motion(), true);
        }
        rig.Pose(time);
    }
};

// The legs walking at frame time under the shot.
MotionLayer Legs(float time, float weight = 1.0f) {
    MotionLayer layer;
    layer.seam = "jnt2_1";
    layer.seam_next = "eff2";
    layer.time = time;
    layer.weight = weight;
    return layer;
}

// The angle between two matrices' rotations.
float Angle(const std::array<float, 16> &a, const std::array<float, 16> &b) {
    float trace = 0.0f;
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 3; k++) {
            trace += a[i * 4 + k] * b[i * 4 + k];
        }
    }
    return std::acos(std::clamp((trace - 1.0f) / 2.0f, -1.0f, 1.0f));
}

constexpr float kShot = 245.0f;
constexpr float kStep = 103.0f;
constexpr float kNear = 1e-4f;

} // namespace

// Below the chest the body takes the walk's frame, as the walk alone poses it; the arm keeps the shot's own turn.
TEST(MotionLayer, TheLowerBodyWalks) {
    Body walk, shot, both;
    walk.Pose(kStep, nullptr);
    shot.Pose(kShot, nullptr);
    MotionLayer legs = Legs(kStep);
    both.Pose(kShot, &legs);
    for (int joint : {kHips, kThigh, kShin}) {
        EXPECT_LT(MatrixGap(both.rig.Local(joint), walk.rig.Local(joint)), kNear) << joint;
        EXPECT_GT(MatrixGap(both.rig.Local(joint), shot.rig.Local(joint)), 0.05f) << joint;
    }
    EXPECT_LT(MatrixGap(both.rig.Local(kArm), shot.rig.Local(kArm)), kNear);
}

// The upper body holds the shot's pose in the world while the hips sway under it: from the joint under the chest up,
// every joint turns as the shot alone turns it; the chest takes half the waist's turn.
TEST(MotionLayer, TheUpperBodyHoldsTheShot) {
    Body shot, ride, both;
    shot.Pose(kShot, nullptr);
    MotionLayer riding = Legs(kStep);
    riding.torso_turn = 1.0f;
    ride.Pose(kShot, &riding);
    MotionLayer legs = Legs(kStep);
    both.Pose(kShot, &legs);
    for (int joint : {kRestJoint, kArm}) {
        EXPECT_LT(MatrixGap(both.rig.World(joint), shot.rig.World(joint), false), kNear) << joint;
        EXPECT_GT(Angle(ride.rig.World(joint), shot.rig.World(joint)), 0.1f) << joint;
    }
    const float waist = Angle(ride.rig.World(kChest), shot.rig.World(kChest));
    EXPECT_NEAR(Angle(both.rig.World(kChest), shot.rig.World(kChest)), waist / 2.0f, 1e-3f);
}

// Riding the hips the upper body keeps the shot's locals: the chest's own turn, the joint under it at rest.
TEST(MotionLayer, RidingTheHipsKeepsTheShotsLocals) {
    Body shot, ride;
    shot.Pose(kShot, nullptr);
    MotionLayer riding = Legs(kStep);
    riding.torso_turn = 1.0f;
    ride.Pose(kShot, &riding);
    for (int joint : {kChest, kRestJoint, kArm}) {
        EXPECT_LT(MatrixGap(ride.rig.Local(joint), shot.rig.Local(joint)), kNear) << joint;
    }
}

// No weight, no change: the shot as the game poses it.
TEST(MotionLayer, NoWeightLeavesTheMotionAsItIs) {
    Body shot, none;
    shot.Pose(kShot, nullptr);
    MotionLayer legs = Legs(kStep, 0.0f);
    none.Pose(kShot, &legs);
    for (int joint = kHips; joint <= kShin; joint++) {
        EXPECT_LT(MatrixGap(none.rig.Local(joint), shot.rig.Local(joint)), kNear) << joint;
    }
}

// Fading in, the lower body turns weight of the way from the shot's legs to the walk's.
TEST(MotionLayer, WeightFadesTheLegsIn) {
    Body walk, shot, half;
    walk.Pose(kStep, nullptr);
    shot.Pose(kShot, nullptr);
    MotionLayer legs = Legs(kStep, 0.5f);
    half.Pose(kShot, &legs);
    const float whole = Angle(walk.rig.Local(kThigh), shot.rig.Local(kThigh));
    EXPECT_NEAR(Angle(half.rig.Local(kThigh), shot.rig.Local(kThigh)), whole / 2.0f, 1e-3f);
    EXPECT_NEAR(half.rig.Local(kHips)[13], (walk.rig.Local(kHips)[13] + shot.rig.Local(kHips)[13]) / 2.0f, 1e-4f);
}

// Changing step, the legs cross-fade from the old one held still.
TEST(MotionLayer, CrossFadesFromAHeldFrame) {
    Body from, to, mid;
    from.Pose(115.0f, nullptr);
    to.Pose(kStep, nullptr);
    MotionLayer legs = Legs(kStep);
    legs.from_time = 115.0f;
    legs.from_weight = 0.0f;
    mid.Pose(kShot, &legs);
    EXPECT_LT(MatrixGap(mid.rig.Local(kThigh), from.rig.Local(kThigh)), kNear);
    legs.from_weight = 0.5f;
    mid.Pose(kShot, &legs);
    const float whole = Angle(from.rig.Local(kThigh), to.rig.Local(kThigh));
    EXPECT_NEAR(Angle(mid.rig.Local(kThigh), from.rig.Local(kThigh)), whole / 2.0f, 1e-3f);
}

// The joint under the chest has no track to put it back: lifting the layer does, unless its model is gone.
TEST(MotionLayer, LiftingPutsTheRestJointBack) {
    Body        body;
    const auto  rest = body.rig.Local(kRestJoint);
    MotionLayer legs = Legs(kStep);
    body.Pose(kShot, &legs);
    EXPECT_GT(MatrixGap(body.rig.Local(kRestJoint), rest), 0.01f);
    expanded::LiftMotion(&body.rig.Motion(), true);
    EXPECT_LT(MatrixGap(body.rig.Local(kRestJoint), rest), 1e-6f);

    body.Pose(kShot, &legs);
    expanded::LiftMotion(&body.rig.Motion(), false);
    EXPECT_GT(MatrixGap(body.rig.Local(kRestJoint), rest), 0.01f);
}

// A weight run down to nothing puts it back too, and the next pose is the motion's own.
TEST(MotionLayer, NoWeightPutsTheRestJointBack) {
    Body body, shot;
    shot.Pose(kShot, nullptr);
    MotionLayer legs = Legs(kStep);
    body.Pose(kShot, &legs);
    legs.weight = 0.0f;
    body.Pose(kShot, &legs);
    for (int joint = kHips; joint <= kShin; joint++) {
        EXPECT_LT(MatrixGap(body.rig.Local(joint), shot.rig.Local(joint)), kNear) << joint;
    }
}

// A key change into the carried key arrives on the carried frame; any other key starts where the game starts it.
TEST(MotionLayer, AKeyChangeTakesTheCarry) {
    Body        body;
    MotionLayer legs = Legs(kStep);
    legs.carry_key = 21;
    legs.carry_frame = 132.4f;
    expanded::LayMotion(&body.rig.Motion(), &legs);
    MOTION_STATE state = {};
    state.playing_no = 13;
    state.motion_no = 21;
    state.next_frame = 120;
    expanded::MotionPosing(&body.rig.Motion(), &state);
    EXPECT_EQ(state.next_frame, 132u);

    state.next_frame = 120;
    state.motion_no = 18;
    expanded::MotionPosing(&body.rig.Motion(), &state);
    EXPECT_EQ(state.next_frame, 120u);

    state.motion_no = state.playing_no = 21; // under way already
    expanded::MotionPosing(&body.rig.Motion(), &state);
    EXPECT_EQ(state.next_frame, 120u);
}

// A model and its shadow each take the one layer; a model without the seam is left alone.
TEST(MotionLayer, EachLaidMotionTakesIt) {
    Body        walk, model, shadow, shot;
    MotionLayer legs = Legs(kStep);
    walk.Pose(kStep, nullptr);
    shot.Pose(kShot, nullptr);
    expanded::LayMotion(&model.rig.Motion(), &legs);
    expanded::LayMotion(&shadow.rig.Motion(), &legs);
    model.rig.Pose(kShot);
    shadow.rig.Pose(kShot);
    EXPECT_LT(MatrixGap(model.rig.Local(kThigh), walk.rig.Local(kThigh)), kNear);
    EXPECT_LT(MatrixGap(shadow.rig.Local(kThigh), walk.rig.Local(kThigh)), kNear);

    MotionRig other{
        {"root",  -1, 0, 0, 0},
        {"thigh", 0,  0, 0, 0}
    };
    MotionLayer seamless = Legs(kStep);
    other.Turn(1, 0, {
                         {100,  0.5f},
                         {240,  0.0f},
                         {1000, 0.0f}
    });
    expanded::LayMotion(&other.Motion(), &seamless);
    other.Pose(kShot);
    const auto shot_thigh = other.Local(1);
    expanded::LiftMotion(&other.Motion(), true);
    other.Pose(kShot);
    EXPECT_LT(MatrixGap(other.Local(1), shot_thigh), kNear);
}
