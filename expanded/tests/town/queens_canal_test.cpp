#include <gtest/gtest.h>

#include <utility>

#include "editloop.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/data/queens_canal.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/town/queens_canal.hpp"
#include "main.hpp"

namespace {

constexpr int kQueensBank = 6, kQueensFloor = 8;
// The game's clock: an hour into each part of the day.
constexpr float kMorning = 9.0f, kDay = 0.0f, kEvening = 3.0f, kNight = 6.0f;

// MapNo and the clock for a test, put back after.
class Town {
public:
    Town(int map_no, float time) : map_no_(MapNo), time_(NowTime) {
        MapNo = map_no;
        NowTime = time;
    }

    ~Town() {
        MapNo = map_no_;
        NowTime = time_;
    }

    Town(const Town &) = delete;
    Town &operator=(const Town &) = delete;

private:
    int   map_no_;
    float time_;
};

ED_EVENT_POINT Point(int type, float x, int label = 0) {
    ED_EVENT_POINT point{};
    point.enabled = 1;
    point.event_type = type;
    point.position[0] = x;
    point.side = label;
    return point;
}

} // namespace

TEST(QueensCanal, TheTideFollowsTheClock) {
    for (float time : {kMorning, 8.5f, 11.49f}) {
        EXPECT_EQ(expanded::CanalTideLevel(time), expanded::kLowTide) << time;
    }
    for (float time : {kDay, kNight, 11.5f, 2.49f, 5.5f, 8.49f}) {
        EXPECT_EQ(expanded::CanalTideLevel(time), expanded::kMidTide) << time;
    }
    for (float time : {kEvening, 2.5f, 5.49f}) {
        EXPECT_EQ(expanded::CanalTideLevel(time), expanded::kHighTide) << time;
    }
    EXPECT_LT(expanded::kLowTide, expanded::kMidTide);
    EXPECT_LT(expanded::kMidTide, expanded::kHighTide);
}

TEST(QueensCanal, TheLadderGoesDownOnlyAtLowTide) {
    const ED_EVENT_POINT top = Point(ED_EVENT_POINT_LADDER_TOP, expanded::kLadderTop[0]);
    const ED_EVENT_POINT bottom = Point(ED_EVENT_POINT_LADDER_BOTTOM, expanded::kLadderBottom[0]);
    const ED_EVENT_POINT message = Point(ED_EVENT_POINT_EVENT, expanded::kLadderTop[0], expanded::kLadderMessageLabel);
    {
        Town queens(expanded::kQueensMapNo, kMorning);
        EXPECT_TRUE(expanded::CanalEventPointOn(top));
        EXPECT_TRUE(expanded::CanalEventPointOn(bottom));
        EXPECT_FALSE(expanded::CanalEventPointOn(message));
    }
    for (float time : {kDay, kEvening, kNight}) {
        Town queens(expanded::kQueensMapNo, time);
        EXPECT_FALSE(expanded::CanalEventPointOn(top)) << time;
        EXPECT_TRUE(expanded::CanalEventPointOn(bottom)) << time; // a player left down there can always climb out
        EXPECT_TRUE(expanded::CanalEventPointOn(message)) << time;
    }
    // Other points, and other towns' ladders, are left as they are.
    Town queens(expanded::kQueensMapNo, kEvening);
    EXPECT_TRUE(expanded::CanalEventPointOn(Point(ED_EVENT_POINT_LADDER_TOP, 100.0f)));
    EXPECT_TRUE(expanded::CanalEventPointOn(Point(ED_EVENT_POINT_EVENT, expanded::kLadderTop[0], expanded::kFishingSignLabel)));
    Town norune(0, kEvening);
    EXPECT_TRUE(expanded::CanalEventPointOn(top));
}

TEST(QueensCanal, QueensFishingWaterFollowsTheTide) {
    for (auto [time, level] : {
             std::pair{kMorning, expanded::kLowTide },
             std::pair{kDay,     expanded::kMidTide },
             std::pair{kEvening, expanded::kHighTide},
             std::pair{kNight,   expanded::kMidTide }
    }) {
        Town queens(expanded::kQueensMapNo, time);
        EXPECT_EQ(expanded::FishingSpotLoad(kQueensBank, 31.0f), level) << time;
        EXPECT_EQ(expanded::FishingSpotLoad(kQueensFloor, 8.0f), level) << time;
    }
    Town norune(0, kMorning);
    EXPECT_EQ(expanded::FishingSpotLoad(0, 20.0f), 20.0f);
}

TEST(QueensCanal, TheTideSetsTheFishsDepthAndEachSignItsCamera) {
    const expanded::FishingSpot *bank = expanded::FishingSpotOfArea(kQueensBank);
    const expanded::FishingSpot *floor = expanded::FishingSpotOfArea(kQueensFloor);
    ASSERT_TRUE(bank && floor);
    EXPECT_GT(expanded::kLowTide - expanded::kLowTideFishDepth, floor->ground); // the fish swim over the canal floor
    {
        Town queens(expanded::kQueensMapNo, kMorning);
        expanded::FishingSpotLoad(kQueensFloor, floor->water);
        EXPECT_EQ(expanded::SpotFishDepth(12.0f), expanded::kLowTideFishDepth);
        EXPECT_NEAR(expanded::LineSegment(19), (expanded::kLowTideFishDepth - 3.47f) / 5.0f, 1e-5f); // the bait at the fish
        EXPECT_EQ(expanded::FishingCameraHeight(40.0f), floor->camera_height);
        expanded::FishingSpotLoad(kQueensBank, bank->water);
        EXPECT_EQ(expanded::SpotFishDepth(12.0f), expanded::kLowTideFishDepth);
    }
    for (float time : {kDay, kEvening, kNight}) {
        Town queens(expanded::kQueensMapNo, time);
        expanded::FishingSpotLoad(kQueensBank, bank->water);
        EXPECT_EQ(expanded::SpotFishDepth(5.0f), bank->fish_depth) << time;
    }
    // The spot loaded is fished only in its town.
    Town norune(0, kMorning);
    EXPECT_EQ(expanded::SpotFishDepth(5.0f), 5.0f);
    EXPECT_EQ(expanded::FishingCameraHeight(40.0f), 40.0f);
    expanded::FishingSpotLoad(0, 20.0f);
}

TEST(QueensCanal, OnlyTheMistLeansSpray) {
    float velocity[4] = {0.1f, 1.2f, -0.5f, 0.0f};
    expanded::SprayLean(velocity);
    EXPECT_EQ(velocity[0], 0.1f);
    EXPECT_EQ(velocity[1], 1.2f);
    EXPECT_EQ(velocity[2], -0.5f);
}
