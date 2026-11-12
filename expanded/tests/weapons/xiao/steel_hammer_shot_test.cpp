#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/weapons/toan/baselard.hpp"
#include "expanded/weapons/xiao/steel_hammer_shot.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"

TEST(SteelHammerShot, SuperSteveWithItsSphereFiresHeads) {
    {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_STEEL_HAMMER);
        EXPECT_TRUE(expanded::SteelHammerPellets());
    }
    expanded::test::Wielding other(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_KITCHEN_KNIFE);
    EXPECT_FALSE(expanded::SteelHammerPellets());
}

TEST(SteelHammerShot, ItsPelletsThrowAsTheBaselardDoes) {
    COLLISION_HIT pellet = {};
    pellet.owner = 1;
    pellet.attack_no = 0;
    {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_STEEL_HAMMER);
        EXPECT_TRUE(expanded::HeavyHandHit(pellet));
    }
    expanded::test::Wielding goro(ITEM_WEAPON_STEEL_HAMMER);
    COLLISION_HIT            swing = {};
    swing.owner = 2; // Goro's own swing: not the sphere's
    EXPECT_FALSE(expanded::HeavyHandHit(swing));
}

TEST(SteelHammerShot, ItsPelletsFlyAtHalfSpeedAsFar) {
    CSHOT shots = {};
    shots.vector[2][0] = 4.0f;
    shots.vector[2][2] = -6.0f;
    shots.life[2] = 30;
    {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_STEEL_HAMMER);
        expanded::SteelHeadShotFired(shots, 2);
    }
    EXPECT_FLOAT_EQ(shots.vector[2][0], 2.0f);
    EXPECT_FLOAT_EQ(shots.vector[2][2], -3.0f);
    EXPECT_EQ(shots.life[2], 60);

    expanded::test::Wielding other(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_KITCHEN_KNIFE);
    expanded::SteelHeadShotFired(shots, 2);
    EXPECT_FLOAT_EQ(shots.vector[2][0], 2.0f);
    EXPECT_EQ(shots.life[2], 60);
}
