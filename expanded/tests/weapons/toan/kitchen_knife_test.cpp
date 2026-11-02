#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/weapons/kick_back.hpp"
#include "expanded/weapons/kitchen_knife_shot.hpp"
#include "expanded/weapons/toan/baselard.hpp"
#include "expanded/weapons/toan/kitchen_knife.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"

using expanded::test::TestFloor;
using expanded::test::Wielding;

namespace {

// Toan with the knife in slot 0 at whp of 40, and no blessing to start with.
struct Spring {
    TestFloor    floor;
    WEAPON_HAVE &knife = floor.status().chara_weapons[CHARA_TOAN][0];

    explicit Spring(float whp = 40.0f) {
        floor.status().cur_chara = CHARA_TOAN;
        floor.status().equipped_weapon_slot[CHARA_TOAN] = 0;
        knife.item_no = ITEM_WEAPON_KITCHEN_KNIFE;
        knife.durability = 40;
        knife.durability_f = whp;
        Clear();
    }

    ~Spring() { Clear(); }

    // A step without the knife out ends any blessing.
    static void Clear() {
        Wielding dagger(ITEM_WEAPON_DAGGER);
        expanded::SpringsBlessingStep();
        expanded::SpringsBlessingStep();
    }

    // One step, standing in the spring or not.
    static void Step(bool in_spring) {
        expanded::SpringsBlessingStep();
        if (in_spring) {
            expanded::SpringsBlessingSoak();
        }
    }
};

} // namespace

TEST(KitchenKnife, SpringBlessesTheKnife) {
    Spring   spring;
    Wielding wielding(ITEM_WEAPON_KITCHEN_KNIFE);
    EXPECT_EQ(expanded::SpringsBlessingAttack(30), 30);

    Spring::Step(true);
    EXPECT_EQ(expanded::SpringsBlessingLeft(), expanded::kBlessingSteps);
    EXPECT_EQ(expanded::SpringsBlessingAttack(30), 60);

    // Out of the water it counts down, and the attack holds until it ends.
    for (int i = 0; i < 100; i++) {
        Spring::Step(false);
    }
    EXPECT_EQ(expanded::SpringsBlessingLeft(), expanded::kBlessingSteps - 100);
    EXPECT_EQ(expanded::SpringsBlessingAttack(30), 60);

    // Back in, fresh again.
    Spring::Step(true);
    EXPECT_EQ(expanded::SpringsBlessingLeft(), expanded::kBlessingSteps);

    for (int i = 0; i < expanded::kBlessingSteps; i++) {
        Spring::Step(false);
    }
    EXPECT_EQ(expanded::SpringsBlessingLeft(), 0);
    EXPECT_EQ(expanded::SpringsBlessingAttack(30), 30);
}

TEST(KitchenKnife, SteppingInMendsTheKnife) {
    Spring   spring(12.0f);
    Wielding wielding(ITEM_WEAPON_KITCHEN_KNIFE);
    Spring::Step(true);
    EXPECT_FLOAT_EQ(spring.knife.durability_f, 40.0f);

    // Only a step in mends it, not standing there.
    spring.knife.durability_f = 20.0f;
    Spring::Step(true);
    EXPECT_FLOAT_EQ(spring.knife.durability_f, 20.0f);

    Spring::Step(false);
    Spring::Step(true);
    EXPECT_FLOAT_EQ(spring.knife.durability_f, 40.0f);
}

TEST(KitchenKnife, OnlyTheKnifeIsBlessed) {
    Spring spring;
    {
        Wielding dagger(ITEM_WEAPON_DAGGER);
        Spring::Step(true);
        EXPECT_EQ(expanded::SpringsBlessingLeft(), 0);
    }

    // Putting the knife away ends the blessing.
    Wielding knife(ITEM_WEAPON_KITCHEN_KNIFE);
    Spring::Step(true);
    EXPECT_GT(expanded::SpringsBlessingLeft(), 0);
    {
        Wielding dagger(ITEM_WEAPON_DAGGER);
        Spring::Step(false);
        EXPECT_EQ(expanded::SpringsBlessingLeft(), 0);
    }

    // Another character in the water blesses nothing.
    spring.floor.status().cur_chara = CHARA_XIAO;
    Spring::Step(true);
    EXPECT_EQ(expanded::SpringsBlessingLeft(), 0);
}

namespace {

COLLISION_HIT PelletHit(int owner, int attack) {
    COLLISION_HIT record = {};
    record.owner = owner;
    record.attack_no = attack;
    return record;
}

} // namespace

TEST(KitchenKnife, SuperStevesPelletsWithItsSphereAreGems) {
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_KITCHEN_KNIFE);
    EXPECT_TRUE(expanded::KitchenKnifePellets());
    EXPECT_TRUE(expanded::KitchenKnifeHit(PelletHit(1, 0)));
    EXPECT_FALSE(expanded::KitchenKnifeHit(PelletHit(0, 0)));
    EXPECT_FALSE(expanded::KitchenKnifeHit(PelletHit(1, 2)));

    steve.weapon().attach[0].sphere_weapon_no = ITEM_WEAPON_BASELARD;
    EXPECT_FALSE(expanded::KitchenKnifePellets());
    EXPECT_FALSE(expanded::KitchenKnifeHit(PelletHit(1, 0)));
}

TEST(KitchenKnife, AGemThrowsHalfAsFarAsABaselardPellet) {
    auto carries = [](expanded::KickBack kick) { return kick.speed * kick.speed / (2 * kick.decay); };
    EXPECT_NEAR(carries(expanded::kGemKickBack), carries(expanded::kHeavyKickBack) / 2, 0.01f);
    EXPECT_FALSE(expanded::HeavyHandHit(PelletHit(1, 0))); // the Baselard's own throw is not added
}

TEST(KitchenKnife, APelletKeepsItsSpriteWithNoGemLoaded) {
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_KITCHEN_KNIFE);
    CSHOT                    shots = {};
    shots.used[0] = 1;
    shots.size[0] = 1.5f;
    expanded::GemShotFired(shots, 0);
    EXPECT_FLOAT_EQ(shots.size[0], 1.5f);
    expanded::GemShotEnded(shots, 0);
}
