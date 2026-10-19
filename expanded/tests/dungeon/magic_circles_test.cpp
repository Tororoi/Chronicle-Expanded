#include <gtest/gtest.h>

#include <cstdlib>

#include "dungeonmap.hpp"
#include "expanded/dungeon/magic_circles.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// A save whose Toan wields a Baselard (a bag of eight free slots), owning the amplifying weapons asked for.
struct Circle {
    expanded::test::TestSave  save;
    expanded::test::TestFloor floor;
    WEAPON_HAVE              *weapon;
    WEAPON_HAVE              *saved_weapon = NowWeaponHave;

    Circle(bool crysknife, bool hammer) {
        UserStatus = &save.status();
        CUserStatus &status = save.status();
        status.party_size = 3;
        status.cur_chara = 0;
        status.equipped_weapon_slot[0] = 0;
        weapon = &status.chara_weapons[0][0];
        weapon->item_no = ITEM_WEAPON_BASELARD; // not a default weapon, which the rolls leave alone
        weapon->attack = 20;
        weapon->durability = 40;
        weapon->durability_f = 40.0f;
        NowWeaponHave = weapon;
        status.chara_weapons[0][1].item_no = crysknife ? ITEM_WEAPON_CRYSKNIFE : 0;
        status.chara_weapons[2][0].item_no = hammer ? ITEM_WEAPON_MAGICAL_HAMMER : 0;
        status.item_pack.num = 8;
        for (s16 &item : status.item_pack.item) {
            item = -1;
        }
    }

    ~Circle() { NowWeaponHave = saved_weapon; }

    bool Run(int kind) { return expanded::AmplifiedCircle(kind, *weapon); }

    int Carried(int item_no) {
        int count = 0;
        for (s16 item : save.status().item_pack.item) {
            count += item == item_no;
        }
        return count;
    }

    CUserStatus &Status() { return save.status(); }
};

} // namespace

TEST(MagicCircles, OwningEachAmplifierAddsOne) {
    EXPECT_EQ(Circle(false, false).Run(TRAP_CIRCLE_MONEY_UP), false); // retail's
    {
        Circle circle(true, false);
        EXPECT_EQ(expanded::CircleMagnitude(), 2);
    }
    {
        Circle circle(false, true);
        EXPECT_EQ(expanded::CircleMagnitude(), 2);
    }
    {
        Circle circle(true, true);
        EXPECT_EQ(expanded::CircleMagnitude(), 3);
    }
}

TEST(MagicCircles, MoneyAndTimersScale) {
    Circle circle(true, true);
    circle.Status().money = 100;
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_MONEY_UP));
    EXPECT_EQ(circle.Status().money, 100 + 3 * (120 + 10));
    circle.Status().money = 100;
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_MONEY_DOWN));
    EXPECT_EQ(circle.Status().money, static_cast<int>(100 - 0.2f * 3 * 100)); // retail's float arithmetic: 39

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_STAMINA));
    EXPECT_EQ(circle.Status().ailment_frames[0], 1800 * 3);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ALL_MONSTER_STAMINA));
    EXPECT_EQ(circle.floor.unit().monster[7].anger_timer, 900);
}

TEST(MagicCircles, WeaponRollsScaleAndTheQuarterTakesWhpToOne) {
    Circle circle(true, false);
    std::srand(3);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_UP));
    EXPECT_GE(circle.weapon->durability, 40 + 6);
    EXPECT_LE(circle.weapon->durability, 40 + 10);
    EXPECT_EQ(circle.weapon->durability % 2, 0); // a roll of 3-5, doubled

    int before = circle.weapon->durability;
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_DOWN));
    EXPECT_GE(before - circle.weapon->durability, 6);
    EXPECT_LE(before - circle.weapon->durability, 10);

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_QUARTER));
    EXPECT_EQ(circle.weapon->durability_f, 1.0f);
}

namespace {

// Xiao's equipped slingshot and the Crysknife in Toan's bag, both empty of ABS and worn.
void Wear(Circle &circle) {
    CUserStatus &status = circle.Status();
    status.equipped_weapon_slot[1] = 0;
    for (WEAPON_HAVE *weapon : {circle.weapon, &status.chara_weapons[1][0], &status.chara_weapons[0][1]}) {
        weapon->experience = 0;
        weapon->durability = 40;
        weapon->durability_f = 5.0f;
    }
    status.chara_weapons[1][0].item_no = ITEM_WEAPON_BANDIT_SLINGSHOT;
}

} // namespace

TEST(MagicCircles, DoubleFillsReachEveryEquippedWeapon) {
    Circle circle(true, false);
    Wear(circle);
    WEAPON_HAVE &slingshot = circle.Status().chara_weapons[1][0];
    WEAPON_HAVE &crysknife = circle.Status().chara_weapons[0][1];

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
    EXPECT_GT(circle.weapon->experience, 0);
    EXPECT_GT(slingshot.experience, 0);
    EXPECT_EQ(crysknife.experience, 0);

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_CURE));
    EXPECT_EQ(circle.weapon->durability_f, 40.0f);
    EXPECT_EQ(slingshot.durability_f, 40.0f);
    EXPECT_EQ(crysknife.durability_f, 5.0f);
    EXPECT_EQ(circle.Carried(ITEM_POWERUP_POWDER), 0);
}

TEST(MagicCircles, TripleFillsReachEveryCarriedWeapon) {
    Circle circle(true, true);
    Wear(circle);
    WEAPON_HAVE &crysknife = circle.Status().chara_weapons[0][1];

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
    EXPECT_GT(crysknife.experience, 0);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_CURE));
    EXPECT_EQ(crysknife.durability_f, 40.0f);
}

TEST(MagicCircles, AbsFullLeavesAnOvertrainedWeaponAsItIs) {
    for (bool amplified : {false, true}) {
        Circle circle(amplified, false);
        circle.weapon->experience = 0;
        ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
        s16 max = circle.weapon->experience;
        ASSERT_GT(max, 0);

        // Past its max: not lowered to it, not raised further.
        circle.weapon->experience = static_cast<s16>(max + max / 2);
        ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
        EXPECT_EQ(circle.weapon->experience, max + max / 2);
    }
}
