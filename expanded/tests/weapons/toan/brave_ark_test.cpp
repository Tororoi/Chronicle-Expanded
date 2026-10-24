#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/weapons/toan/brave_ark.hpp"
#include "itemdata.hpp"
#include "shot_freefuncs.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

// A floor with the active character (Toan unless given) holding weapon_no, carrying sphere's SynthSphere when given.
struct Equipped {
    expanded::test::TestFloor floor;

    explicit Equipped(int weapon_no, int sphere = 0, int chara = CHARA_TOAN) {
        CUserStatus &status = floor.status();
        status.cur_chara = chara;
        status.equipped_weapon_slot[chara] = 0;
        WEAPON_HAVE &weapon = status.chara_weapons[chara][0];
        weapon = {};
        weapon.item_no = static_cast<s16>(weapon_no);
        if (sphere != 0) {
            weapon.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
            weapon.attach[0].sphere_weapon_no = static_cast<s16>(sphere);
        }
    }

    CUserStatus &status() { return floor.status(); }
};

} // namespace

TEST(BraveArk, AMonstersHitGivesNoneOfTheFour) {
    Equipped      ark(ITEM_WEAPON_BRAVE_ARK);
    COLLISION_HIT hit = {};
    hit.flags = expanded::kHerosCourageHit | 0x1000 | 0x40000; // with Stamina and the money drain
    expanded::HerosCourageHit(hit);
    EXPECT_EQ(hit.flags, 0x1000 | 0x40000);
}

TEST(BraveArk, NoTrapOrScriptGivesThem) {
    Equipped ark(ITEM_WEAPON_BRAVE_ARK);
    for (int ailment : {AILMENT_FREEZE, AILMENT_POISON, AILMENT_CURSE, AILMENT_GOO}) {
        BtSetStatusErr(ailment);
    }
    EXPECT_EQ(ark.status().ailments[CHARA_TOAN], 0);
    BtSetStatusErr(AILMENT_STAMINA);
    EXPECT_EQ(ark.status().ailments[CHARA_TOAN], AILMENT_STAMINA);
}

TEST(BraveArk, OneAlreadyOnIsClearedOnTheNextStep) {
    Equipped ark(ITEM_WEAPON_BRAVE_ARK);
    ark.status().ailments[CHARA_TOAN] = AILMENT_POISON | AILMENT_CURSE;
    expanded::HerosCourageStep();
    EXPECT_EQ(ark.status().ailments[CHARA_TOAN], 0);
}

TEST(BraveArk, OnlyEquippedOrInSuperStevesSphere) {
    {
        Equipped      dagger(ITEM_WEAPON_DAGGER);
        COLLISION_HIT hit = {};
        hit.flags = expanded::kHitPoison;
        expanded::HerosCourageHit(hit);
        EXPECT_EQ(hit.flags, expanded::kHitPoison);
        dagger.status().ailments[CHARA_TOAN] = AILMENT_POISON;
        expanded::HerosCourageStep();
        EXPECT_EQ(dagger.status().ailments[CHARA_TOAN], AILMENT_POISON);
    }
    {
        Equipped steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BRAVE_ARK, CHARA_XIAO);
        steve.status().ailments[CHARA_XIAO] = AILMENT_GOO;
        expanded::HerosCourageStep();
        EXPECT_EQ(steve.status().ailments[CHARA_XIAO], 0);
        EXPECT_FALSE(expanded::HerosCourageAllowsAilment(AILMENT_FREEZE));
    }
}
