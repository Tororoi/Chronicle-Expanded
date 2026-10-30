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

// The stick pulling against a fish running to side, and left alone.
constexpr float kBack = 0.8f, kAside = 0.8f;

// Six fish, the hooked one a usual-sized Nonky out from the angler, who stands at the origin facing +z; no camera.
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
    }

    void TearDown() override {
        Fish = saved_fish_;
        Chara = saved_chara_;
        BattleFish = saved_battle_;
        NowCamera = saved_camera_;
        DebugMode = saved_debug_;
    }

    void Strike() { FishingBattleFish(kHooked); }

    // A frame of the fight with the stick at (x, y).
    FightStep Step(float x, float y) { return expanded::FishingFightStep(angler_, facing_, x, y); }

    // A frame pulling against the fish's run.
    FightStep Pull() { return Step(-static_cast<float>(expanded::FishingFightSide()) * kAside, kBack); }

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

TEST(FishingFightRules, OnlyBackAndAwayFromTheFishPulls) {
    // Down is positive: the stick pulled back toward the player.
    EXPECT_TRUE(expanded::FightPulls(0.8f, 0.8f, -1));   // running left: down and right
    EXPECT_TRUE(expanded::FightPulls(-0.8f, 0.8f, 1));   // running right: down and left
    EXPECT_FALSE(expanded::FightPulls(-0.8f, 0.8f, -1)); // toward the fish
    EXPECT_FALSE(expanded::FightPulls(0.8f, -0.8f, -1)); // pushed forward
    EXPECT_FALSE(expanded::FightPulls(0.0f, 1.0f, -1));  // straight back
    EXPECT_FALSE(expanded::FightPulls(1.0f, 0.0f, -1));  // straight aside
    EXPECT_FALSE(expanded::FightPulls(0.0f, 0.0f, 1));   // let go
}

TEST_F(FishingFight, AStruckFishIsFoughtFromAThirdOfTheBar) {
    Strike();
    EXPECT_EQ(BattleFish, &fish_[kHooked]);
    EXPECT_FLOAT_EQ(expanded::FishingFightBar(), expanded::kStartBar);
    EXPECT_NE(expanded::FishingFightSide(), 0);
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
    const int drain = static_cast<int>(std::ceil(expanded::kStartBar * expanded::kFillFrames / expanded::kDrain));
    EXPECT_GE(frames, drain + expanded::kStartGrace);
    EXPECT_LE(frames, drain + expanded::kStartGrace + expanded::kSwitchGrace + 1);
    // Off it swims, a fish again.
    EXPECT_EQ(BattleFish, nullptr);
    EXPECT_EQ(fish_[kHooked].action, FISH_ACTION_LEAVE);
    EXPECT_EQ(fish_[kHooked].move_mode, FISH_MOVE_SWIM);
    EXPECT_EQ(expanded::FishingFightSide(), 0);
    EXPECT_EQ(Step(0.0f, 0.0f), FightStep::kRetail);
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

    // With no camera, the angler's right as he faces now (-x facing +z): the fish heads for the side it runs to, 20 out
    // from where it was hooked, as far as the cone away from the angler (+z) lets it (the game's thrash rolled straight
    // ahead): its cone's edge on that side.
    CFish &fish = fish_[kHooked];
    fish.target_yaw = 0.0f;
    expanded::FishFightSwim(fish);
    const float side = static_cast<float>(expanded::FishingFightSide());
    const float yaw = angler_.GetRotation()->y;
    const float run = std::atan2(-side * std::cos(yaw), side * std::sin(yaw));
    EXPECT_NEAR(fish.target_yaw, expanded::FightConeYaw(0.0f, run), 1e-5f);
    EXPECT_NEAR(std::fabs(fish.target_yaw), expanded::kCone, 1e-5f);
    EXPECT_LT(side * std::sin(fish.target_yaw), 0.0f); // toward -x for the right
    EXPECT_FLOAT_EQ(fish.target_speed, expanded::FightRunSpeed(0.0f));
}

TEST_F(FishingFight, TheFishRunsToTheScreensSides) {
    // The camera looks along +x from behind the angler: the screen's right is +z.
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
    // The run's end lies 20 along +z for the right (straight away from the angler) and 20 back for the left, where the
    // cone stops it at an edge.
    const float side = static_cast<float>(expanded::FishingFightSide());
    EXPECT_NEAR(fish.target_yaw, expanded::FightConeYaw(0.0f, std::atan2(0.0f, side * expanded::kRunWidth)), 1e-5f);
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
    // The line (23 segments of 1, 23) from a rod tip over the angler reaches 34.5; the fish past it at 40.
    const float saved_distp = distp;
    float       saved_rod[4];
    std::copy_n(rod_top, 4, saved_rod);
    distp = 1.0f;
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

TEST_F(FishingFight, PushingAsideTurnsTheAngler) {
    Strike();
    Step(1.0f, 1.0f);
    EXPECT_FLOAT_EQ(facing_[1], -0.01f);
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
