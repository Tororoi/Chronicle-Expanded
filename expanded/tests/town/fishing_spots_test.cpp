#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include "collision.hpp"
#include "expanded/data/fishing_collision.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/town/fishing.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "main.hpp"

namespace {

using expanded::scene::Triangle;

constexpr int kBrownboo = 14, kBrownbooSpot = 5, kQueens = 2, kQueensSpot = 6;

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

TEST(FishingSpots, BrownbooFishArePiccolysNegiesAndGummiesAndOneInFiftyAGarayan) {
    using namespace expanded::fish;
    EXPECT_EQ(expanded::SpotFish(kBrownbooSpot, 1, -1), kPiccoly);
    EXPECT_EQ(expanded::SpotFish(kBrownbooSpot, 2, -1), kNegie);
    EXPECT_EQ(expanded::SpotFish(kBrownbooSpot, 3, -1), kGummy);
    EXPECT_EQ(expanded::SpotFish(kBrownbooSpot, 5, -1), kPiccoly);
    for (int roll : {0, 50, 12350}) {
        int kind = expanded::SpotFish(kBrownbooSpot, roll, -1);
        EXPECT_TRUE(kind == kMardanGarayan || kind == kBaronGarayan) << roll;
    }
    EXPECT_EQ(expanded::SpotFishCount(kBrownbooSpot, 3), expanded::kSpotFishCount);
}

TEST(FishingSpots, BrownbooFishesWithTheGamesLineAndDepth) {
    EXPECT_EQ(expanded::SpotLineSegment(kBrownbooSpot), distp);
    Town brownboo(kBrownboo);
    EXPECT_EQ(expanded::SpotFishDepth(12.0f), 12.0f);
}

// Every face of the pond's rocks is a whole poly in the fishing collision.
TEST(FishingSpots, BrownboosRocksAreWholePolys) {
    std::vector<Triangle> rocks = expanded::BrownbooRocks();
    std::vector<CCPoly>   polys(1228);
    EXPECT_EQ(expanded::AddCollision(polys.data(), 1, 1228, rocks), 1 + static_cast<int>(rocks.size()));
    EXPECT_EQ(rocks.size(), 202u);
}

// A floor over a ladder's top is dropped from what fishing gathers; one low, one away and one dipping below the zone's
// height stay, in their order.
TEST(FishingSpots, BrownboosLadderTopsAreDroppedFromWhatIsGathered) {
    auto floor = [](float x, float z, float y0, float y1, float y2) {
        CCPoly      poly{};
        const float corners[3][3] = {
            {x - 2, y0, z - 2},
            {x + 2, y1, z - 2},
            {x,     y2, z + 2}
        };
        for (int v = 0; v < 3; v++) {
            for (int k = 0; k < 3; k++) {
                poly.vertex[v][k] = corners[v][k];
            }
        }
        return poly;
    };
    std::array<CCPoly, 5> polys = {
        floor(10, 10, 50, 50, 50), // the ladder's top: dropped
        floor(10, 10, 10, 10, 10), // the boardwalk at its foot
        floor(70, 10, 50, 50, 50), // high, but away from the ladder
        floor(12, 12, 20, 50, 50), // a stair dipping under the zone's height
        floor(-20, 0, 26, 26, 26), // high and within reach: dropped
    };
    const expanded::FloorDropZone zones[] = {
        {0.0f, 0.0f, 45.0f, 25.0f}
    };
    ASSERT_EQ(expanded::DropPolys(polys.data(), 5, zones), 3);
    EXPECT_EQ(polys[0].vertex[0][1], 10.0f);
    EXPECT_EQ(polys[1].vertex[2][0], 70.0f);
    EXPECT_EQ(polys[2].vertex[0][1], 20.0f);
    EXPECT_EQ(expanded::DropPolys(polys.data(), 3, {}), 3);
}

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
