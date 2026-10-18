#include <gtest/gtest.h>

#include <memory>

#include "btactstatus.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/toan/small_sword.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::test::Wielding;

namespace {

// A character to step, and the action status put back after.
struct Swing {
    std::unique_ptr<CMonstorUnit> unit = std::make_unique<CMonstorUnit>();
    BT_ACT_STATUS                 saved = BtActStatus;

    explicit Swing(int action_no) { BtActStatus.action_no = action_no; }

    ~Swing() { BtActStatus = saved; }

    float &Time() { return unit->chara[0][0].motion_type.state.time; }

    CCharacter &Chara() { return unit->chara[0][0]; }
};

} // namespace

TEST(SmallSword, OpeningSwingSkipsItsWindUp) {
    for (int weapon : {ITEM_WEAPON_SMALL_SWORD, ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_HEAVEN_S_CLOUD}) {
        Wielding wielding(weapon);
        Swing    swing(0x24);

        // The step-in plays first.
        swing.Time() = 820.3f;
        expanded::QuickDrawSwing(swing.Chara());
        EXPECT_FLOAT_EQ(swing.Time(), 820.3f);

        swing.Time() = 820.6f;
        expanded::QuickDrawSwing(swing.Chara());
        EXPECT_FLOAT_EQ(swing.Time(), 824.0f);

        swing.Time() = 826.0f;
        expanded::QuickDrawSwing(swing.Chara());
        EXPECT_FLOAT_EQ(swing.Time(), 826.0f);
    }
}

TEST(SmallSword, OtherSwingsAndWeaponsPlayAsRetail) {
    {
        Wielding wielding(ITEM_WEAPON_DAGGER);
        Swing    swing(0x24);
        swing.Time() = 820.6f;
        expanded::QuickDrawSwing(swing.Chara());
        EXPECT_FLOAT_EQ(swing.Time(), 820.6f);
    }

    Wielding wielding(ITEM_WEAPON_SMALL_SWORD);
    Swing    swing(0x25);
    swing.Time() = 820.6f;
    expanded::QuickDrawSwing(swing.Chara());
    EXPECT_FLOAT_EQ(swing.Time(), 820.6f);
}

TEST(SmallSword, SphereShotFiresOnRelease) {
    for (int action : {11, 12}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_TSUKIKAGE);
        Swing    shot(action);
        shot.Time() = 243.0f;
        expanded::QuickDrawShot(shot.Chara(), 0);
        EXPECT_EQ(BtActStatus.action_no, 13);
        EXPECT_EQ(BtActStatus.motion_no, 13);
        EXPECT_EQ(BtActStatus.action_step, 1);
        EXPECT_FLOAT_EQ(shot.Time(), 251.0f);
    }
}

TEST(SmallSword, ShotPlaysAsRetailOtherwise) {
    {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
        Swing    shot(11);
        shot.Time() = 243.0f;
        expanded::QuickDrawShot(shot.Chara(), 0);
        EXPECT_EQ(BtActStatus.action_no, 11);
        EXPECT_FLOAT_EQ(shot.Time(), 243.0f);
    }

    Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SMALL_SWORD);
    {
        // Aimed in first person.
        Swing shot(11);
        shot.Time() = 15.0f;
        expanded::QuickDrawShot(shot.Chara(), 1);
        EXPECT_EQ(BtActStatus.action_no, 11);
    }

    Swing shot(13);
    shot.Time() = 255.0f;
    expanded::QuickDrawShot(shot.Chara(), 0);
    EXPECT_FLOAT_EQ(shot.Time(), 255.0f);
}

TEST(SmallSword, SphereRefillsXiaosGauge) {
    expanded::test::TestFloor floor;
    UserStatus->cur_chara = CHARA_XIAO;
    {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HEAVEN_S_CLOUD);
        EXPECT_EQ(expanded::QuickDrawGaugeSpeed(99), expanded::kQuickDrawShotSpeed);
    }
    {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE);
        EXPECT_EQ(expanded::QuickDrawGaugeSpeed(99), 99);
    }

    // Toan's Small Sword keeps its speed.
    UserStatus->cur_chara = CHARA_TOAN;
    Wielding wielding(ITEM_WEAPON_SMALL_SWORD);
    EXPECT_EQ(expanded::QuickDrawGaugeSpeed(40), 40);
}
