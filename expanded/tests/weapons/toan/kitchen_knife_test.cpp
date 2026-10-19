#include <gtest/gtest.h>

#include "expanded/weapons/toan/kitchen_knife.hpp"
#include "itemdata.hpp"
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
