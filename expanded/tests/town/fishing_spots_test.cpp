#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include "collision.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/town/fishing.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "main.hpp"

namespace {

using expanded::scene::Triangle;

constexpr int kQueens = 2, kQueensSpot = 6;

// MapNo for a test, put back after.
class Town {
public:
    explicit Town(int map_no) : saved_(MapNo) { MapNo = map_no; }

    ~Town() { MapNo = saved_; }

    Town(const Town &) = delete;
    Town &operator=(const Town &) = delete;

private:
    int saved_;
};

// A wall across +Z at z, x and y from -20 to 40.
std::vector<Triangle> WallAt(double z) {
    return {
        {{{-20, -20, z}, {20, -20, z}, {20, 40, z}}},
        {{{-20, -20, z}, {20, 40, z}, {-20, 40, z}}},
    };
}

} // namespace

TEST(FishingSpots, QueensFishAreBobos) {
    for (int roll : {0, 1, 2, 3, 49, 50, 12345}) {
        EXPECT_EQ(expanded::SpotFish(kQueensSpot, roll, -1), expanded::fish::kBobo) << roll;
    }
    EXPECT_EQ(expanded::SpotFishCount(kQueensSpot, 5), expanded::kSpotFishCount);
}

TEST(FishingSpots, TheGamesSpotsKeepTheirFishAndLine) {
    const float retail = distp;
    for (int spot = 0; spot <= 4; spot++) {
        EXPECT_EQ(expanded::SpotFish(spot, 3, 7), 7) << spot;
        EXPECT_EQ(expanded::SpotFishCount(spot, 4), 4) << spot;
        EXPECT_EQ(expanded::SpotLineSegment(spot), retail) << spot;
    }
    Town norune(0);
    EXPECT_EQ(expanded::SpotFishDepth(12.0f), 12.0f);
}

TEST(FishingSpots, QueensLineReachesTheCanalAndItsFishTheHook) {
    const expanded::FishingSpot *queens = expanded::FishingSpotOfMap(kQueens);
    ASSERT_NE(queens, nullptr);
    EXPECT_FLOAT_EQ(expanded::SpotLineSegment(kQueensSpot), distp * queens->line);
    EXPECT_GT(queens->line, 1.0f); // the bank stands high over the water
    Town town(kQueens);
    EXPECT_EQ(expanded::SpotFishDepth(12.0f), queens->fish_depth);
}

TEST(FishingSpots, AddedCollisionIsWholePolysAsFarAsTheBufferGoes) {
    std::array<CCPoly, 4> polys{};
    polys[0].attr.ground_kind = 3;
    std::vector<Triangle> triangles = WallAt(5.0);
    const Triangle        line = {
        {{0, 0, 0}, {1, 1, 1}, {2, 2, 2}}
    };
    triangles.insert(triangles.begin() + 1, line); // no area: left out
    triangles.push_back(WallAt(9.0)[0]);
    EXPECT_EQ(expanded::AddCollision(polys.data(), 1, 3, triangles), 3);
    for (int i : {1, 2}) {
        const CCPoly &poly = polys[static_cast<std::size_t>(i)];
        EXPECT_EQ(poly.vertex[0][2], 5.0f) << i;
        EXPECT_EQ(poly.vertex[0][3], 1.0f) << i;
        EXPECT_FLOAT_EQ(std::abs(poly.normal[2]), 1.0f) << i; // a unit normal across the wall
        EXPECT_EQ(poly.attr.ground_kind, 3) << i;
    }
    EXPECT_EQ(expanded::AddCollision(polys.data(), 3, 3, triangles), 3);
}

TEST(FishingSpots, NoTownSpotAddsNoCollision) {
    Town                  norune(0);
    std::array<CCPoly, 2> polys{};
    EXPECT_EQ(expanded::AddSpotCollision(polys.data(), 1, 2), 1);
}

// A fish swimming +Z is stopped by a thin wall across its way, as one swimming -Z is.
TEST(FishingSpots, AFishSwimmingPlusZMeetsAThinWall) {
    for (float direction : {1.0f, -1.0f}) {
        std::array<CCPoly, 2> wall{};
        ASSERT_EQ(expanded::AddCollision(wall.data(), 0, 2, WallAt(3.0 * direction)), 2);
        auto fish = std::make_unique<CFish>();
        fish->fish_kind = 0;
        fish->food_kind = -1;
        fish->action = FISH_ACTION_NONE;
        fish->action_timer = 1000;
        fish->move_mode = FISH_MOVE_SWIM;
        fish->move_timer = 1000;
        fish->target_yaw = direction > 0 ? 0.0f : 3.14159265f;
        fish->target_speed = fish->speed = 0.5f;
        fish->angle_model_scale = fish->model_scale = 1.0f;
        float start[4] = {0.0f, 19.0f, 0.0f, 1.0f}, turn[4] = {0.0f, fish->target_yaw, 0.0f, 0.0f};
        fish->SetPosition(start);
        fish->SetRotation(turn);
        fish->SetCPoly(wall.data(), 2);
        for (int step = 0; step < 40; step++) {
            fish->Step();
        }
        float at[4];
        fish->GetPosition(at);
        EXPECT_LT(at[2] * direction, 3.0f) << direction;
    }
}
