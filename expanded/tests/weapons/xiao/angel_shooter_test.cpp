#include <gtest/gtest.h>

#include "btactstatus.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/heal.hpp"
#include "expanded/weapons/xiao/angel_shooter.hpp"
#include "healeffect.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern s32 NewChangeFxFlag;
} // namespace dc::game

namespace {

using expanded::test::TestFloor;
using expanded::test::Wielding;

constexpr int kHealAbility = 0x800;

// Xiao with weapon_no (carrying sphere when given), its Heal ability on when heal, 50 HP.
class GuardianGrace : public ::testing::Test {
protected:
    explicit GuardianGrace(int weapon_no = ITEM_WEAPON_ANGEL_SHOOTER, int sphere = 0, bool heal = true) : wielding_(weapon_no, sphere) {
        if (heal) {
            wielding_.weapon().flags = kHealAbility;
        }
    }

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_XIAO;
        floor_.status().hp[CHARA_XIAO] = 50;
        HealEffect.active = 0;
        NewChangeFxFlag = 0;
        expanded::GuardianGraceReset();
    }

    void TearDown() override {
        expanded::GuardianGraceReset();
        BtActStatus = {};
        HealEffect.active = 0;
        NewChangeFxFlag = 0;
    }

    void Guard(bool on) { BtActStatus.guard_mode = on ? 5 : 0; }

    // The heal tick over frames as the dungeon runs it: how many times it heals.
    int Heals(int frames) {
        int heals = 0;
        for (int frame = 0; frame < frames; frame++) {
            expanded::GuardianGraceStep();
            counter_++;
            counter_ = expanded::GuardianGraceCounter(counter_, expanded::kHealPeriod);
            if (counter_ >= expanded::kHealPeriod) {
                if ((wielding_.weapon().flags & kHealAbility) || expanded::GuardianGraceHeals()) {
                    heals++;
                }
                counter_ = 0;
            }
        }
        return heals;
    }

    TestFloor floor_;
    Wielding  wielding_;
    int       counter_ = 0;
};

class GuardianGraceStripped : public GuardianGrace {
protected:
    GuardianGraceStripped() : GuardianGrace(ITEM_WEAPON_ANGEL_SHOOTER, 0, false) {}
};

class GuardianGraceBySuperSteve : public GuardianGrace {
protected:
    GuardianGraceBySuperSteve() : GuardianGrace(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_ANGEL_SHOOTER, false) {}
};

class GuardianGraceByAngelGear : public GuardianGrace {
protected:
    GuardianGraceByAngelGear() : GuardianGrace(ITEM_WEAPON_ANGE_GEAR) {}
};

class GuardianGraceBySuperSteveWithTheAngelGear : public GuardianGrace {
protected:
    GuardianGraceBySuperSteveWithTheAngelGear() : GuardianGrace(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_ANGE_GEAR, false) {}
};

class GuardianGraceWithAnotherWeapon : public GuardianGrace {
protected:
    GuardianGraceWithAnotherWeapon() : GuardianGrace(ITEM_WEAPON_DIVINEBEASTTITLE) {}
};

} // namespace

TEST_F(GuardianGrace, GuardingTheTickHealsEverySecond) {
    Guard(true);
    EXPECT_EQ(Heals(expanded::kGuardianGraceFrames - 1), 0); // the first a second into the guard
    EXPECT_EQ(Heals(1), 1);
    EXPECT_EQ(Heals(expanded::kGuardianGraceFrames * 10), 10);
}

TEST_F(GuardianGrace, UnguardedTheTickKeepsItsPace) {
    Guard(false);
    EXPECT_EQ(Heals(expanded::kHealPeriod * 3), 3);
}

TEST_F(GuardianGrace, TheGuardStartsWithTheSpringsSparkleOnce) {
    Guard(true);
    Heals(1);
    EXPECT_EQ(HealEffect.active, 1);
    HealEffect.active = 0;
    Heals(30);
    EXPECT_EQ(HealEffect.active, 0);
    Guard(false);
    Heals(1);
    Guard(true);
    Heals(1);
    EXPECT_EQ(HealEffect.active, 1);
}

TEST_F(GuardianGrace, HerColourSwellsWithTheTick) {
    Guard(true);
    Heals(1); // the counter held at the period less a second: the swell's height
    float high[3] = {}, low[3] = {};
    expanded::GuardianGraceLight(CharaMain, high);
    EXPECT_FLOAT_EQ(high[0], expanded::kGuardianGraceHigh[0]);
    Heals(expanded::kGuardianGraceFrames / 2);
    expanded::GuardianGraceLight(CharaMain, low);
    EXPECT_NEAR(low[0], expanded::kGuardianGraceLow[0], 0.1f);
    Guard(false);
    Heals(1);
    float none[3] = {};
    expanded::GuardianGraceLight(CharaMain, none);
    EXPECT_FLOAT_EQ(none[0], 0.0f);
}

TEST_F(GuardianGraceStripped, AStrippedHealGivesNothing) {
    Guard(true);
    EXPECT_FALSE(expanded::GuardianGraceOn());
    EXPECT_EQ(Heals(600), 0);
}

TEST_F(GuardianGraceBySuperSteve, TheSphereHealsWhileGuarding) {
    Guard(true);
    EXPECT_EQ(Heals(expanded::kGuardianGraceFrames * 10), 10);
    Guard(false);
    EXPECT_EQ(Heals(expanded::kHealPeriod * 3), 0);
}

TEST_F(GuardianGraceWithAnotherWeapon, NeverQuickensTheTick) {
    Guard(true);
    EXPECT_FALSE(expanded::GuardianGraceCarried());
    EXPECT_EQ(Heals(expanded::kHealPeriod * 3), 3);
}

TEST_F(GuardianGraceByAngelGear, GuardingTheTickHealsEveryHalfSecond) {
    Guard(true);
    EXPECT_EQ(Heals(expanded::kGuardianGraceGearFrames - 1), 0);
    EXPECT_EQ(Heals(1), 1);
    EXPECT_EQ(Heals(expanded::kGuardianGraceGearFrames * 10), 10);
    Guard(false);
    EXPECT_EQ(Heals(expanded::kHealPeriod * 3), 3);
}

TEST_F(GuardianGraceByAngelGear, ItsColourStillSwellsOnceASecond) {
    Guard(true);
    Heals(1); // the swell's height
    float high[3] = {}, low[3] = {}, again[3] = {};
    expanded::GuardianGraceLight(CharaMain, high);
    EXPECT_FLOAT_EQ(high[0], expanded::kGuardianGraceGearHigh[0]);
    Heals(expanded::kGuardianGraceGearFrames); // a tick: half a swell
    expanded::GuardianGraceLight(CharaMain, low);
    EXPECT_NEAR(low[0], expanded::kGuardianGraceGearLow[0], 0.1f);
    Heals(expanded::kGuardianGraceGearFrames); // the next: the height again
    expanded::GuardianGraceLight(CharaMain, again);
    EXPECT_FLOAT_EQ(again[0], expanded::kGuardianGraceGearHigh[0]);
}

TEST_F(GuardianGraceByAngelGear, ItsGraceComesWithTheChangeBurstAndGlowsGold) {
    Guard(true);
    EXPECT_EQ(Heals(expanded::kGuardianGraceGearFrames * 10), 10);
    EXPECT_EQ(NewChangeFxFlag, 1);
    EXPECT_EQ(HealEffect.active, 0);
    Guard(false);
    Heals(1);
    Guard(true);
    Heals(1); // the swell's height
    float high[3] = {};
    expanded::GuardianGraceLight(CharaMain, high);
    for (int i = 0; i < 3; i++) {
        EXPECT_FLOAT_EQ(high[i], expanded::kGuardianGraceGearHigh[i]);
    }
}

TEST_F(GuardianGraceBySuperSteveWithTheAngelGear, TheGearsSphereHealsEveryHalfSecondWhileGuarding) {
    Guard(true);
    EXPECT_EQ(Heals(expanded::kGuardianGraceGearFrames * 10), 10);
    EXPECT_EQ(NewChangeFxFlag, 1);
}
