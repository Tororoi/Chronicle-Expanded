#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "collision.hpp"
#include "expanded/dungeon/navigation.hpp"

namespace {

using expanded::NavigationFloor;

std::vector<CCPoly> g_polys;
bool                g_gridded = true;

// Two triangles over the quad a b c d, facing normal.
void Quad(const float (&a)[3], const float (&b)[3], const float (&c)[3], const float (&d)[3], const float (&normal)[3]) {
    const float *triangles[2][3] = {
        {a, b, c},
        {a, c, d}
    };
    for (auto &triangle : triangles) {
        CCPoly poly = {};
        for (int v = 0; v < 3; v++) {
            std::copy(triangle[v], triangle[v] + 3, poly.vertex[v]);
            poly.vertex[v][3] = 1.0f;
        }
        std::copy(normal, normal + 3, poly.normal);
        g_polys.push_back(poly);
    }
}

// A wall standing on z, from x0 to x1, seen from both sides.
void Wall(float z, float x0, float x1) {
    Quad({x0, 0.0f, z}, {x0, 100.0f, z}, {x1, 100.0f, z}, {x1, 0.0f, z}, {0.0f, 0.0f, -1.0f});
    Quad({x1, 0.0f, z}, {x1, 100.0f, z}, {x0, 100.0f, z}, {x0, 0.0f, z}, {0.0f, 0.0f, 1.0f});
}

bool Gridded() {
    return g_gridded;
}

// Every tile has a part but (5, 5).
bool Open(int col, int row) {

    return col != 5 || row != 5;
}

int Polys(float *, float, float, CCPoly *polys, int limit) {
    int count = std::min(static_cast<int>(g_polys.size()), limit);
    std::copy(g_polys.begin(), g_polys.begin() + count, polys);
    return count;
}

const NavigationFloor kFloor = {Gridded, Open, Polys};

// Flat ground at 0 over the grid, and a wall on z = 320 from the floor's edge to x = 560.
class Navigation : public ::testing::Test {
protected:
    void SetUp() override {
        g_polys.clear();
        g_gridded = true;
        Quad({-80.0f, 0.0f, -80.0f}, {-80.0f, 0.0f, 3120.0f}, {3120.0f, 0.0f, 3120.0f}, {3120.0f, 0.0f, -80.0f}, {0.0f, 1.0f, 0.0f});
        Wall(320.0f, -80.0f, 560.0f);
        expanded::SetNavigationFloor(&kFloor);
    }

    void TearDown() override {
        expanded::SetNavigationFloor(nullptr);
    }
};

} // namespace

TEST_F(Navigation, FloorsOffTheGridLeaveTheGoal) {
    g_gridded = false;
    expanded::SetNavigationFloor(&kFloor);
    float from[3] = {200.0f, 0.0f, 200.0f};
    float goal[3] = {200.0f, 0.0f, 440.0f};
    float waypoint[3];
    float reached[3];
    EXPECT_FALSE(expanded::NavigateTowards(from, goal, 20.0f, 0.0f, waypoint, reached));
    EXPECT_EQ(waypoint[0], goal[0]);
    EXPECT_EQ(waypoint[2], goal[2]);
    EXPECT_TRUE(expanded::NavigateClear(from, goal, 20.0f, 0.0f));
    EXPECT_EQ(expanded::NavigateRegion(from, 20.0f, 0.0f, 100), -1);
}

TEST_F(Navigation, AClearGoalIsHeadedStraightFor) {
    float from[3] = {200.0f, 0.0f, 100.0f};
    float goal[3] = {300.0f, 0.0f, 200.0f};
    float waypoint[3];
    float reached[3];
    EXPECT_TRUE(expanded::NavigateTowards(from, goal, 20.0f, 0.0f, waypoint, reached));
    EXPECT_EQ(waypoint[0], goal[0]);
    EXPECT_EQ(waypoint[2], goal[2]);
    EXPECT_EQ(reached[0], goal[0]);
    EXPECT_EQ(reached[2], goal[2]);
}

TEST_F(Navigation, AWallInTheWayIsHeadedRound) {
    float from[3] = {200.0f, 0.0f, 200.0f};
    float goal[3] = {200.0f, 0.0f, 440.0f};
    float waypoint[3];
    float reached[3];
    EXPECT_TRUE(expanded::NavigateTowards(from, goal, 20.0f, 0.0f, waypoint, reached));
    // Towards the wall's open end, kept clear of it.
    EXPECT_GT(waypoint[0], 560.0f);
    EXPECT_LT(waypoint[2], 320.0f);
    EXPECT_EQ(reached[0], goal[0]);
    EXPECT_EQ(reached[2], goal[2]);
}

TEST_F(Navigation, BodiesAreKeptTheirClearanceFromWalls) {
    // From open ground to 40 from the wall.
    float from[3] = {100.0f, 0.0f, 120.0f};
    float to[3] = {400.0f, 0.0f, 280.0f};
    EXPECT_TRUE(expanded::NavigateClear(from, to, 20.0f, 0.0f));
    EXPECT_FALSE(expanded::NavigateClear(from, to, 60.0f, 0.0f));
}

TEST_F(Navigation, ABodyPressedToAWallCanStepAway) {
    // 30 from the wall, inside a 60 clearance.
    float pressed[3] = {200.0f, 0.0f, 290.0f};
    float away[3] = {200.0f, 0.0f, 120.0f};
    EXPECT_TRUE(expanded::NavigateClear(pressed, away, 60.0f, 0.0f));
    // From open ground in to there is not clear.
    EXPECT_FALSE(expanded::NavigateClear(away, pressed, 60.0f, 0.0f));
}

TEST_F(Navigation, AnUnreachableGoalIsSwappedForTheNearestReachableSpot) {
    // The middle of tile (5, 5), which has no part.
    float from[3] = {400.0f, 0.0f, 800.0f};
    float goal[3] = {800.0f, 0.0f, 800.0f};
    float spot[3];
    EXPECT_TRUE(expanded::NavigateNearest(from, goal, 20.0f, 0.0f, spot));
    // Just off the tile.
    float off = std::max(std::fabs(spot[0] - goal[0]), std::fabs(spot[2] - goal[2]));
    EXPECT_GT(off, 80.0f);
    EXPECT_LT(off, 80.0f + expanded::kNavCell);
}

TEST_F(Navigation, ARegionPullsPointsIntoWhatItReaches) {
    float start[3] = {200.0f, 0.0f, 200.0f};
    int   region = expanded::NavigateRegion(start, 20.0f, 0.0f, 200 * 200);
    ASSERT_GE(region, 0);
    // Only its start cell until grown.
    float far[3] = {1200.0f, 0.0f, 200.0f};
    float spot[3];
    EXPECT_TRUE(expanded::NavigateRegionNearest(region, far, spot));
    EXPECT_LT(spot[0], 240.0f);
    expanded::NavigateRegionGrow(region, 200 * 200);
    EXPECT_TRUE(expanded::NavigateRegionNearest(region, far, spot));
    EXPECT_EQ(spot[0], far[0]);
    EXPECT_EQ(spot[2], far[2]);
    // Inside the partless tile: pulled out of it.
    float hole[3] = {800.0f, 0.0f, 800.0f};
    EXPECT_TRUE(expanded::NavigateRegionNearest(region, hole, spot));
    EXPECT_LT(std::max(std::fabs(spot[0] - 800.0f), std::fabs(spot[2] - 800.0f)), 120.0f);
    EXPECT_GT(std::max(std::fabs(spot[0] - 800.0f), std::fabs(spot[2] - 800.0f)), 80.0f);
}
