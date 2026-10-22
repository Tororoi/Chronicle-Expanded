#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"

namespace {

using expanded::SolarPhase;

// Toan with the Sun Sword on a floor of two Master Jackets, one near and one out of reach, the hits recorded.
struct Scene {
    expanded::test::TestFloor       floor;
    expanded::test::Wielding        sword{ITEM_WEAPON_SUN_SWORD};
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits = NowColData;
    MOTION_INFO                     motions[12] = {};

    Scene() {
        NowColData = hits.get();
        sword.weapon().attack = 40;
        floor.status().cur_chara = 0;
        floor.SetUpModel(0, expanded::Monster::MASTER_JACKET);
        for (int monster = 0; monster < 2; monster++) {
            floor.Spawn(monster, 0);
            floor.unit().monster[monster].revealed = -1;
            floor.unit().monster[monster].knockback[2] = 1.0f;
            floor.unit().chara[monster][0].motion_type.motion_info = motions;
        }
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        floor.unit().chara[0][0].SetPosition(100.0f, 0.0f, 0.0f);
        floor.unit().chara[1][0].SetPosition(400.0f, 0.0f, 0.0f);
        BtActStatus = {};
        expanded::SolarFlashReset();
    }

    ~Scene() {
        expanded::SolarFlashReset();
        BtActStatus = {};
        NowColData = saved_hits;
    }

    void Steps(int n) {
        for (int i = 0; i < n; i++) {
            expanded::SolarFlashStep();
        }
    }

    void Guard(bool on) { BtActStatus.guard_mode = on ? 5 : 0; }

    void Swing(float time) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 0x24;
        CharaMain.motion_type.state.time = time;
    }

    int Planted() {
        int count = 0;
        for (int i = 0; i < 96; i++) {
            count += hits->active[i] != 0 ? 1 : 0;
        }
        return count;
    }
};

} // namespace

TEST(SolarFlash, GuardHeldPrimesTheBladeAndReleasedEarlyDropsIt) {
    Scene scene;
    scene.Guard(true);
    scene.Steps(expanded::kSolarChargeFrames - 10);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kCharging);
    EXPECT_GT(expanded::SceneDarkness(), 0.0f);
    // A blocked hit drops the guard for a few frames without losing the charge.
    scene.Guard(false);
    scene.Steps(expanded::kSolarGuardGrace);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kCharging);
    scene.Steps(1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kIdle);
    EXPECT_EQ(expanded::SceneDarkness(), 0.0f);

    scene.Guard(true);
    scene.Steps(expanded::kSolarChargeFrames + 1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kPrimed);
    scene.Guard(false);
    scene.Steps(10);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kPrimed);
}

TEST(SolarFlash, TheSwingFlashesAtItsHitFrameStrikesThoseInReachAndBlinds) {
    Scene scene;
    scene.Guard(true);
    scene.Steps(expanded::kSolarChargeFrames + 1);
    scene.Guard(false);

    scene.Swing(822.0f); // winding up
    scene.Steps(1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kWindup);
    EXPECT_EQ(scene.Planted(), 0);

    scene.Swing(826.0f);
    scene.Steps(1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kIdle);
    EXPECT_TRUE(expanded::SceneFlashing());
    EXPECT_TRUE(expanded::Blinding());
    ASSERT_EQ(scene.Planted(), 1); // the far one is out of reach
    const COLLISION_HIT &hit = scene.hits->hit[0];
    EXPECT_EQ(hit.damage, 10); // a quarter of the attack
    EXPECT_EQ(hit.flags, 0);   // elementless
    EXPECT_FLOAT_EQ(hit.pos[0], 100.0f);
    EXPECT_FLOAT_EQ(hit.radius, expanded::kSolarHitRadius);
    EXPECT_EQ(hit.knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit.knockback_speed, expanded::kSolarKickSpeed);

    // No new charge while the floor is blinded.
    BtActStatus.action_on = 0;
    scene.Guard(true);
    scene.Steps(expanded::kSolarChargeFrames + 1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kIdle);
}

TEST(SolarFlash, AnUnusedChargeFadesAway) {
    Scene scene;
    scene.Guard(true);
    scene.Steps(expanded::kSolarChargeFrames + 1);
    scene.Guard(false);
    scene.Steps(expanded::kSolarPrimedFrames + 1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kFading);
    scene.Steps(expanded::kSolarFadeFrames + 1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kIdle);
    EXPECT_EQ(expanded::SceneDarkness(), 0.0f);
    EXPECT_FALSE(expanded::Blinding());
}

TEST(SolarFlash, NeedsTheSwordInToansHand) {
    Scene scene;
    scene.sword.weapon().item_no = ITEM_WEAPON_DAGGER;
    scene.Guard(true);
    scene.Steps(expanded::kSolarChargeFrames + 1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kIdle);
}

TEST(SolarFlash, TheGlowIsTheTorchRecolouredKeepingItsAlpha) {
    // A 4x4 torch: bright in the middle, dim at the corners.
    uint8_t torch[4 * 4 * 4] = {};
    for (int i = 0; i < 16; i++) {
        int  x = i % 4, y = i / 4;
        bool middle = (x == 1 || x == 2) && (y == 1 || y == 2);
        torch[i * 4] = torch[i * 4 + 1] = torch[i * 4 + 2] = middle ? 200 : 40;
        torch[i * 4 + 3] = middle ? 255 : 60;
    }
    auto glow = expanded::RecolourGlow(torch, 4, 4, {255, 238, 180}, {255, 176, 40});
    ASSERT_EQ(glow.size(), sizeof(torch));
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(glow[i * 4 + 3], torch[i * 4 + 3]);
        EXPECT_GE(glow[i * 4], glow[i * 4 + 2]); // gold: more red than blue
    }
    EXPECT_GT(glow[5 * 4], glow[0]); // the middle brighter than the corner
}

TEST(SolarFlash, SolarShotRidesThePelletAndFlashesWhereItEnds) {
    Scene scene;
    scene.sword.weapon().item_no = ITEM_WEAPON_SUPER_STEVE;
    scene.sword.weapon().attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    scene.sword.weapon().attach[0].sphere_weapon_no = ITEM_WEAPON_SUN_SWORD;
    scene.floor.status().cur_chara = CHARA_XIAO;
    auto shots = std::make_unique<CSHOT>();

    scene.Guard(true);
    scene.Steps(expanded::kSolarChargeFrames + 1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kCharging); // the slingshot takes longer
    scene.Steps(expanded::kSolarShotChargeFrames - expanded::kSolarChargeFrames);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kPrimed);
    scene.Guard(false);

    // A swing's action does not spend it; the pellet does.
    scene.Swing(826.0f);
    scene.Steps(1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kPrimed);
    shots->size[3] = 1.0f;
    expanded::SolarShotFired(*shots, 3);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kFlying);
    EXPECT_FLOAT_EQ(shots->size[3], expanded::kSolarPelletSize);
    scene.Steps(10);
    EXPECT_EQ(scene.Planted(), 0);

    // Another pellet's end is not this one's.
    float near[4] = {110.0f, 0.0f, 0.0f, 1.0f};
    expanded::SolarShotEnded(*shots, 4, near, true);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kFlying);

    // It met the near monster: the room plunges, then the flash from there spares it and reaches the far one.
    expanded::SolarShotEnded(*shots, 3, near, true);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kLanding);
    scene.Steps(expanded::kRampFrames - 1);
    EXPECT_FALSE(expanded::SceneFlashing());
    EXPECT_GT(expanded::SceneDarkness(), 0.35f);
    scene.Steps(1);
    EXPECT_TRUE(expanded::SceneFlashing());
    EXPECT_TRUE(expanded::Blinding());
    ASSERT_EQ(scene.Planted(), 1);
    EXPECT_FLOAT_EQ(scene.hits->hit[0].pos[0], 400.0f);
}

TEST(SolarFlash, SolarShotNeedsTheSunSwordsSphere) {
    Scene scene;
    scene.sword.weapon().item_no = ITEM_WEAPON_SUPER_STEVE;
    scene.floor.status().cur_chara = CHARA_XIAO;
    scene.Guard(true);
    scene.Steps(expanded::kSolarShotChargeFrames + 1);
    EXPECT_EQ(expanded::SolarFlashPhase(), SolarPhase::kIdle);
}
