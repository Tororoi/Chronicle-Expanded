#include <gtest/gtest.h>

#include <vector>

#include "expanded/town/camera_sweep.hpp"

namespace expanded::test {

namespace {

// A wall across x = 10, from y -50 to 50 and z -50 to 50.
std::vector<Triangle> Wall(float x = 10.0f) {
    return {
        Triangle{Point{x, -50, -50}, Point{x, 50, -50}, Point{x, 50, 50} },
        Triangle{Point{x, -50, -50}, Point{x, 50, 50},  Point{x, -50, 50}},
    };
}

SweepMesh Mesh(std::vector<Triangle> triangles) {
    SweepMesh mesh;
    mesh.Build(std::move(triangles));
    return mesh;
}

TEST(CameraSweep, StopsShortOfAWallByItsRadius) {
    const SweepMesh mesh = Mesh(Wall());
    // From x 0 to 20: the sphere of 2 touches the wall at x 8, 0.4 of the way.
    EXPECT_NEAR(mesh.Sweep({0, 0, 0}, {20, 0, 0}, 2.0f), 0.4f, 1e-4f);
}

TEST(CameraSweep, GoesAllTheWayWhenNothingIsInTheWay) {
    const SweepMesh mesh = Mesh(Wall());
    EXPECT_EQ(mesh.Sweep({0, 0, 0}, {0, 0, 40}, 2.0f), 1.0f);
    EXPECT_EQ(mesh.Sweep({0, 0, 0}, {-30, 0, 0}, 2.0f), 1.0f);
}

TEST(CameraSweep, GrazingAnEdgeStopsIt) {
    // Past the wall's end at z 50 by less than the radius: a ray would miss it, the sphere does not.
    const SweepMesh mesh = Mesh(Wall());
    EXPECT_LT(mesh.Sweep({0, 0, 51}, {20, 0, 51}, 2.0f), 1.0f);
    EXPECT_EQ(mesh.Sweep({0, 0, 53}, {20, 0, 53}, 2.0f), 1.0f);
}

TEST(CameraSweep, ATouchingWallStopsItOnlyGoingNearer) {
    // Starting 1 from the wall with a radius of 2.
    const SweepMesh mesh = Mesh(Wall());
    EXPECT_NEAR(mesh.Sweep({9, 0, 0}, {20, 0, 0}, 2.0f), 0.0f, 1e-4f);
    EXPECT_EQ(mesh.Sweep({9, 0, 0}, {-20, 0, 0}, 2.0f), 1.0f);
    EXPECT_EQ(mesh.Sweep({9, 0, 0}, {9, 0, 30}, 2.0f), 1.0f);
}

TEST(CameraSweep, TheNearestOfSeveralWallsStopsIt) {
    std::vector<Triangle> walls = Wall(30.0f);
    for (const Triangle &t : Wall(10.0f)) {
        walls.push_back(t);
    }
    // Many more far away, so the tree has branches to pass over.
    for (int i = 0; i < 100; i++) {
        for (const Triangle &t : Wall(200.0f + i)) {
            walls.push_back(t);
        }
    }
    const SweepMesh mesh = Mesh(walls);
    EXPECT_NEAR(mesh.Sweep({0, 0, 0}, {100, 0, 0}, 2.0f), 0.08f, 1e-4f);
}

TEST(CameraSweep, PushOutMovesAPointClearOfAWall) {
    const SweepMesh mesh = Mesh(Wall());
    const Point     out = mesh.PushOut({9, 0, 0}, 3.0f, 10.0f);
    EXPECT_NEAR(out[0], 7.0f, 1e-4f);
    const Point held = mesh.PushOut({9, 0, 0}, 3.0f, 0.5f);
    EXPECT_NEAR(held[0], 8.5f, 1e-4f);
}

TEST(CameraSweep, TrianglesWithNoAreaAreLeftOut) {
    const SweepMesh mesh = Mesh({
        Triangle{Point{10, 0, 0}, Point{10, 0, 0}, Point{10, 5, 0}}
    });
    EXPECT_TRUE(mesh.Empty());
    EXPECT_EQ(mesh.Sweep({0, 0, 0}, {20, 0, 0}, 2.0f), 1.0f);
}

TEST(CameraSweep, ARayStopsAtTheWall) {
    const SweepMesh mesh = Mesh(Wall());
    EXPECT_NEAR(mesh.Ray({0, 0, 0}, {20, 0, 0}), 0.5f, 1e-5f);
    EXPECT_EQ(mesh.Ray({0, 0, 60}, {20, 0, 60}), 1.0f);
    EXPECT_EQ(mesh.Ray({0, 0, 0}, {5, 0, 0}), 1.0f);
}

TEST(CameraSweep, TouchesWhatIsWithinTheRadius) {
    const SweepMesh mesh = Mesh(Wall());
    EXPECT_TRUE(mesh.Touches({8, 0, 0}, 3.0f));
    EXPECT_FALSE(mesh.Touches({6, 0, 0}, 3.0f));
    EXPECT_FALSE(mesh.Touches({8, 0, 60}, 3.0f));
}

TEST(CameraSweep, ClosestFindsTheNearestPointWithinTheRadius) {
    const SweepMesh mesh = Mesh(Wall());
    Point           nearest{};
    EXPECT_NEAR(mesh.Closest({7, 2, 3}, 5.0f, nearest), 3.0f, 1e-5f);
    EXPECT_NEAR(nearest[0], 10.0f, 1e-5f);
    EXPECT_NEAR(nearest[1], 2.0f, 1e-5f);
    EXPECT_EQ(mesh.Closest({0, 0, 0}, 5.0f, nearest), 5.0f);
}

// A box from (x, y, z) up height, size across in x and deep (size when 0) in z, as its 12 triangles; its corners
// shared.
std::vector<Triangle> Box(float x, float z, float size, float height, float deep = 0.0f, float y = 0.0f) {
    const float d = deep > 0.0f ? deep : size;
    const float top = y + height;
    const Point p[8] = {
        {x,        y,   z    },
        {x + size, y,   z    },
        {x + size, y,   z + d},
        {x,        y,   z + d},
        {x,        top, z    },
        {x + size, top, z    },
        {x + size, top, z + d},
        {x,        top, z + d}
    };
    const int faces[6][4] = {
        {0, 1, 2, 3},
        {4, 5, 6, 7},
        {0, 1, 5, 4},
        {1, 2, 6, 5},
        {2, 3, 7, 6},
        {3, 0, 4, 7}
    };
    std::vector<Triangle> out;
    for (const auto &f : faces) {
        out.push_back({p[f[0]], p[f[1]], p[f[2]]});
        out.push_back({p[f[0]], p[f[2]], p[f[3]]});
    }
    return out;
}

TEST(CameraSweep, KeepBlockingLetsPassThroughPiecesGo) {
    std::vector<Triangle> all;
    const auto            add = [&](const std::vector<Triangle> &part) { all.insert(all.end(), part.begin(), part.end()); };
    add(Box(0, 0, 100, 60));       // a house: kept
    add(Box(200, 0, 10, 80));      // a pillar: narrow
    add(Box(300, 0, 60, 15, 4));   // a low wall: low
    add(Box(-500, -500, 1000, 2)); // the ground: broad
    const std::vector<Triangle> kept = KeepBlocking(all);
    EXPECT_EQ(kept.size(), 24u);
    const SweepMesh mesh = Mesh(kept);
    EXPECT_LT(mesh.Sweep({50, 30, -20}, {50, 30, 20}, 2.0f), 1.0f);   // the house stops it
    EXPECT_EQ(mesh.Sweep({205, 30, -20}, {205, 30, 20}, 2.0f), 1.0f); // the pillar does not
    EXPECT_EQ(mesh.Sweep({330, 10, -20}, {330, 10, 20}, 2.0f), 1.0f); // nor the low wall
    EXPECT_LT(mesh.Sweep({0, 10, 0}, {0, -10, 0}, 2.0f), 1.0f);       // the ground does
}

TEST(CameraSweep, KeepBlockingSizesTouchingPiecesTogether) {
    std::vector<Triangle>       all = Box(0, 0, 100, 60);    // a tent
    const std::vector<Triangle> door = Box(40, 100, 20, 20); // the low curtain across its door, touching it: kept with it
    all.insert(all.end(), door.begin(), door.end());
    const std::vector<Triangle> ground = Box(-500, -500, 1000, 1); // the ground under both
    all.insert(all.end(), ground.begin(), ground.end());
    const std::vector<Triangle> post = Box(300, 0, 10, 50); // a post on the ground: still passes
    all.insert(all.end(), post.begin(), post.end());
    const std::vector<Triangle> leg = Box(-12, 40, 10, 80); // a pillar against the tent: still a pillar
    all.insert(all.end(), leg.begin(), leg.end());
    EXPECT_EQ(KeepBlocking(all).size(), 36u);
}

TEST(CameraSweep, KeepBlockingKeepsFloorTiles) {
    // Low and narrow, but floor: a plaza's tile is kept, a low wall's rim is not.
    std::vector<Triangle> all = {
        Triangle{Point{0, 0, 0}, Point{50, 0, 0},  Point{50, 0, 50}},
        Triangle{Point{0, 0, 0}, Point{50, 0, 50}, Point{0, 0, 50} },
    };
    const std::vector<Triangle> rim = Box(300, 0, 60, 10, 4);
    all.insert(all.end(), rim.begin(), rim.end());
    EXPECT_EQ(KeepBlocking(all).size(), 2u);
}

TEST(CameraSweep, KeepBlockingLetsAnArchsLegsThroughHoweverWide) {
    std::vector<Triangle> all = Box(0, 0, 45, 60); // a leg as wide as a house
    const auto            add = [&](const std::vector<Triangle> &part) { all.insert(all.end(), part.begin(), part.end()); };
    add(Box(100, 0, 45, 60));         // the other leg
    add(Box(-5, 0, 155, 25, 45, 59)); // the span across them, above y 59
    add(Box(300, 0, 45, 60));         // a lone rock as wide: still blocks
    const std::vector<Triangle> kept = KeepBlocking(all);
    const SweepMesh             mesh = Mesh(kept);
    EXPECT_EQ(mesh.Sweep({20, 30, -20}, {20, 30, 80}, 2.0f), 1.0f);   // through one leg
    EXPECT_EQ(mesh.Sweep({120, 30, -20}, {120, 30, 80}, 2.0f), 1.0f); // and the other
    EXPECT_LT(mesh.Sweep({72, 70, -20}, {72, 70, 80}, 2.0f), 1.0f);   // the span between them blocks
    EXPECT_LT(mesh.Sweep({320, 30, -20}, {320, 30, 80}, 2.0f), 1.0f); // the lone rock blocks
}

TEST(CameraSweep, KeepBlockingLetsThroughWhatAPillarHoldsJustOverIt) {
    // A stilt house: a stilt 20 across and 30 tall, its rounded top 40 across up to y 51, the house on that.
    std::vector<Triangle> all = Box(50, 50, 20, 30);
    const auto            add = [&](const std::vector<Triangle> &part) { all.insert(all.end(), part.begin(), part.end()); };
    add(Box(40, 40, 40, 22, 0, 29));   // the stilt's top, touching the house: it would block with it
    add(Box(0, 0, 120, 25, 0, 51.5f)); // the house
    const SweepMesh mesh = Mesh(KeepBlocking(all));
    EXPECT_EQ(mesh.Sweep({60, 15, -20}, {60, 15, 140}, 2.0f), 1.0f); // through the stilt
    EXPECT_EQ(mesh.Sweep({60, 40, -20}, {60, 40, 140}, 2.0f), 1.0f); // and its top
    EXPECT_LT(mesh.Sweep({60, 65, -20}, {60, 65, 140}, 2.0f), 1.0f); // the house blocks
}

TEST(CameraSweep, PassThroughPartsTellWhichWayTheirFacesTurn) {
    // A post as a tetrahedron, (b - a) × (c - a) pointing out of each face; then the same turned inside out.
    const Point                 o{0, 0, 0}, x{10, 0, 0}, y{0, 10, 0}, z{0, 0, 10};
    const std::vector<Triangle> out = {
        Triangle{o, y, x},
        Triangle{o, x, z},
        Triangle{o, z, y},
        Triangle{x, y, z},
    };
    const std::vector<bool> none(out.size(), false);
    std::vector<PassPart>   parts = PassThroughParts(out, none);
    ASSERT_EQ(parts.size(), 1u);
    EXPECT_TRUE(parts[0].outward);
    EXPECT_EQ(parts[0].max[1], 10.0f);
    std::vector<Triangle> in;
    for (const Triangle &t : out) {
        in.push_back({t.a, t.c, t.b});
    }
    parts = PassThroughParts(in, none);
    ASSERT_EQ(parts.size(), 1u);
    EXPECT_FALSE(parts[0].outward);
    // A blocking mesh has no pass-through parts.
    EXPECT_TRUE(PassThroughParts(out, std::vector<bool>(out.size(), true)).empty());
}

TEST(CameraSweep, KeepBlockingCanKeepTallPiecesHoweverNarrow) {
    // A stilt house's leg, 15 across and 27 tall, under its floor: passes, unless tall pieces block.
    std::vector<Triangle>       all = Box(50, 50, 15, 27);
    const std::vector<Triangle> house = Box(0, 0, 120, 40, 0, 27);
    all.insert(all.end(), house.begin(), house.end());
    EXPECT_EQ(KeepBlocking(all).size(), 12u);
    Blocking legs;
    legs.tall_blocks = true;
    EXPECT_EQ(KeepBlocking(all, legs).size(), 24u);
}

TEST(CameraSweep, KeepBlockingWeldsCornersSplitByTheMesh) {
    // A tall wide wall as two triangles whose shared corners differ by under a hundredth: one piece, kept.
    const std::vector<Triangle> wall = {
        Triangle{Point{0, 0, 0},      Point{100, 0, 0},       Point{100, 60, 0}},
        Triangle{Point{0.001f, 0, 0}, Point{100.001f, 60, 0}, Point{0, 60, 0}  },
    };
    EXPECT_EQ(KeepBlocking(wall).size(), 2u);
}

TEST(CameraSweep, ATouchingTriangleStillStopsItFurtherAlong) {
    // A floor at y 0 from x -50 to 50; starting 1 above it near its edge at x -49 with a radius of 2, moving away from
    // its nearest point (up) and along it: it may not come nearer than 1 anywhere further along.
    const SweepMesh mesh = Mesh({
        Triangle{Point{-50, 0, -50}, Point{50, 0, -50}, Point{50, 0, 50} },
        Triangle{Point{-50, 0, -50}, Point{50, 0, 50},  Point{-50, 0, 50}}
    });
    EXPECT_EQ(mesh.Sweep({0, 1, 0}, {0, 10, 0}, 2.0f), 1.0f);   // straight away
    EXPECT_EQ(mesh.Sweep({0, 1, 0}, {30, 1, 0}, 2.0f), 1.0f);   // along it, no nearer
    EXPECT_LT(mesh.Sweep({0, 1, 0}, {30, -5, 0}, 2.0f), 0.01f); // down into it
}

} // namespace

} // namespace expanded::test
