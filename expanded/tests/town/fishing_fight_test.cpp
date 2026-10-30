#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "camera.hpp"
#include "character.hpp"
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
    EXPECT_FLOAT_EQ(expanded::FightRunSpeed(0.0f), 0.35f);
    EXPECT_FLOAT_EQ(expanded::FightRunSpeed(2.0f), 0.525f);
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

    // With no camera, the angler's right as he faces now (-x facing +z): the fish heads to the side it runs to, 20 out
    // from where it was hooked (the game's thrash rolled straight ahead).
    CFish &fish = fish_[kHooked];
    fish.target_yaw = 0.0f;
    expanded::FishFightSwim(fish);
    const float side = static_cast<float>(expanded::FishingFightSide());
    const float yaw = angler_.GetRotation()->y;
    EXPECT_NEAR(fish.target_yaw, std::atan2(-side * std::cos(yaw), side * std::sin(yaw)), 1e-5f);
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
    const float side = static_cast<float>(expanded::FishingFightSide());
    EXPECT_NEAR(fish.target_yaw, std::atan2(0.0f, side * expanded::kRunWidth), 1e-5f);
    NowCamera = nullptr;
}

TEST_F(FishingFight, TheFishThrashesAboutItsRunsHeading) {
    Strike();
    CFish &fish = fish_[kHooked];
    fish.target_yaw = 0.0f;
    expanded::FishFightSwim(fish);
    const float heading = fish.target_yaw;
    // The game's thrash rolls a heading anywhere; the run keeps half of it either side.
    for (float roll : {1.0f, -2.0f, 3.0f}) {
        fish.target_yaw = roll;
        expanded::FishFightSwim(fish);
        EXPECT_NEAR(std::remainder(fish.target_yaw - heading, 2.0f * PI), roll * 0.5f, 1e-4f) << roll;
    }
    // At its run's end (facing +z with no camera, its right is -x) it still struggles.
    const float side = static_cast<float>(expanded::FishingFightSide());
    float       end[4] = {-side * expanded::kRunWidth, -12.0f, 40.0f, 1.0f};
    fish.SetPosition(end);
    expanded::FishFightSwim(fish);
    EXPECT_GT(fish.target_speed, 0.0f);
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
    expanded::FishFightSwim(fish_[0]);
    EXPECT_FLOAT_EQ(fish_[0].target_yaw, 9.0f);
}

} // namespace
