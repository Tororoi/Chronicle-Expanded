#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

#include "camera.hpp"
#include "camerafollow.hpp"
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

// The stick across (as read) that puts the block at along on the gauge.
float StickAt(float along) {
    return along / expanded::kCone;
}

// The stick across that puts the block as far from the mark at target as the gauge allows: at least kCone off it.
float StickAwayFrom(float target) {
    return target > 0.0f ? -1.0f : 1.0f;
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

    // A frame with the block on the mark, pulled back (back) or not.
    FightStep Pull(bool back = false) { return Step(StickAt(expanded::FishingFightTarget()), back ? 1.0f : 0.0f); }

    // A frame with the block far off the mark, pulled back (back) or not.
    FightStep Miss(bool back = false) { return Step(StickAwayFrom(expanded::FishingFightTarget()), back ? 1.0f : 0.0f); }

    // The frames of the fight's start, the bar held, missing.
    void PastTheStart() {
        for (int i = 0; i < expanded::kStartGrace; i++) {
            Miss();
        }
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
    EXPECT_EQ(expanded::FightRunFrames(0.0f, 0.0f), 60);
    EXPECT_EQ(expanded::FightRunFrames(0.0f, 1.0f), 120);
    EXPECT_EQ(expanded::FightRunFrames(2.0f, 0.0f), 42);
    EXPECT_EQ(expanded::FightRunFrames(2.0f, 1.0f), 84);
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

TEST(FishingFightRules, TheMarkIsAgainstTheFishAsTheCameraSeesIt) {
    // The camera looking +z (its right -x).
    // A fish swimming straight away from the camera: the middle.
    EXPECT_NEAR(expanded::FightTargetAlong(0.0f, 0.0f), 0.0f, 1e-5f);
    // Swimming away and to the screen's left (+x): pulled from the right, the yaw's way.
    EXPECT_NEAR(expanded::FightTargetAlong(0.0f, 0.5f), 0.5f, 1e-5f);
    EXPECT_NEAR(expanded::FightTargetAlong(0.0f, -0.5f), -0.5f, 1e-5f);
    // Square across or toward the camera: held at the arc's ends.
    EXPECT_NEAR(expanded::FightTargetAlong(0.0f, -PI / 2.0f), -expanded::kCone, 1e-5f);
    EXPECT_NEAR(expanded::FightTargetAlong(0.0f, 2.5f), expanded::kCone, 1e-5f);
    // Only the camera's way counts: looking +x, a fish swimming +x is in the middle, one turning toward -z (the screen's
    // left) is pulled from the right, round the turn of the circle too.
    EXPECT_NEAR(expanded::FightTargetAlong(PI / 2.0f, PI / 2.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(expanded::FightTargetAlong(PI / 2.0f, PI / 2.0f + 0.3f), 0.3f, 1e-5f);
    EXPECT_NEAR(expanded::FightTargetAlong(3.0f, -3.0f), 2.0f * PI - 6.0f, 1e-5f);
}

TEST(FishingFightRules, TheMarkEasesAndSlides) {
    const float ease = expanded::kTargetEase;
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.0f, 0.2f), 0.2f * ease);
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.5f, 0.5f), 0.5f);
    // A far turn (the fish switching sides) slides at kTargetTurn a frame.
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.0f, 1.0f), expanded::kTargetTurn);
    EXPECT_FLOAT_EQ(expanded::FightEaseTarget(0.0f, -1.0f), -expanded::kTargetTurn);
}

TEST(FishingFightRules, TheStickMovesTheBlockLeftAndRight) {
    EXPECT_FLOAT_EQ(expanded::FightStickAlong(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::FightStickAlong(1.0f), expanded::kCone);
    EXPECT_FLOAT_EQ(expanded::FightStickAlong(-0.5f), -expanded::kCone / 2.0f);
    EXPECT_FLOAT_EQ(expanded::FightStickAlong(-1.5f), -expanded::kCone);
}

TEST(FishingFightRules, TheBlockPullsOnTheMarkAndHoldsNearIt) {
    const float pull = expanded::kPullTolerance, hold = expanded::kHoldTolerance;
    // At rest, on a mark in the middle: no push needed.
    EXPECT_TRUE(expanded::FightPulls(0.0f, 0.0f));
    for (float target : {0.0f, 0.6f, -expanded::kCone}) {
        EXPECT_TRUE(expanded::FightPulls(StickAt(target), target)) << target;
        for (float side : {1.0f, -1.0f}) {
            const float x = StickAt(target + side * (pull - 0.01f));
            if (std::fabs(x) <= 1.0f) {
                EXPECT_TRUE(expanded::FightPulls(x, target)) << target;
            }
            // Between the two tolerances: it holds and does not pull.
            const float near = StickAt(target + side * (pull + hold) / 2.0f);
            if (std::fabs(near) <= 1.0f) {
                EXPECT_FALSE(expanded::FightPulls(near, target)) << target;
                EXPECT_TRUE(expanded::FightHolds(near, target)) << target;
            }
            const float off = StickAt(target + side * (hold + 0.01f));
            if (std::fabs(off) <= 1.0f) {
                EXPECT_FALSE(expanded::FightHolds(off, target)) << target;
            }
        }
    }
    // Only left and right count, the push back none.
    EXPECT_TRUE(expanded::FightPulls(StickAt(0.6f), 0.6f));
    // A pull holds too.
    EXPECT_TRUE(expanded::FightHolds(0.0f, 0.0f));
}

TEST(FishingFightRules, PulledBackIsPastThePullBack) {
    EXPECT_FALSE(expanded::FightPullsBack(0.0f));
    EXPECT_FALSE(expanded::FightPullsBack(-1.0f)); // pushed forward
    EXPECT_FALSE(expanded::FightPullsBack(expanded::kPullBack));
    EXPECT_TRUE(expanded::FightPullsBack(expanded::kPullBack + 0.01f));
}

TEST(FishingFightRules, TheGaugeLiesFlatOnTheWater) {
    const float centre[4] = {10.0f, -2.0f, 40.0f, 1.0f};
    float       point[4] = {};
    // Its middle points back toward the camera (-z): radius out that way, at the centre's height.
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

TEST_F(FishingFight, TheFishStartsStraightAwaySoTheStickAtRestPulls) {
    // The fish out along +z from the angler, turned any way as it bit: struck, it faces straight away, and with the
    // camera behind the angler the mark sits in the middle, on the block of a stick at rest.
    float turned[4] = {0.0f, 2.0f, 0.0f, 0.0f};
    fish_[kHooked].SetRotation(turned);
    Strike();
    float heading[4] = {};
    fish_[kHooked].GetRotation(heading);
    EXPECT_FLOAT_EQ(heading[1], 0.0f);
    EXPECT_FLOAT_EQ(expanded::FishingFightTarget(), 0.0f);
    // It keeps swimming straight away while the bar holds, whatever the game's thrash rolls.
    CFish &fish = fish_[kHooked];
    for (int i = 0; i < expanded::kStartGrace; i++) {
        fish.target_yaw = 2.5f;
        expanded::FishFightSwim(fish);
        EXPECT_FLOAT_EQ(fish.target_yaw, 0.0f);
        EXPECT_EQ(Step(0.0f, 0.0f), FightStep::kFighting);
    }
    EXPECT_FLOAT_EQ(expanded::FishingFightTarget(), 0.0f);
    EXPECT_EQ(Step(0.0f, 0.0f), FightStep::kFighting);
    EXPECT_GT(expanded::FishingFightBar(), expanded::kStartBar);
}

TEST_F(FishingFight, TheCameraTurnsToLookAlongTheLine) {
    // The fishing camera looking +x; the fish out along +z from the angler. Struck, the camera turns to look +z (its
    // eye toward -z), over its steps.
    CCameraFollow camera(30.0f, 20.0f, -PI / 2.0f, 8.0f);
    camera.SetFollow(0.0f, 0.0f, 36.0f);
    camera.Step(-1);
    Strike();
    expanded::FishingFightTurnCamera(camera);
    EXPECT_NEAR(std::remainder(camera.next_angle - PI, 2.0f * PI), 0.0f, 1e-5f);
    camera.Step(1);
    EXPECT_GT(std::fabs(std::remainder(camera.GetAngle() - PI, 2.0f * PI)), 0.1f); // not at once
    for (int i = 0; i < 60; i++) {
        camera.Step(1);
    }
    EXPECT_NEAR(std::remainder(camera.GetAngle() - PI, 2.0f * PI), 0.0f, 1e-3f);
    // With no fight it is left alone.
    ASSERT_TRUE(options_.Set(expanded::kFishingFights, false));
    expanded::FishingFightStarted(fish_[0]);
    camera.SetAngle(1.0f);
    expanded::FishingFightTurnCamera(camera);
    EXPECT_FLOAT_EQ(camera.next_angle, 1.0f);
}

TEST_F(FishingFight, TheMarkFollowsTheFish) {
    Strike();
    // The fish swimming away and to +x (the screen's left): the mark eases round to the right, never past the arc.
    float heading[4] = {0.0f, 0.8f, 0.0f, 0.0f};
    fish_[kHooked].SetRotation(heading);
    float previous = 0.0f;
    for (int i = 0; i < 60; i++) {
        Pull();
        const float mark = expanded::FishingFightTarget();
        EXPECT_GE(mark, previous - 1e-5f);
        EXPECT_LE(mark, 0.8f + 1e-5f);
        previous = mark;
    }
    EXPECT_NEAR(previous, 0.8f, 0.01f);
    // Swimming square across to -x: held at the arc's left end.
    heading[1] = -PI / 2.0f;
    fish_[kHooked].SetRotation(heading);
    for (int i = 0; i < 120; i++) {
        Pull();
    }
    EXPECT_NEAR(expanded::FishingFightTarget(), -expanded::kCone, 0.01f);
}

TEST_F(FishingFight, TheGaugeFacesTheCamera) {
    // The camera kept 90 degrees round, looking +x across the line: the fish swimming straight out (+z) swims to the
    // screen's right, so the mark slides to the left end and the stick pulls left, not at rest.
    CCamera camera(1.0f);
    float   eye[4] = {-30.0f, 20.0f, 20.0f, 1.0f};
    camera.SetPos(eye);
    camera.SetRef(0.0f, 0.0f, 20.0f);
    NowCamera = &camera;
    Strike();
    for (int i = 0; i < expanded::kStartGrace; i++) {
        Step(0.0f, 0.0f);
    }
    EXPECT_NEAR(expanded::FishingFightTarget(), -expanded::kCone, 0.02f);
    Step(0.0f, 0.0f);
    EXPECT_LT(expanded::FishingFightBar(), expanded::kStartBar);
    const float down = expanded::FishingFightBar();
    Step(-1.0f, 0.0f);
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

TEST_F(FishingFight, MissingLosesTheFish) {
    Strike();
    FightStep end = FightStep::kFighting;
    int       frames = 0;
    while (end == FightStep::kFighting && frames < 2000) {
        end = Miss();
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
    PastTheStart();
    // Halfway between the two tolerances off the mark (which moves no more than kTargetTurn a frame), on whichever
    // side the gauge has room, the bar neither fills nor drains.
    const float off = (expanded::kPullTolerance + expanded::kHoldTolerance) / 2.0f;
    for (int i = 0; i < 30; i++) {
        const float bar = expanded::FishingFightBar();
        const float target = expanded::FishingFightTarget();
        const float side = (i % 2 == 0) == (target + off <= expanded::kCone) ? 1.0f : -1.0f;
        EXPECT_EQ(Step(StickAt(target + side * off), 0.0f), FightStep::kFighting);
        EXPECT_FLOAT_EQ(expanded::FishingFightBar(), bar) << i;
    }
}

TEST_F(FishingFight, AMissDrainsTwiceAsFastOnceTheBarHasRisen) {
    Strike();
    PastTheStart();
    float bar = expanded::FishingFightBar();
    Miss();
    const float fill = bar - expanded::FishingFightBar();
    EXPECT_NEAR(fill, expanded::FightFill(0.0f), 1e-6f);
    while (expanded::FishingFightBar() <= expanded::kStartBar) {
        ASSERT_EQ(Pull(), FightStep::kFighting);
    }
    bar = expanded::FishingFightBar();
    Miss();
    EXPECT_NEAR(bar - expanded::FishingFightBar(), fill * expanded::kDrain, 1e-6f);
}

TEST_F(FishingFight, PullingBackMakesTheBarGoFasterEitherWay) {
    Strike();
    PastTheStart();
    const float fill = expanded::FightFill(0.0f);
    float       bar = expanded::FishingFightBar();
    Miss(true);
    EXPECT_NEAR(bar - expanded::FishingFightBar(), fill * expanded::kPullBoost, 1e-6f);
    bar = expanded::FishingFightBar();
    Pull(true);
    EXPECT_NEAR(expanded::FishingFightBar() - bar, fill * expanded::kPullBoost, 1e-6f);
    while (expanded::FishingFightBar() <= expanded::kStartBar) {
        ASSERT_EQ(Pull(), FightStep::kFighting);
    }
    // Once the bar has risen, a miss pulled back drains three times as fast as a pull fills.
    bar = expanded::FishingFightBar();
    Miss(true);
    EXPECT_NEAR(bar - expanded::FishingFightBar(), fill * expanded::kDrain * expanded::kPullBoost, 1e-6f);
}

TEST_F(FishingFight, TheBarHoldsAsTheFightStarts) {
    Strike();
    for (int i = 0; i < expanded::kStartGrace; i++) {
        EXPECT_EQ(Miss(), FightStep::kFighting);
    }
    EXPECT_FLOAT_EQ(expanded::FishingFightBar(), expanded::kStartBar);
    Miss();
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
    PastTheStart();
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
    PastTheStart();
    Pull();
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
