#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

#include "camera.hpp"
#include "character.hpp"
#include "collision.hpp"
#include "editloop.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fishing_fight.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "mainselect.hpp"
#include "test_support.hpp"

namespace {

using expanded::FightStep;
using expanded::test::OptionScope;

constexpr int kSlots = 6;
constexpr int kHooked = 2; // the slot of the fish that took the hook

// A Nonky's sizes: usual 8, largest 19.
FishInfo Nonky() {
    return {.model_size = 25.5f, .min_size = 8.0f, .max_size = 19.0f, .min_fp = 8, .max_fp = 25, .food_affinity = {}};
}

// How far the stick is pushed to pull.
constexpr float kPush = 0.8f;

// A stick pushed push the way yaw across the water, with the camera looking camera_yaw, as the fight reads it: x right,
// y down.
std::array<float, 2> StickToward(float camera_yaw, float yaw, float push = kPush) {
    const expanded::FightStick read = expanded::FightStickRead(push * std::sin(camera_yaw - yaw), -push * std::cos(camera_yaw - yaw));
    return {read.x, read.y};
}

// A stick pushed fully the way yaw.
std::array<float, 2> StickNear(float camera_yaw, float yaw) {
    return StickToward(camera_yaw, yaw, 1.0f);
}

// Six fish, the hooked one a usual-sized Nonky out from the angler, who stands at the origin facing +z, the float just
// short of it; no camera.
class FishingFight : public ::testing::Test {
protected:
    void SetUp() override {
        Fish = fish_.get();
        Chara = &angler_;
        BattleFish = nullptr;
        NowCamera = nullptr;
        DebugMode = 0;
        angler_.SetPosition(0.0f, 0.0f, 0.0f);
        angler_.SetRotation(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < kSlots; i++) {
            fish_[i].fish_kind = expanded::fish::kNonky;
            fish_[i].info = Nonky();
            fish_[i].size = 8.0f;
            float position[4] = {0.0f, -12.0f, 40.0f, 1.0f};
            fish_[i].SetPosition(position);
        }
        fish_[kHooked].action = FISH_ACTION_BITE_HOOK;
        std::copy_n(point[kFloat], 4, saved_float_);
        const float bob[4] = {0.0f, 0.0f, 36.0f, 1.0f};
        std::copy_n(bob, 4, point[kFloat]);
    }

    void TearDown() override {
        Fish = saved_fish_;
        Chara = saved_chara_;
        BattleFish = saved_battle_;
        NowCamera = saved_camera_;
        DebugMode = saved_debug_;
        std::copy_n(saved_float_, 4, point[kFloat]);
    }

    void Strike() { FishingBattleFish(kHooked); }

    // A frame of the fight with the stick at (x, y).
    FightStep Step(float x, float y) { return expanded::FishingFightStep(angler_, facing_, x, y); }

    // The way the camera looks: with none, the way the angler faces.
    float CameraYaw() {
        if (NowCamera == nullptr) {
            return angler_.GetRotation()->y;
        }
        float eye[4];
        float look[4];
        NowCamera->GetPos(eye);
        NowCamera->GetRef(look);
        return std::atan2(look[0] - eye[0], look[2] - eye[2]);
    }

    // A frame pulling the way to pull.
    FightStep Pull() {
        const auto stick = StickNear(CameraYaw(), expanded::FishingFightTarget());
        return Step(stick[0], stick[1]);
    }

    // Frames of Pull until the fight ends (or limit); the frames it took.
    int PullUntilOver(FightStep &end, int limit = 2000) {
        for (int frame = 1; frame <= limit; frame++) {
            end = Pull();
            if (end != FightStep::kFighting) {
                return frame;
            }
        }
        return limit;
    }

    OptionScope              options_;
    std::unique_ptr<CFish[]> fish_ = std::make_unique<CFish[]>(kSlots);
    CCharacter               angler_;
    float                    facing_[4] = {};
    CFish                   *saved_fish_ = Fish;
    CCharacter              *saved_chara_ = Chara;
    CFish                   *saved_battle_ = BattleFish;
    CCamera                 *saved_camera_ = NowCamera;
    s32                      saved_debug_ = DebugMode;
    float                    saved_float_[4] = {};

    static constexpr int kFloat = 18; // the line's point the float rides
};

TEST(FishingFightRules, StrengthIsTheSizeOverTheUsualAndAGarayanOneMore) {
    const FishInfo info = Nonky();
    EXPECT_FLOAT_EQ(expanded::FightStrength(expanded::fish::kNonky, 8.0f, info), 0.0f);
    EXPECT_FLOAT_EQ(expanded::FightStrength(expanded::fish::kNonky, 6.0f, info), 0.0f);
    EXPECT_FLOAT_EQ(expanded::FightStrength(expanded::fish::kNonky, 13.5f, info), 0.5f);
    EXPECT_FLOAT_EQ(expanded::FightStrength(expanded::fish::kNonky, 19.0f, info), 1.0f);
    EXPECT_FLOAT_EQ(expanded::FightStrength(expanded::fish::kMardanGarayan, 8.0f, info), 1.0f);
    EXPECT_FLOAT_EQ(expanded::FightStrength(expanded::fish::kBaronGarayan, 19.0f, info), 2.0f);
}

TEST(FishingFightRules, AStrongerFishFillsSlowerAndRunsShorterAndFaster) {
    EXPECT_FLOAT_EQ(expanded::FightFill(0.0f), 1.0f / 240.0f);
    EXPECT_FLOAT_EQ(expanded::FightFill(2.0f), 1.0f / 360.0f);
    EXPECT_EQ(expanded::FightRunFrames(0.0f, 0.0f), 120);
    EXPECT_EQ(expanded::FightRunFrames(0.0f, 1.0f), 240);
    EXPECT_EQ(expanded::FightRunFrames(2.0f, 0.0f), 84);
    EXPECT_EQ(expanded::FightRunFrames(2.0f, 1.0f), 168);
    EXPECT_FLOAT_EQ(expanded::FightRunSpeed(0.0f), 0.35f * expanded::kFightPace);
    EXPECT_FLOAT_EQ(expanded::FightRunSpeed(2.0f), 0.525f * expanded::kFightPace);
}

TEST(FishingFightRules, TheConeKeepsTheFishSwimmingAway) {
    const float cone = expanded::kCone;
    EXPECT_FLOAT_EQ(expanded::FightConeYaw(0.0f, 0.5f), 0.5f);
    EXPECT_FLOAT_EQ(expanded::FightConeYaw(0.0f, 1.5f), cone);
    EXPECT_FLOAT_EQ(expanded::FightConeYaw(0.0f, -2.0f), -cone);
    // Around the turn of the circle: away at 3, a heading of -3 is 0.28 from it.
    EXPECT_NEAR(std::remainder(expanded::FightConeYaw(3.0f, -3.0f) + 3.0f, 2.0f * PI), 0.0f, 1e-5f);
    EXPECT_NEAR(expanded::FightConeYaw(3.0f, 0.5f), 3.0f - cone, 1e-5f);
}

TEST(FishingFightRules, TheLineHoldsTheFishAcrossTheGround) {
    // 40 of reach holds the fish 40 across the ground from the rod, however high the rod and deep the fish.
    const float rod[3] = {10.0f, 60.0f, 10.0f};
    float       at[3] = {10.0f, -12.0f, 70.0f};
    EXPECT_TRUE(expanded::FightLineHold(at, rod, 40.0f));
    EXPECT_FLOAT_EQ(at[0], 10.0f);
    EXPECT_FLOAT_EQ(at[1], -12.0f);
    EXPECT_FLOAT_EQ(at[2], 50.0f);
    EXPECT_FALSE(expanded::FightLineHold(at, rod, 40.0f));
    EXPECT_FALSE(expanded::FightLineHold(at, rod, 41.0f));
}

TEST(FishingFightRules, TheStickPointsAcrossTheWaterAsTheScreenShowsIt) {
    // Up into the screen, right to its right, down toward the camera: the camera's ahead is (sin, cos) of its yaw and
    // its right (-cos, sin).
    auto yaw = [](float camera, float x, float y) { return expanded::FightStickYaw(camera, x, y); };
    auto same = [](float a, float b) { return std::fabs(std::remainder(a - b, 2.0f * PI)) < 1e-5f; };
    // Behind the angler, looking +z (his way): up +z, right -x, down -z.
    EXPECT_TRUE(same(yaw(0.0f, 0.0f, -1.0f), 0.0f));
    EXPECT_TRUE(same(yaw(0.0f, 1.0f, 0.0f), -PI / 2.0f));
    EXPECT_TRUE(same(yaw(0.0f, 0.0f, 1.0f), PI));
    EXPECT_TRUE(same(yaw(0.0f, 0.7f, 0.7f), -3.0f * PI / 4.0f));
    // Turned 90 degrees to look -x across him: up -x, right -z, down +x.
    EXPECT_TRUE(same(yaw(-PI / 2.0f, 0.0f, -1.0f), -PI / 2.0f));
    EXPECT_TRUE(same(yaw(-PI / 2.0f, 1.0f, 0.0f), PI));
    EXPECT_TRUE(same(yaw(-PI / 2.0f, 0.0f, 1.0f), PI / 2.0f));
    // Turned the other way, looking +x: right +z.
    EXPECT_TRUE(same(yaw(PI / 2.0f, 1.0f, 0.0f), 0.0f));
    // Facing him from out on the water, looking -z: up -z (toward him), right +x.
    EXPECT_TRUE(same(yaw(PI, 0.0f, -1.0f), PI));
    EXPECT_TRUE(same(yaw(PI, 1.0f, 0.0f), PI / 2.0f));
    // A stick as the fight reads it points its own way, whichever way it is pushed.
    for (float camera : {0.0f, 1.0f, -2.5f}) {
        for (float off : {0.0f, PI / 4.0f, 2.0f, -0.9f, PI}) {
            const float way = camera - off;
            const auto  stick = StickToward(camera, way, 1.0f);
            EXPECT_TRUE(same(yaw(camera, stick[0], stick[1]), way))
                << camera << " " << way;
        }
    }
}

TEST(FishingFightRules, TheDeadZoneIsMeasuredFromTheCentre) {
    auto read = [](float x, float y) {
        const expanded::FightStick stick = expanded::FightStickRead(x, y);
        return std::array<float, 2>{stick.x, stick.y};
    };
    const float dead = expanded::kStickDead;
    // Within the dead zone, nothing, whichever way.
    EXPECT_EQ(read(0.0f, 0.0f), (std::array<float, 2>{0.0f, 0.0f}));
    EXPECT_EQ(read(0.3f, 0.2f), (std::array<float, 2>{0.0f, 0.0f}));
    EXPECT_EQ(read(dead * 0.7f, dead * 0.7f), (std::array<float, 2>{0.0f, 0.0f}));
    // Full on an axis reads full; past the edge, the push is stretched over 0 to 1 and held to 1.
    EXPECT_FLOAT_EQ(read(1.0f, 0.0f)[0], 1.0f);
    EXPECT_FLOAT_EQ(read(0.0f, -1.0f)[1], -1.0f);
    EXPECT_NEAR(std::hypot(read(0.0f, (1.0f + dead) / 2.0f)[0], read(0.0f, (1.0f + dead) / 2.0f)[1]), 0.5f, 1e-5f);
    EXPECT_NEAR(std::hypot(read(1.0f, 1.0f)[0], read(1.0f, 1.0f)[1]), 1.0f, 1e-5f);
    // Its own way however near an axis: 10 and 30 degrees off down read 10 and 30 (the game's reads give 0 and 13).
    for (float degrees : {10.0f, 30.0f, -5.0f}) {
        const float angle = degrees * PI / 180.0f;
        const auto  stick = read(std::sin(angle), std::cos(angle));
        EXPECT_NEAR(std::atan2(stick[0], stick[1]) * 180.0f / PI, degrees, 1e-3f) << degrees;
    }
}

TEST(FishingFightRules, TheWayToPullIsAgainstTheFishInsideTheCone) {
    // The gauge's middle -z (pi): the float out along +z from the angler.
    auto off = [](float way) { return std::remainder(way - PI, 2.0f * PI); };
    // A fish swimming straight away (+z): pull straight back.
    EXPECT_NEAR(off(expanded::FightTargetYaw(PI, 0.0f)), 0.0f, 1e-5f);
    // Swimming away and to +x: pull back and to -x, the yaw's way up from -z.
    EXPECT_NEAR(off(expanded::FightTargetYaw(PI, 0.5f)), 0.5f, 1e-5f);
    EXPECT_NEAR(off(expanded::FightTargetYaw(PI, -0.5f)), -0.5f, 1e-5f);
    // Square across or toward the angler: held at the cone's edge.
    EXPECT_NEAR(off(expanded::FightTargetYaw(PI, PI / 2.0f)), expanded::kCone, 1e-5f);
    EXPECT_NEAR(off(expanded::FightTargetYaw(PI, -2.5f)), -expanded::kCone, 1e-5f);
    // The middle anywhere round the circle.
    EXPECT_NEAR(std::remainder(expanded::FightTargetYaw(1.0f, 1.0f + PI + 0.3f) - 1.3f, 2.0f * PI), 0.0f, 1e-5f);
}

TEST(FishingFightRules, TheWayToPullEasesAndSlides) {
    const float ease = expanded::kTargetEase;
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.0f, 0.2f), 0.2f * ease);
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.5f, 0.5f), 0.5f);
    // A far turn (the fish switching sides) slides at kTargetTurn a frame.
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.0f, 1.5f), expanded::kTargetTurn);
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.0f, -1.5f), -expanded::kTargetTurn);
    // The short way round the circle: from 3.1 toward -3.1 is up past pi.
    const float eased = expanded::FightEaseTarget(3.1f, -3.1f);
    EXPECT_NEAR(std::remainder(eased - 3.1f, 2.0f * PI), ease * (2.0f * PI - 6.2f), 1e-5f);
}

TEST(FishingFightRules, OnlyAPushWithinTheToleranceOfTheWayPulls) {
    // The camera behind the angler, the way to pull straight back to him: the stick down.
    const float tolerance = expanded::kPullTolerance;
    auto        pulls = [&](float way, float target) {
        const auto stick = StickToward(0.0f, way, 1.0f);
        return expanded::FightPulls(stick[0], stick[1], 0.0f, target);
    };
    EXPECT_TRUE(pulls(PI, PI));
    EXPECT_FALSE(pulls(0.0f, PI)); // pushed forward
    // Down and to the side.
    const float slant = 3.0f * PI / 4.0f;
    EXPECT_TRUE(pulls(slant, slant));
    EXPECT_TRUE(pulls(slant + tolerance - 0.01f, slant));
    EXPECT_TRUE(pulls(slant - tolerance + 0.01f, slant));
    EXPECT_FALSE(pulls(slant + tolerance + 0.01f, slant));
    EXPECT_FALSE(pulls(slant - tolerance - 0.01f, slant));
    EXPECT_FALSE(expanded::FightPulls(0.0f, expanded::kPullBack - 0.01f, 0.0f, PI)); // not far enough (as read)
    EXPECT_TRUE(expanded::FightPulls(0.0f, expanded::kPullBack + 0.01f, 0.0f, PI));
    EXPECT_FALSE(expanded::FightPulls(0.0f, 0.0f, 0.0f, PI)); // let go
    // Down and right is down and right of the screen whichever way the camera looks: for a way to pull of -3/4 pi
    // (-x and -z), looking +z it is down and right; looking +x, down and left.
    EXPECT_TRUE(expanded::FightPulls(0.6f, 0.6f, 0.0f, -3.0f * PI / 4.0f));
    EXPECT_FALSE(expanded::FightPulls(0.6f, 0.6f, PI / 2.0f, -3.0f * PI / 4.0f));
    EXPECT_TRUE(expanded::FightPulls(-0.6f, 0.6f, PI / 2.0f, -3.0f * PI / 4.0f));
    // A way 30 degrees off down, pushed there, pulls though the game reads the push 13 degrees off down.
    const auto off_down = StickToward(0.0f, PI - PI / 6.0f);
    EXPECT_TRUE(expanded::FightPulls(off_down[0], off_down[1], 0.0f, PI - PI / 6.0f));
    EXPECT_FALSE(expanded::FightPulls(off_down[0], off_down[1], 0.0f, PI + PI / 9.0f));
}

TEST(FishingFightRules, ANearMissHolds) {
    // Between the pull's tolerance and the hold's, pushed far enough: it holds and does not pull.
    const float slant = 3.0f * PI / 4.0f;
    const float between = (expanded::kPullTolerance + expanded::kHoldTolerance) / 2.0f;
    for (float off : {between, -between}) {
        const auto stick = StickToward(0.0f, slant + off, 1.0f);
        EXPECT_TRUE(expanded::FightHolds(stick[0], stick[1], 0.0f, slant));
        EXPECT_FALSE(expanded::FightPulls(stick[0], stick[1], 0.0f, slant));
    }
    // A pull holds too; past the hold's tolerance, or let go, it does neither.
    const auto on = StickToward(0.0f, slant, 1.0f);
    EXPECT_TRUE(expanded::FightHolds(on[0], on[1], 0.0f, slant));
    const auto off = StickToward(0.0f, slant + expanded::kHoldTolerance + 0.01f, 1.0f);
    EXPECT_FALSE(expanded::FightHolds(off[0], off[1], 0.0f, slant));
    EXPECT_FALSE(expanded::FightHolds(0.0f, 0.0f, 0.0f, slant));
}

TEST(FishingFightRules, TheGaugeLiesFlatOnTheWater) {
    const float centre[4] = {10.0f, -2.0f, 40.0f, 1.0f};
    float       point[4] = {};
    // Its middle points back to the angler (-z): radius out that way, at the centre's height.
    expanded::FightGaugePoint(centre, PI, 0.0f, 7.0f, point);
    EXPECT_NEAR(point[0], 10.0f, 1e-5f);
    EXPECT_FLOAT_EQ(point[1], -2.0f);
    EXPECT_NEAR(point[2], 33.0f, 1e-5f);
    EXPECT_FLOAT_EQ(point[3], 1.0f);
    // Its ends, kCone either side.
    expanded::FightGaugePoint(centre, PI, expanded::kCone, 7.0f, point);
    EXPECT_NEAR(point[0], 10.0f - 7.0f * std::sin(expanded::kCone), 1e-4f);
    EXPECT_NEAR(point[2], 40.0f - 7.0f * std::cos(expanded::kCone), 1e-4f);
    expanded::FightGaugePoint(centre, PI, -expanded::kCone, 9.0f, point);
    EXPECT_NEAR(point[0], 10.0f + 9.0f * std::sin(expanded::kCone), 1e-4f);
    EXPECT_NEAR(std::hypot(point[0] - 10.0f, point[2] - 40.0f), 9.0f, 1e-4f);
}

TEST_F(FishingFight, AStruckFishIsFoughtFromThirtyPercentOfTheBar) {
    Strike();
    EXPECT_EQ(BattleFish, &fish_[kHooked]);
    EXPECT_FLOAT_EQ(expanded::FishingFightBar(), expanded::kStartBar);
    EXPECT_NE(expanded::FishingFightSide(), 0);
}

TEST_F(FishingFight, TheWayToPullStartsBackToTheAnglerAndFollowsTheFish) {
    // The way to pull starts as the gauge's middle, from the float to the angler (-z).
    Strike();
    EXPECT_NEAR(std::fabs(expanded::FishingFightTarget()), PI, 1e-5f);
    // The fish swimming away and to +x: the way eases round toward pulling back and to -x (the yaw's way up from -z),
    // never past the cone.
    float heading[4] = {0.0f, 0.8f, 0.0f, 0.0f};
    fish_[kHooked].SetRotation(heading);
    float previous = 0.0f;
    for (int i = 0; i < 60; i++) {
        Pull();
        const float way = std::remainder(expanded::FishingFightTarget() - PI, 2.0f * PI);
        EXPECT_GE(way, previous - 1e-5f);
        EXPECT_LE(way, 0.8f + 1e-5f);
        previous = way;
    }
    EXPECT_NEAR(previous, 0.8f, 0.01f);
    // Swimming square across to -x: held at the cone's far edge.
    heading[1] = -PI / 2.0f;
    fish_[kHooked].SetRotation(heading);
    for (int i = 0; i < 120; i++) {
        Pull();
    }
    EXPECT_NEAR(std::remainder(expanded::FishingFightTarget() - PI, 2.0f * PI), -expanded::kCone, 0.01f);
}

TEST_F(FishingFight, TheStickFollowsTheScreenWithTheCameraTurned) {
    // The camera 90 degrees round, looking +x across the line (the screen's right +z): the pull back to the angler
    // (-z) is the screen's left, so the stick pulls left, not down.
    CCamera camera(1.0f);
    float   eye[4] = {-30.0f, 20.0f, 20.0f, 1.0f};
    camera.SetPos(eye);
    camera.SetRef(0.0f, 0.0f, 20.0f);
    NowCamera = &camera;
    Strike();
    for (int i = 0; i < expanded::kStartGrace; i++) {
        Step(0.0f, kPush);
    }
    Step(0.0f, kPush);
    EXPECT_LT(expanded::FishingFightBar(), expanded::kStartBar);
    const float down = expanded::FishingFightBar();
    Step(-kPush, 0.0f);
    EXPECT_GT(expanded::FishingFightBar(), down);
    NowCamera = nullptr;
}

TEST_F(FishingFight, PullingLandsTheFish) {
    Strike();
    FightStep end = FightStep::kFighting;
    const int frames = PullUntilOver(end);
    EXPECT_EQ(end, FightStep::kLanded);
    // Two thirds of the bar at a fill a frame, the grace held at the start and after each switch on top.
    const int fill = static_cast<int>(std::ceil((1.0f - expanded::kStartBar) * expanded::kFillFrames));
    EXPECT_GE(frames, fill + expanded::kStartGrace);
    EXPECT_LE(frames, fill + expanded::kStartGrace + 4 * expanded::kSwitchGrace);
    // It stays landed until the game takes it.
    EXPECT_EQ(Pull(), FightStep::kLanded);
    EXPECT_EQ(Step(0.0f, 0.0f), FightStep::kLanded);
}

TEST_F(FishingFight, LettingGoLosesTheFishThreeTimesAsFast) {
    Strike();
    FightStep end = FightStep::kFighting;
    int       frames = 0;
    while (end == FightStep::kFighting && frames < 2000) {
        end = Step(0.0f, 0.0f);
        frames++;
    }
    EXPECT_EQ(end, FightStep::kGotAway);
    // The bar never rose, so it drains only as fast as a pull fills it.
    const int drain = static_cast<int>(std::ceil(expanded::kStartBar * expanded::kFillFrames));
    EXPECT_GE(frames, drain + expanded::kStartGrace);
    EXPECT_LE(frames, drain + expanded::kStartGrace + expanded::kSwitchGrace + 1);
    // Off it swims, a fish again.
    EXPECT_EQ(BattleFish, nullptr);
    EXPECT_EQ(fish_[kHooked].action, FISH_ACTION_LEAVE);
    EXPECT_EQ(fish_[kHooked].move_mode, FISH_MOVE_SWIM);
    EXPECT_EQ(expanded::FishingFightSide(), 0);
    EXPECT_EQ(Step(0.0f, 0.0f), FightStep::kRetail);
}

TEST_F(FishingFight, ANearMissHoldsTheBarStill) {
    Strike();
    for (int i = 0; i < expanded::kStartGrace; i++) {
        Pull();
    }
    // Halfway between the two tolerances off the way to pull (which moves no more than kTargetTurn a frame), either
    // side, the bar neither fills nor drains.
    const float off = (expanded::kPullTolerance + expanded::kHoldTolerance) / 2.0f;
    for (int i = 0; i < 30; i++) {
        const float bar = expanded::FishingFightBar();
        const auto  stick = StickToward(CameraYaw(), expanded::FishingFightTarget() + (i % 2 == 0 ? off : -off), 1.0f);
        EXPECT_EQ(Step(stick[0], stick[1]), FightStep::kFighting);
        EXPECT_FLOAT_EQ(expanded::FishingFightBar(), bar) << i;
    }
}

TEST_F(FishingFight, AMissDrainsFasterOnceTheBarHasRisen) {
    Strike();
    for (int i = 0; i < expanded::kStartGrace; i++) {
        Step(0.0f, 0.0f);
    }
    float bar = expanded::FishingFightBar();
    Step(0.0f, 0.0f);
    const float gentle = bar - expanded::FishingFightBar();
    EXPECT_GT(gentle, 0.0f);
    while (expanded::FishingFightBar() <= expanded::kStartBar) {
        ASSERT_EQ(Pull(), FightStep::kFighting);
    }
    bar = expanded::FishingFightBar();
    Step(0.0f, 0.0f);
    EXPECT_NEAR(bar - expanded::FishingFightBar(), gentle * expanded::kDrain, 1e-6f);
}

TEST_F(FishingFight, TheBarHoldsAsTheFightStarts) {
    Strike();
    for (int i = 0; i < expanded::kStartGrace; i++) {
        EXPECT_EQ(Step(0.0f, 0.0f), FightStep::kFighting);
    }
    EXPECT_FLOAT_EQ(expanded::FishingFightBar(), expanded::kStartBar);
    Step(0.0f, 0.0f);
    EXPECT_LT(expanded::FishingFightBar(), expanded::kStartBar);
}

TEST_F(FishingFight, TheFishSwitchesSidesAndRunsToThem) {
    Strike();
    const int first = expanded::FishingFightSide();
    int       frames = 0;
    while (expanded::FishingFightSide() == first && frames < expanded::kLongestRun + 1) {
        Pull();
        frames++;
    }
    EXPECT_EQ(expanded::FishingFightSide(), -first);
    EXPECT_GE(frames, expanded::kShortestRun);
    EXPECT_LE(frames, expanded::kLongestRun);

    // The angler's right as he looks at the fish (-x, the fish out along +z): the fish heads for the side it runs to,
    // 20 out from where it was hooked, as far as the cone away from the angler (+z) lets it (the game's thrash rolled
    // straight ahead): its cone's edge on that side.
    CFish &fish = fish_[kHooked];
    fish.target_yaw = 0.0f;
    expanded::FishFightSwim(fish);
    const float side = static_cast<float>(expanded::FishingFightSide());
    EXPECT_NEAR(fish.target_yaw, expanded::FightConeYaw(0.0f, std::atan2(-side, 0.0f)), 1e-5f);
    EXPECT_NEAR(std::fabs(fish.target_yaw), expanded::kCone, 1e-5f);
    EXPECT_LT(side * std::sin(fish.target_yaw), 0.0f); // toward -x for the right
    EXPECT_FLOAT_EQ(fish.target_speed, expanded::FightRunSpeed(0.0f));
}

TEST_F(FishingFight, TheRunsStaySquareToTheLineWhicheverWayTheCameraLooks) {
    // The camera looks along +x across the line (the screen's right is +z, the way out to the fish).
    CCamera camera(1.0f);
    float   eye[4] = {-30.0f, 20.0f, 0.0f, 1.0f};
    camera.SetPos(eye);
    camera.SetRef(0.0f, 0.0f, 0.0f);
    NowCamera = &camera;
    Strike();
    Pull();
    CFish &fish = fish_[kHooked];
    fish.target_yaw = 0.0f;
    expanded::FishFightSwim(fish);
    // The runs still go to the angler's sides, square to the line (his right -x), to the cone's edge on that side.
    const float side = static_cast<float>(expanded::FishingFightSide());
    EXPECT_NEAR(fish.target_yaw, expanded::FightConeYaw(0.0f, std::atan2(-side, 0.0f)), 1e-5f);
    EXPECT_NEAR(fish.target_yaw, -side * expanded::kCone, 1e-5f);
    NowCamera = nullptr;
}

TEST_F(FishingFight, TheFishThrashesAboutItsRunsHeading) {
    Strike();
    CFish &fish = fish_[kHooked];
    fish.target_yaw = 0.0f;
    expanded::FishFightSwim(fish);
    const float heading = fish.target_yaw;
    // The game's thrash rolls a heading anywhere; the run keeps a third of it either side, inside the cone away from the
    // angler (+z).
    for (float roll : {1.0f, -2.0f, 3.0f, -3.0f}) {
        fish.target_yaw = roll;
        expanded::FishFightSwim(fish);
        EXPECT_NEAR(fish.target_yaw, expanded::FightConeYaw(0.0f, heading + roll / 3.0f), 1e-4f) << roll;
        EXPECT_LE(std::fabs(std::remainder(fish.target_yaw, 2.0f * PI)), expanded::kCone + 1e-5f) << roll;
    }
    // At its run's end (facing +z with no camera, its right is -x) it still struggles.
    const float side = static_cast<float>(expanded::FishingFightSide());
    float       end[4] = {-side * expanded::kRunWidth, -12.0f, 40.0f, 1.0f};
    fish.SetPosition(end);
    expanded::FishFightSwim(fish);
    EXPECT_GT(fish.target_speed, 0.0f);
}

TEST_F(FishingFight, TheLineHoldsTheFishToItsEnd) {
    // The line, 23 segments of distp from the rod tip, holds the fish to kLineReach times its length across the ground.
    const float saved_distp = distp;
    float       saved_rod[4];
    std::copy_n(rod_top, 4, saved_rod);
    distp = 1.5f;
    const float rod[4] = {0.0f, 10.0f, 0.0f, 1.0f};
    std::copy_n(rod, 4, rod_top);
    EXPECT_FLOAT_EQ(expanded::FightLineLength(), 23 * 1.5f);
    Strike();
    CFish &fish = fish_[kHooked];
    float  far[4] = {45.0f, -12.0f, 60.0f, 1.0f};
    fish.SetPosition(far);
    expanded::FishFightHold(fish, far, nullptr, 0);
    float held[4] = {};
    fish.GetPosition(held);
    EXPECT_NEAR(std::hypot(held[0], held[2]), 23 * 1.5f * expanded::kLineReach, 1e-3f);
    EXPECT_NEAR(std::atan2(held[0], held[2]), std::atan2(45.0f, 60.0f), 1e-5f);
    EXPECT_FLOAT_EQ(held[1], -12.0f);
    // Within it, it is left alone; so is another fish.
    float near[4] = {5.0f, -12.0f, 20.0f, 1.0f};
    fish.SetPosition(near);
    expanded::FishFightHold(fish, near, nullptr, 0);
    fish.GetPosition(held);
    EXPECT_FLOAT_EQ(held[0], 5.0f);
    EXPECT_FLOAT_EQ(held[2], 20.0f);
    fish_[0].SetPosition(far);
    expanded::FishFightHold(fish_[0], far, nullptr, 0);
    fish_[0].GetPosition(held);
    EXPECT_FLOAT_EQ(held[0], 45.0f);
    distp = saved_distp;
    std::copy_n(saved_rod, 4, rod_top);
}

// A wall triangle through a, b and c, facing normal.
CCPoly Wall(std::array<float, 3> a, std::array<float, 3> b, std::array<float, 3> c, std::array<float, 3> normal) {
    CCPoly                              poly{};
    std::array<std::array<float, 3>, 3> corners{a, b, c};
    for (int i = 0; i < 3; i++) {
        std::copy_n(corners[i].data(), 3, poly.vertex[i]);
        poly.vertex[i][3] = 1.0f;
    }
    std::copy_n(normal.data(), 3, poly.normal);
    return poly;
}

TEST_F(FishingFight, ABankInTheWayHoldsTheFishAsWellAsItCan) {
    // The line (23 segments) from a rod tip over the angler reaches 34.5; the fish past it at 40.
    const float saved_distp = distp;
    float       saved_rod[4];
    std::copy_n(rod_top, 4, saved_rod);
    distp = 34.5f / (23.0f * expanded::kLineReach);
    ASSERT_FLOAT_EQ(expanded::FightLineLength() * expanded::kLineReach, 34.5f);
    const float rod[4] = {0.0f, 10.0f, 0.0f, 1.0f};
    std::copy_n(rod, 4, rod_top);
    Strike();
    CFish &fish = fish_[kHooked];
    float  out[4] = {0.0f, -12.0f, 40.0f, 1.0f};
    float  held[4] = {};

    // A bank square across the pull at z 38: no way in, so it ends its step where it started, nearer the rod.
    CCPoly across = Wall({-100.0f, -100.0f, 38.0f}, {100.0f, -100.0f, 38.0f}, {0.0f, 100.0f, 38.0f}, {0.0f, 0.0f, 1.0f});
    float  start[4] = {0.0f, -12.0f, 39.5f, 1.0f};
    fish.SetPosition(out);
    expanded::FishFightHold(fish, start, &across, 1);
    fish.GetPosition(held);
    EXPECT_FLOAT_EQ(held[0], 0.0f);
    EXPECT_FLOAT_EQ(held[2], 39.5f);
    // Started farther out, it stays where its swim left it, still on its side of the bank.
    float farther[4] = {0.0f, -12.0f, 41.0f, 1.0f};
    fish.SetPosition(out);
    expanded::FishFightHold(fish, farther, &across, 1);
    fish.GetPosition(held);
    EXPECT_FLOAT_EQ(held[2], 40.0f);

    // A bank at a slant (x + z = 38): it slides along it, nearer the rod, without crossing it.
    const float slant = 1.0f / std::sqrt(2.0f);
    CCPoly      sloped = Wall({-62.0f, -100.0f, 100.0f}, {138.0f, -100.0f, -100.0f}, {38.0f, 100.0f, 0.0f}, {slant, 0.0f, slant});
    fish.SetPosition(out);
    expanded::FishFightHold(fish, farther, &sloped, 1);
    fish.GetPosition(held);
    EXPECT_LT(std::hypot(held[0], held[2]), 40.0f);
    EXPECT_GE(held[0] + held[2], 38.0f);
    EXPECT_GT(held[0], 0.0f);

    distp = saved_distp;
    std::copy_n(saved_rod, 4, rod_top);
}

TEST_F(FishingFight, PushingAsideLeavesTheAnglerTurnedAsHeIs) {
    Strike();
    Step(1.0f, 1.0f);
    EXPECT_FLOAT_EQ(facing_[1], 0.0f);
}

TEST_F(FishingFight, DebugModeAlwaysPulls) {
    DebugMode = 1;
    Strike();
    FightStep end = FightStep::kFighting;
    int       frames = 0;
    while (end == FightStep::kFighting && frames < 2000) {
        end = Step(0.0f, 0.0f);
        frames++;
    }
    EXPECT_EQ(end, FightStep::kLanded);
}

TEST_F(FishingFight, OffTheStruckFishIsLandedAsInTheGame) {
    ASSERT_TRUE(options_.Set(expanded::kFishingFights, false));
    Strike();
    EXPECT_EQ(BattleFish, &fish_[kHooked]);
    EXPECT_EQ(Step(0.0f, 0.0f), FightStep::kRetail);
    EXPECT_EQ(expanded::FishingFightSide(), 0);
}

TEST_F(FishingFight, OnlyTheFoughtFishIsSteered) {
    Strike();
    fish_[0].target_yaw = 9.0f;
    EXPECT_FLOAT_EQ(expanded::FishFightSwim(fish_[0]), 1.0f);
    EXPECT_FLOAT_EQ(fish_[0].target_yaw, 9.0f);
    // The fought fish turns and speeds up at the fight's pace.
    EXPECT_FLOAT_EQ(expanded::FishFightSwim(fish_[kHooked]), expanded::kFightPace);
}

} // namespace
