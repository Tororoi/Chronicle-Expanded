#include <gtest/gtest.h>

#include <memory>

#include "expanded/weapons/toan/shamshir.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::test::Wielding;

namespace {

// A character playing motion_no, from a table where every motion steps 0.3.
struct Playing {
    std::unique_ptr<CMonstorUnit> unit = std::make_unique<CMonstorUnit>();
    MOTION_INFO                   motions[0x30] = {};

    explicit Playing(int motion_no, float motion_speed = -1.0f) {
        for (MOTION_INFO &info : motions) {
            info.speed = 0.3f;
        }
        Chara().motion_type.motion_info = motions;
        Chara().motion_no = motion_no;
        Chara().motion_speed = motion_speed;
    }

    CCharacter &Chara() { return unit->chara[0][0]; }
};

} // namespace

TEST(Shamshir, ComboSwingsPlayFaster) {
    for (int weapon : {ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_DUSACK, ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_ATLAMILLIA_SWORD,
                       ITEM_WEAPON_CHRONICLE_SWORD}) {
        Wielding wielding(weapon);
        for (int swing = 0x24; swing <= 0x28; swing++) {
            Playing playing(swing);
            expanded::SwiftStrikesMotion(playing.Chara(), CHARA_TOAN);
            EXPECT_FLOAT_EQ(playing.Chara().motion_speed, 0.4f);
        }
    }
}

TEST(Shamshir, OtherMotionsAndWeaponsPlayAsRetail) {
    {
        Wielding wielding(ITEM_WEAPON_DAGGER);
        Playing  playing(0x24);
        expanded::SwiftStrikesMotion(playing.Chara(), CHARA_TOAN);
        EXPECT_FLOAT_EQ(playing.Chara().motion_speed, -1.0f);
    }

    Wielding wielding(ITEM_WEAPON_SHAMSHIR);
    for (int motion : {14, 15, 0x23, 0x29}) {
        Playing playing(motion);
        expanded::SwiftStrikesMotion(playing.Chara(), CHARA_TOAN);
        EXPECT_FLOAT_EQ(playing.Chara().motion_speed, -1.0f);
    }

    // A speed the game set stands.
    Playing set(0x24, 0.2f);
    expanded::SwiftStrikesMotion(set.Chara(), CHARA_TOAN);
    EXPECT_FLOAT_EQ(set.Chara().motion_speed, 0.2f);
}

TEST(Shamshir, SphereSpeedsXiaosShot) {
    for (int sphere : {ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_CHRONICLE_SWORD, ITEM_WEAPON_PARTISAN}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);

        Playing draw(11);
        expanded::SwiftStrikesMotion(draw.Chara(), CHARA_XIAO);
        EXPECT_FLOAT_EQ(draw.Chara().motion_speed, 0.3f * expanded::kSwiftDrawFactor);

        Playing hold(12);
        expanded::SwiftStrikesMotion(hold.Chara(), CHARA_XIAO);
        EXPECT_FLOAT_EQ(hold.Chara().motion_speed, -1.0f);

        Playing shot(13);
        expanded::SwiftStrikesMotion(shot.Chara(), CHARA_XIAO);
        EXPECT_FLOAT_EQ(shot.Chara().motion_speed, expanded::kSwiftShotSpeed);
    }

    // Another sphere, or Toan's motion numbers on Xiao.
    Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
    Playing  draw(11);
    expanded::SwiftStrikesMotion(draw.Chara(), CHARA_XIAO);
    EXPECT_FLOAT_EQ(draw.Chara().motion_speed, -1.0f);

    Wielding shamshir(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SHAMSHIR);
    Playing  swing(0x24);
    expanded::SwiftStrikesMotion(swing.Chara(), CHARA_XIAO);
    EXPECT_FLOAT_EQ(swing.Chara().motion_speed, -1.0f);
}
