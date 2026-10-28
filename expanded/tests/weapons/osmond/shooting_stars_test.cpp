#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "character.hpp"
#include "collision.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/event_sparkle.hpp"
#include "expanded/dungeon/floor_height.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/enemies/miniboss_loot.hpp"
#include "expanded/weapons/osmond/star_breaker/shooting_stars.hpp"
#include "itemdata.hpp"
#include "randomitem.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::NavigationFloor;
using expanded::test::TestFloor;

// The player stands at (400, 0, 330), facing +z unless a test turns him: the spots drawn lie 345 to 480 in z and 350 to 450
// in x. Over them the floor is flat at 0 but for a ledge at 50 (x 420-450, z 350-400) and a pit (x 360-380, z 350-380);
// tile (2, 3) (x below 400, z from 400) has no part, and a wall stands on z = 444 from x = 420 to 470.
constexpr float kPlayer[3] = {400.0f, 0.0f, 330.0f};

bool InLedge(float x, float z) {
    return x >= 420.0f && x <= 450.0f && z >= 350.0f && z <= 400.0f;
}

bool InPit(float x, float z) {
    return x >= 360.0f && x <= 380.0f && z >= 350.0f && z <= 380.0f;
}

bool InHole(float x, float z) {
    return std::floor((x + 80.0f) / 160.0f) == 2.0f && std::floor((z + 80.0f) / 160.0f) == 3.0f;
}

// Behind the wall, as a straight walk from the player meets it.
bool BehindWall(float x, float z) {
    if (z <= 444.0f) {
        return false;
    }
    float t = (444.0f - kPlayer[2]) / (z - kPlayer[2]);
    float across = kPlayer[0] + (x - kPlayer[0]) * t;
    return across >= 420.0f && across <= 470.0f;
}

bool g_walls = true;

bool FakeFloor(float x, float z, float from, float *ground) {
    if (g_walls && InPit(x, z)) {
        return false;
    }
    *ground = g_walls && InLedge(x, z) ? 50.0f : 0.0f;
    return *ground <= from;
}

std::vector<CCPoly> g_polys;

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

bool Gridded() {
    return true;
}

bool Open(int col, int row) {
    return !g_walls || col != 2 || row != 3;
}

int Polys(float *, float, float, CCPoly *polys, int limit) {
    int count = std::min(static_cast<int>(g_polys.size()), limit);
    std::copy(g_polys.begin(), g_polys.begin() + count, polys);
    return count;
}

const NavigationFloor kFloor = {Gridded, Open, Polys};

// No disc in the tests: the sparkle's picture is never in.
unsigned int *NoImage() {
    return nullptr;
}

// The floor above, the player on it, the floor's drops empty and the save's attachment board for pickups.
class ShootingStars : public ::testing::Test {
protected:
    void SetUp() override {
        g_walls = true;
        g_polys.clear();
        Quad({-80.0f, 0.0f, -80.0f}, {-80.0f, 0.0f, 3120.0f}, {3120.0f, 0.0f, 3120.0f}, {3120.0f, 0.0f, -80.0f}, {0.0f, 1.0f, 0.0f});
        Quad({420.0f, 0.0f, 444.0f}, {420.0f, 100.0f, 444.0f}, {470.0f, 100.0f, 444.0f}, {470.0f, 0.0f, 444.0f}, {0.0f, 0.0f, -1.0f});
        Quad({470.0f, 0.0f, 444.0f}, {470.0f, 100.0f, 444.0f}, {420.0f, 100.0f, 444.0f}, {420.0f, 0.0f, 444.0f}, {0.0f, 0.0f, 1.0f});
        expanded::SetNavigationFloor(&kFloor);
        expanded::SetFloorSearch(FakeFloor);
        expanded::SetSparkleImageRead(NoImage);
        floor_.status().cur_chara = CHARA_OSMOND;
        saved_status_ = UserStatus;
        UserStatus = &save_.status();
        UserStatus->cur_chara = CHARA_OSMOND;
        Stand(kPlayer[0], kPlayer[2], 0.0f);
        for (int i = 0; i < 32; i++) {
            items_.id[i] = -1;
            items_.pickup_event[i] = -1;
            items_.pickup_blocked[i] = 0;
        }
        saved_items_ = RandomItem;
        RandomItem = &items_;
        expanded::ShootingStarsVisitBegins();
        expanded::MinibossDropsCleared();
        expanded::SeedModRandom(3);
    }

    void TearDown() override {
        expanded::ShootingStarsVisitBegins();
        RandomItem = saved_items_;
        UserStatus = saved_status_;
        expanded::SetFloorSearch(nullptr);
        expanded::SetNavigationFloor(nullptr);
        expanded::SetSparkleImageRead(nullptr);
    }

    void Stand(float x, float z, float yaw) {
        CharaMain.SetPosition(x, 0.0f, z);
        CharaMain.SetRotation(0.0f, yaw, 0.0f);
    }

    // The star sent and stepped until it lies: false when it does not within limit steps.
    bool Land(int limit = 200) {
        expanded::SendShootingStar();
        for (int i = 0; i < limit && expanded::StarsLying() == 0; i++) {
            expanded::ShootingStarsStep();
        }
        return expanded::StarsLying() == 1;
    }

    // The lying star's drop and its place.
    int Slot() {
        const CRandomItem *items = nullptr;
        int                slot = -1;
        EXPECT_TRUE(expanded::LyingStar(0, &items, &slot));
        EXPECT_EQ(items, &items_);
        return slot;
    }

    // A frame of the floor's drops: the hop and the pickup as the game steps them (DunMoveChara), then the stars.
    void Frame() {
        items_.CheckPosition();
        items_.Step();
        expanded::ShootingStarsStep();
    }

    int BoardSpheres() {
        int count = 0;
        for (const DNG_CONSUMABLE &item : UserStatus->consumable_items) {
            count += item.id == ITEM_ATTACH_SYNTHESIS_SPHERE ? 1 : 0;
        }
        return count;
    }

    TestFloor                floor_;
    expanded::test::TestSave save_;
    CUserStatus             *saved_status_ = nullptr;
    CRandomItem              items_ = {};
    CRandomItem             *saved_items_ = nullptr;
};

} // namespace

// Every spot drawn over the box: those taken lie in it, on the floor the player walks; the ledge, the pit, the tile
// without a part and the floor behind the wall are drawn and passed over.
TEST_F(ShootingStars, SpotsAreWalkableFloorInFront) {
    int taken = 0, ledge = 0, pit = 0, hole = 0, wall = 0;
    for (int a = 0; a <= 40; a++) {
        for (int s = 0; s <= 40; s++) {
            float ahead = static_cast<float>(a) / 40.0f, sideways = static_cast<float>(s) / 40.0f;
            float distance = expanded::kStarNearest + ahead * (expanded::kStarFarthest - expanded::kStarNearest);
            float aside = (2.0f * sideways - 1.0f) * expanded::kStarSideways;
            float x = kPlayer[0] + aside, z = kPlayer[2] + distance;
            float spot[3];
            bool  ok = expanded::ShootingStarSpot(kPlayer, 0.0f, ahead, sideways, spot);
            ledge += InLedge(x, z) ? 1 : 0;
            pit += InPit(x, z) ? 1 : 0;
            hole += InHole(x, z) ? 1 : 0;
            wall += BehindWall(x, z) ? 1 : 0;
            if (!ok) {
                continue;
            }
            taken++;
            EXPECT_GE(spot[2] - kPlayer[2], 15.0f - 1e-3f);
            EXPECT_LE(spot[2] - kPlayer[2], 150.0f + 1e-3f);
            EXPECT_LE(std::fabs(spot[0] - kPlayer[0]), 50.0f + 1e-3f);
            EXPECT_FALSE(InLedge(spot[0], spot[2]) || InPit(spot[0], spot[2]) || InHole(spot[0], spot[2]) || BehindWall(spot[0], spot[2]))
                << spot[0] << ", " << spot[2];
            EXPECT_EQ(spot[1], 0.0f);
        }
    }
    EXPECT_GT(taken, 600);
    EXPECT_GT(ledge, 0);
    EXPECT_GT(pit, 0);
    EXPECT_GT(hole, 0);
    EXPECT_GT(wall, 0);
}

// Turned, the box turns with him: ahead along his facing, aside across it.
TEST_F(ShootingStars, TheBoxFollowsHisFacing) {
    g_walls = false;
    float yaw = std::numbers::pi_v<float> / 2.0f; // facing +x
    for (int i = 0; i < 200; i++) {
        float ahead = static_cast<float>(i % 20) / 19.0f, sideways = static_cast<float>(i / 20) / 9.0f;
        float spot[3];
        ASSERT_TRUE(expanded::ShootingStarSpot(kPlayer, yaw, ahead, sideways, spot));
        EXPECT_GE(spot[0] - kPlayer[0], 15.0f - 1e-3f);
        EXPECT_LE(spot[0] - kPlayer[0], 150.0f + 1e-3f);
        EXPECT_LE(std::fabs(spot[2] - kPlayer[2]), 50.0f + 1e-3f);
    }
}

// Sent stars land on walkable floor in the box, drawn again where a draw falls short.
TEST_F(ShootingStars, StarsLandOnlyOnWalkableFloor) {
    for (int star = 0; star < 30; star++) {
        ASSERT_TRUE(Land());
        int   slot = Slot();
        float x = items_.position[slot][0], z = items_.position[slot][2];
        EXPECT_GE(z - kPlayer[2], 15.0f - 1e-3f);
        EXPECT_LE(z - kPlayer[2], 150.0f + 1e-3f);
        EXPECT_LE(std::fabs(x - kPlayer[0]), 50.0f + 1e-3f);
        EXPECT_FALSE(InLedge(x, z) || InPit(x, z) || InHole(x, z) || BehindWall(x, z)) << x << ", " << z;
        items_.id[slot] = -1;
        expanded::ShootingStarsStep();
        ASSERT_EQ(expanded::StarsLying(), 0);
    }
}

// Facing a wall close up, no spot: the star waits for one, and falls once he turns.
TEST_F(ShootingStars, AStarWaitsForASpot) {
    g_polys.clear();
    Quad({-80.0f, 0.0f, -80.0f}, {-80.0f, 0.0f, 3120.0f}, {3120.0f, 0.0f, 3120.0f}, {3120.0f, 0.0f, -80.0f}, {0.0f, 1.0f, 0.0f});
    Quad({300.0f, 0.0f, 340.0f}, {300.0f, 100.0f, 340.0f}, {500.0f, 100.0f, 340.0f}, {500.0f, 0.0f, 340.0f}, {0.0f, 0.0f, -1.0f});
    Quad({500.0f, 0.0f, 340.0f}, {500.0f, 100.0f, 340.0f}, {300.0f, 100.0f, 340.0f}, {300.0f, 0.0f, 340.0f}, {0.0f, 0.0f, 1.0f});
    expanded::SetNavigationFloor(&kFloor);
    expanded::SendShootingStar();
    for (int i = 0; i < 10; i++) {
        expanded::ShootingStarsStep();
    }
    EXPECT_EQ(expanded::StarsWaiting(), 1);
    Stand(kPlayer[0], kPlayer[2], std::numbers::pi_v<float>); // facing -z
    expanded::ShootingStarsStep();
    EXPECT_EQ(expanded::StarsWaiting(), 0);
    EXPECT_EQ(expanded::StarsFalling(), 1);
}

// It comes down a straight line at 60 degrees to the floor, across the way he faces, and lands on its spot.
TEST_F(ShootingStars, TheFallIsSixtyDegrees) {
    g_walls = false;
    expanded::SendShootingStar();
    expanded::ShootingStarsStep();
    float at[3], direction[3];
    ASSERT_TRUE(expanded::FallingStar(0, at, direction));
    float flat = std::hypot(direction[0], direction[2]);
    EXPECT_NEAR(std::atan2(-direction[1], flat), std::numbers::pi_v<float> / 3.0f, 1e-5f);
    EXPECT_NEAR(direction[2], 0.0f, 1e-5f); // across +z
    EXPECT_NEAR(at[1], expanded::kStarFallHeight, 1e-3f);
    float start[3] = {at[0], at[1], at[2]};
    int   steps = 0;
    while (expanded::FallingStar(0, at, direction)) {
        // On the line from its start, kStarFallSpeed a step.
        float along = (start[1] - at[1]) / -direction[1];
        for (int c = 0; c < 3; c++) {
            EXPECT_NEAR(at[c], start[c] + direction[c] * along, 1e-3f);
        }
        EXPECT_NEAR(along, std::min(expanded::kStarFallSpeed * static_cast<float>(steps), expanded::kStarFallHeight / std::sin(expanded::kStarFallAngle)),
                    1e-2f);
        expanded::ShootingStarsStep();
        steps++;
        ASSERT_LT(steps, 100);
    }
    EXPECT_EQ(expanded::StarsLying(), 1);
    int slot = Slot();
    EXPECT_NEAR(items_.position[slot][0], start[0] + direction[0] * (start[1] / -direction[1]), 1e-2f);
    EXPECT_NEAR(items_.position[slot][1], 1.75f, 1e-5f); // CRandomItem::Set's lift
}

// On landing it hops once, the game's drop hop (CRandomItem::Draw lifts it 8 sin(phase)), and then rests.
TEST_F(ShootingStars, ItBouncesOnceAndRests) {
    ASSERT_TRUE(Land());
    int slot = Slot();
    Stand(kPlayer[0] + 300.0f, kPlayer[2], 0.0f); // out of reach
    std::vector<float> lift;
    for (int i = 0; i < 400; i++) {
        lift.push_back(8.0f * std::sin(items_.phase[slot]));
        Frame();
    }
    int peaks = 0;
    for (std::size_t i = 1; i + 1 < lift.size(); i++) {
        peaks += lift[i] > lift[i - 1] && lift[i] >= lift[i + 1] ? 1 : 0;
    }
    EXPECT_EQ(peaks, 1);
    EXPECT_GT(*std::max_element(lift.begin(), lift.end()), 7.9f);
    EXPECT_EQ(items_.state[slot], RandomItemState::WAITING);
    EXPECT_TRUE(std::all_of(lift.begin() + 40, lift.end(), [](float h) { return h == 0.0f; }));
    EXPECT_EQ(expanded::StarsLying(), 1);
}

// Walked over, it is taken as any drop is and gives an empty SynthSphere: no weapon's, nothing in it.
TEST_F(ShootingStars, WalkedOverItGivesAnEmptySphere) {
    ASSERT_TRUE(Land());
    int slot = Slot();
    for (int i = 0; i < 40; i++) {
        Frame(); // the hop, out of reach
    }
    int before = BoardSpheres();
    CharaMain.SetPosition(items_.position[slot][0], 0.0f, items_.position[slot][2]);
    items_.CheckPosition();
    items_.Step();
    expanded::ShootingStarsStep();
    EXPECT_EQ(expanded::StarsLying(), 0);
    int event = expanded::MinibossItemEvent(items_);
    ASSERT_EQ(event, ITEM_ATTACH_SYNTHESIS_SPHERE);
    expanded::MinibossDropPickUp(DUNGEON_DIVINE_BEAST_CAVE, event);
    EXPECT_EQ(BoardSpheres(), before + 1);
    for (const DNG_CONSUMABLE &item : UserStatus->consumable_items) {
        if (item.id != ITEM_ATTACH_SYNTHESIS_SPHERE) {
            continue;
        }
        const auto &sphere = reinterpret_cast<const ATTACH_LIST &>(item);
        EXPECT_EQ(sphere.sphere_weapon_no, 0);
        EXPECT_EQ(sphere.sphere_flags, 0);
        EXPECT_EQ(sphere.sphere_level, 0);
        for (s16 stat : sphere.status) {
            EXPECT_EQ(stat, 0);
        }
        for (s8 value : sphere.elem) {
            EXPECT_EQ(value, 0);
        }
        for (s8 value : sphere.vs_monster) {
            EXPECT_EQ(value, 0);
        }
    }
}

// With the attachment bag full the game leaves it lying (its "bag is full" line), and it is taken once there is room
// and the player comes back to it.
TEST_F(ShootingStars, AFullBagLeavesItLying) {
    for (int i = 0; i < 40; i++) {
        UserStatus->consumable_items[i].id = ITEM_ATTACH_FIRE;
    }
    ASSERT_TRUE(Land());
    int slot = Slot();
    for (int i = 0; i < 40; i++) {
        Frame();
    }
    float x = items_.position[slot][0], z = items_.position[slot][2];
    CharaMain.SetPosition(x, 0.0f, z);
    Frame();
    EXPECT_EQ(items_.id[slot], 1);
    EXPECT_EQ(items_.pickup_blocked[slot], 2);
    EXPECT_EQ(items_.checkErr(), 1);
    EXPECT_EQ(expanded::StarsLying(), 1);
    for (int i = 0; i < 30; i++) {
        Frame();
    }
    EXPECT_EQ(expanded::StarsLying(), 1);

    UserStatus->consumable_items[0].id = 0;
    CharaMain.SetPosition(x + 20.0f, 0.0f, z); // off it: the game lets it be tried again
    Frame();
    EXPECT_EQ(items_.pickup_blocked[slot], 0);
    CharaMain.SetPosition(x, 0.0f, z);
    Frame();
    EXPECT_EQ(expanded::StarsLying(), 0);
    EXPECT_EQ(expanded::MinibossItemEvent(items_), ITEM_ATTACH_SYNTHESIS_SPHERE);
}

// The floor's drops go with the floor, the star among them; one still falling goes with it too. Away on another floor's
// drops (the back floor), it is kept for when the player returns.
TEST_F(ShootingStars, TheFloorTakesItAsItTakesDrops) {
    ASSERT_TRUE(Land());
    CRandomItem back = {};
    for (int i = 0; i < 32; i++) {
        back.id[i] = -1;
    }
    RandomItem = &back;
    expanded::ShootingStarsStep();
    EXPECT_EQ(expanded::StarsLying(), 1);
    RandomItem = &items_;
    for (int i = 0; i < 32; i++) {
        items_.id[i] = -1; // ClearRandomItems, as the next floor is built
    }
    expanded::ShootingStarsStep();
    EXPECT_EQ(expanded::StarsLying(), 0);

    g_walls = false;
    expanded::SendShootingStar();
    expanded::ShootingStarsStep();
    ASSERT_EQ(expanded::StarsFalling(), 1);
    floor_.SetUpModel(0, Monster::GUNNY); // the next floor's kinds
    EXPECT_EQ(expanded::StarsFalling(), 0);
}
