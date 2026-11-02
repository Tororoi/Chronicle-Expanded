#include <gtest/gtest.h>

#include <cmath>

#include "collisiondata.hpp"
#include "dngmessageman.hpp"
#include "expanded/weapons/toan/macho_sword.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

// A save whose Toan holds the Macho Sword (or not) in his bag.
struct Save {
    expanded::test::TestSave save;

    explicit Save(bool macho) {
        if (macho) {
            save.status().chara_weapons[0][3].item_no = ITEM_WEAPON_MACHO_SWORD;
        }
    }
};

WEAPON_HAVE Weapon(int abs) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = ITEM_WEAPON_DAGGER;
    weapon.experience = static_cast<s16>(abs);
    return weapon;
}

} // namespace

TEST(MachoSword, WithoutItTheGameFillsToTheMax) {
    Save        save(false);
    WEAPON_HAVE weapon = Weapon(98);
    EXPECT_FALSE(expanded::OvertrainingAbs(weapon, 6, 100));
    EXPECT_EQ(weapon.experience, 98);
}

TEST(MachoSword, OwnedTheAbsFillsPastTheMaxToTwiceIt) {
    Save        save(true);
    WEAPON_HAVE weapon = Weapon(98);
    DngMessMan.message = 0;
    ASSERT_TRUE(expanded::OvertrainingAbs(weapon, 6, 100));
    EXPECT_EQ(weapon.experience, 104);
    EXPECT_EQ(DngMessMan.message, 150); // ABS MAX as it crosses the max

    DngMessMan.message = 0;
    ASSERT_TRUE(expanded::OvertrainingAbs(weapon, 90, 100));
    EXPECT_EQ(weapon.experience, 194);
    EXPECT_EQ(DngMessMan.message, 0);
    ASSERT_TRUE(expanded::OvertrainingAbs(weapon, 50, 100));
    EXPECT_EQ(weapon.experience, 200);

    WEAPON_HAVE low = Weapon(10);
    ASSERT_TRUE(expanded::OvertrainingAbs(low, 5, 100));
    EXPECT_EQ(low.experience, 15);
}

TEST(MachoSword, ALevelUpCarriesTheSurplus) {
    EXPECT_EQ(expanded::OvertrainingCarry(Weapon(116), 100), 16);
    EXPECT_EQ(expanded::OvertrainingCarry(Weapon(100), 100), 0);
    EXPECT_EQ(expanded::OvertrainingCarry(Weapon(40), 100), 0); // a Powerup Powder's level-up
}

TEST(MachoSword, TheGaugeFillsToFull) {
    EXPECT_EQ(expanded::OvertrainingGaugeAbs(150, 100), 100);
    EXPECT_EQ(expanded::OvertrainingGaugeAbs(60, 100), 60);
}

namespace {

// Toan out with the Macho Sword equipped (slot 2), a Dagger in slot 3; Xiao with Super Steve carrying its sphere.
struct Pumping {
    expanded::test::TestFloor floor;

    Pumping() {
        expanded::PumpedUpReset();
        CUserStatus &status = floor.status();
        status.chara_weapons[CHARA_TOAN][2].item_no = ITEM_WEAPON_MACHO_SWORD;
        status.chara_weapons[CHARA_TOAN][3].item_no = ITEM_WEAPON_DAGGER;
        status.equipped_weapon_slot[CHARA_TOAN] = 2;
        WEAPON_HAVE &steve = status.chara_weapons[CHARA_XIAO][0];
        steve.item_no = ITEM_WEAPON_SUPER_STEVE;
        steve.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
        steve.attach[0].sphere_weapon_no = ITEM_WEAPON_MACHO_SWORD;
        status.equipped_weapon_slot[CHARA_XIAO] = 0;
        status.cur_chara = CHARA_TOAN;
    }

    ~Pumping() { expanded::PumpedUpReset(); }

    void Kills(int n) {
        for (int i = 0; i < n; i++) {
            expanded::PumpedUpKill(floor.unit(), 0);
        }
    }
};

} // namespace

TEST(MachoSword, EachMonsterDownedPumpsTheAttackUp) {
    Pumping p;
    EXPECT_EQ(expanded::PumpedUpAttack(100), 100);
    p.Kills(1);
    EXPECT_EQ(expanded::PumpedUpAttack(100), 110);
    p.Kills(1); // the same monster, back up and down again, counts again
    EXPECT_EQ(expanded::PumpedUpKills(CHARA_TOAN), 2);
    EXPECT_EQ(expanded::PumpedUpAttack(100), 121); // 100 x 1.1^2
    p.Kills(18);
    EXPECT_NEAR(expanded::PumpedUpMultiplier(), std::pow(1.1f, 20.0f), 1e-3f); // no limit
}

TEST(MachoSword, ToansArmsSwellUpToTheirFullLookAtFifteen) {
    Pumping p;
    EXPECT_FLOAT_EQ(expanded::PumpedUpSize(expanded::kPumpedForearm), 1.0f);
    p.Kills(5);
    EXPECT_NEAR(expanded::PumpedUpSize(expanded::kPumpedForearm), 1.0f + (expanded::kPumpedUpLook[expanded::kPumpedForearm] - 1.0f) / 3.0f, 1e-5f);
    p.Kills(10);
    EXPECT_FLOAT_EQ(expanded::PumpedUpSize(expanded::kPumpedHand), expanded::kPumpedUpLook[expanded::kPumpedHand]);
    p.Kills(10);
    EXPECT_FLOAT_EQ(expanded::PumpedUpSize(expanded::kPumpedHand), expanded::kPumpedUpLook[expanded::kPumpedHand]);
}

TEST(MachoSword, ItHoldsThroughAChangeOfMemberAndGoesWithTheWeapon) {
    Pumping p;
    p.Kills(3);
    p.floor.status().cur_chara = CHARA_XIAO; // Xiao's Steve counts on his own
    EXPECT_EQ(expanded::PumpedUpKills(CHARA_XIAO), 0);
    p.Kills(2);
    EXPECT_EQ(expanded::PumpedUpKills(CHARA_XIAO), 2);
    expanded::PumpedUpFrame();
    p.floor.status().cur_chara = CHARA_TOAN;
    EXPECT_EQ(expanded::PumpedUpKills(CHARA_TOAN), 3);

    // Another weapon, even for a moment, and the count is gone.
    p.floor.status().equipped_weapon_slot[CHARA_TOAN] = 3;
    EXPECT_EQ(expanded::PumpedUpAttack(100), 100);
    p.Kills(1); // with the Dagger: nothing
    expanded::PumpedUpFrame();
    p.floor.status().equipped_weapon_slot[CHARA_TOAN] = 2;
    EXPECT_EQ(expanded::PumpedUpKills(CHARA_TOAN), 0);
    p.Kills(1);
    EXPECT_EQ(expanded::PumpedUpKills(CHARA_TOAN), 1);
}

TEST(MachoSword, ANewFloorStartsAfresh) {
    Pumping p;
    p.Kills(4);
    expanded::PumpedUpFloorEntered();
    EXPECT_EQ(expanded::PumpedUpKills(CHARA_TOAN), 0);
    EXPECT_FLOAT_EQ(expanded::PumpedUpMultiplier(), 1.0f);
}

TEST(MachoSword, SuperStevesPelletsArePumpedUpToo) {
    Pumping p;
    p.floor.status().cur_chara = CHARA_XIAO;
    p.Kills(2);
    COLLISION_HIT hit = {};
    hit.damage = 100;
    expanded::PumpedUpPelletHit(hit);
    EXPECT_EQ(hit.damage, 121);
}

TEST(MachoSword, XiaosArmsSwellWithSuperSteves) {
    Pumping p;
    p.floor.status().cur_chara = CHARA_XIAO;
    EXPECT_FLOAT_EQ(expanded::PumpedUpSize(expanded::kPumpedForearm), 1.0f);
    p.Kills(15);
    EXPECT_FLOAT_EQ(expanded::PumpedUpSize(expanded::kPumpedForearm), expanded::kPumpedUpLook[expanded::kPumpedForearm]);
    ASSERT_NE(expanded::PumpedUpRigOf(CHARA_XIAO), nullptr);
    EXPECT_STREQ(expanded::PumpedUpRigOf(CHARA_XIAO)->weapon_hand, "l_hand");
    EXPECT_EQ(expanded::PumpedUpRigOf(CHARA_GORO), nullptr);
}
