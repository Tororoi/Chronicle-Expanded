#include <gtest/gtest.h>
#include <libpad.h>

#include <array>
#include <cmath>
#include <memory>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/blast.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "motionmodel.hpp"
#include "platform/input.hpp"
#include "test_support.hpp"
#include "weaponelement.hpp"

using expanded::Monster;

namespace {

// Four Master Jackets in a row from the blast at the origin, each with one body sphere of radius 5.
struct Floor {
    expanded::test::TestFloor       floor;
    expanded::test::Wielding        sword{ITEM_WEAPON_BIG_BANG};
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved = NowColData;

    explicit Floor(std::initializer_list<float> distances) {
        NowColData = hits.get();
        sword.weapon().attack = 10;
        floor.SetUpModel(0, Monster::MASTER_JACKET);
        int monster = 0;
        for (float x : distances) {
            floor.Spawn(monster, 0);
            CMonstorUnit &unit = floor.unit();
            unit.monster[monster].revealed = -1;
            unit.monster[monster].knockback[2] = 1.0f;
            unit.chara[monster][0].SetPosition(x + 5.0f, 0.0f, 0.0f);
            unit.effect[monster].timer[0] = 1;
            unit.effect[monster].radius[0] = 5.0f;
            unit.effect[monster].position[0][0] = x + 5.0f; // its near edge at x
            monster++;
        }
    }

    ~Floor() { NowColData = saved; }

    int Damage(int hit) { return hits->hit[hit].damage; }
};

const float kOrigin[4] = {0.0f, 0.0f, 0.0f, 1.0f};

} // namespace

TEST(Blast, FallsOffWithDistanceToTheBody) {
    Floor floor({5.0f, 20.0f, 30.0f, 45.0f, 60.0f});
    EXPECT_EQ(expanded::PlantBlast(floor.floor.unit(), kOrigin, 10), 4);
    EXPECT_EQ(floor.Damage(0), 40); // within 10
    EXPECT_EQ(floor.Damage(1), 30); // within 25
    EXPECT_EQ(floor.Damage(2), 20); // within 40
    EXPECT_EQ(floor.Damage(3), 10); // within 50
    EXPECT_EQ(floor.hits->active[4], 0);
}

TEST(Blast, IsElementlessThrowsAwayAndPassesGuards) {
    Floor floor({5.0f});
    expanded::PlantBlast(floor.floor.unit(), kOrigin, 10);
    const COLLISION_HIT &hit = floor.hits->hit[0];
    EXPECT_EQ(hit.flags, 0);
    EXPECT_EQ(hit.knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit.knockback_speed, expanded::kBlastKick.speed);
    EXPECT_TRUE(expanded::BlastHit(hit));
    EXPECT_TRUE(expanded::MonsterGuardBlocks(floor.floor.unit(), 0, COLLISION_HIT{}));
    EXPECT_FALSE(expanded::MonsterGuardBlocks(floor.floor.unit(), 0, hit));
}

TEST(Blast, AGuardStopsAGuardableOne) {
    Floor                  floor({5.0f});
    expanded::BlastOptions options;
    options.guardable = true;
    expanded::PlantBlast(floor.floor.unit(), kOrigin, 10, options);
    const COLLISION_HIT &hit = floor.hits->hit[0];
    EXPECT_FALSE(expanded::BlastHit(hit));
    EXPECT_TRUE(expanded::MonsterGuardBlocks(floor.floor.unit(), 0, hit));
}

TEST(Blast, ScalesAndSparesAsAsked) {
    Floor                  floor({5.0f, 55.0f});
    expanded::BlastOptions options;
    options.damage = 0.5f;
    options.reach = 1.2f;
    options.still = 0;
    EXPECT_EQ(expanded::PlantBlast(floor.floor.unit(), kOrigin, 10, options), 2);
    EXPECT_EQ(floor.Damage(0), 20);
    EXPECT_FLOAT_EQ(floor.hits->hit[0].knockback_speed, 0.0f);
    options.except = 1;
    EXPECT_EQ(expanded::PlantBlast(floor.floor.unit(), kOrigin, 10, options), 1);
}

TEST(Blast, AMonsterAtItsCentreIsThrownARandomWay) {
    Floor floor({-5.0f}); // its root on the blast
    expanded::PlantBlast(floor.floor.unit(), kOrigin, 10);
    const COLLISION_HIT &hit = floor.hits->hit[0];
    EXPECT_GT(hit.knockback_speed, 0.0f);
    EXPECT_NEAR(std::hypot(hit.knockback_origin[0], hit.knockback_origin[2]), 1.0f, 0.01f);
}

TEST(Blast, LeavesTheSparedAlone) {
    Floor floor({5.0f, 20.0f});
    expanded::BlastOptions options;
    options.spared = 1 << 0;
    EXPECT_EQ(expanded::PlantBlast(floor.floor.unit(), kOrigin, 10, options), 1);
    EXPECT_EQ(floor.Damage(0), 30); // the one at 20, the spared one skipped
}

TEST(Blast, CarriesTheElementAskedFor) {
    {
        Floor floor({5.0f});
        expanded::PlantBlast(floor.floor.unit(), kOrigin, 10);
        EXPECT_EQ(floor.hits->hit[0].flags, 0); // none of its own
    }
    Floor                  floor({5.0f});
    expanded::BlastOptions options;
    options.element = WEAPON_ELEMENT_THUNDER;
    expanded::PlantBlast(floor.floor.unit(), kOrigin, 10, options);
    EXPECT_EQ(floor.hits->hit[0].flags, GetWeaponElementAttr(WEAPON_ELEMENT_THUNDER));
}

// The rumble the controller is given after the pad's next step.
int RumbleAfterStep() {
    GamePad.Step();
    return InputGetRumble(0).large_motor;
}

TEST(Blast, IsFeltAsAKnockdown) {
    struct alignas(64) PadDma {
        std::array<unsigned char, 1024> bytes;
    };

    auto dma = std::make_unique<PadDma>();
    ASSERT_EQ(scePadInit(0), 1);
    ASSERT_EQ(scePadPortOpen(0, 0, dma->bytes.data()), 1);
    GamePad.VibrationEnable(1);

    Floor floor({5.0f});
    expanded::PlantBlast(floor.floor.unit(), kOrigin, 10);
    EXPECT_EQ(RumbleAfterStep(), 0xE6);
    for (int frame = 1; frame < 22; frame++) {
        GamePad.Step();
    }
    EXPECT_EQ(RumbleAfterStep(), 0); // 22 frames

    expanded::RumbleBlast(expanded::BlastRumble::kLight);
    EXPECT_EQ(RumbleAfterStep(), 0xDC);
    GamePad.StopVibration();
    RumbleAfterStep();
}

TEST(Blast, AFlatOneTakesTheSameFromAllAndThrowsThemToItsEdge) {
    Floor                  floor({2.0f, 30.0f});
    expanded::BlastOptions options;
    options.flat = true;
    options.throw_to = 60.0f;
    expanded::PlantBlast(floor.floor.unit(), kOrigin, 10, options);
    EXPECT_EQ(floor.Damage(0), 10);
    EXPECT_EQ(floor.Damage(1), 10);
    // The nearer is thrown the harder, each to the edge.
    EXPECT_GT(floor.hits->hit[0].knockback_speed, floor.hits->hit[1].knockback_speed);
    EXPECT_FLOAT_EQ(expanded::BlastThrowSpeed(60.0f, 60.0f), 0.0f);
}
