#include <gtest/gtest.h>

#include "dun/gameloop.hpp"
#include "expanded/weapons/goro/channel_elements.hpp"
#include "itemdata.hpp"
#include "motionmodel.hpp"
#include "test_support.hpp"
#include "weaponelement.hpp"

using expanded::test::TestFloor;

namespace {

constexpr s8 kNoPoints[5] = {0, 0, 0, 0, 0};

} // namespace

TEST(ChannelElements, OnlyTheMagicalHammerChannels) {
    WEAPON_HAVE hammer{};
    hammer.item_no = ITEM_WEAPON_MAGICAL_HAMMER;
    EXPECT_TRUE(expanded::ChannelElements(hammer));
    hammer.item_no = ITEM_WEAPON_BIG_BUCKS_HAMMER;
    EXPECT_FALSE(expanded::ChannelElements(hammer));
}

TEST(ChannelElements, SuperSteveWithItsSphereChannels) {
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_MAGICAL_HAMMER);
    EXPECT_TRUE(expanded::ChannelElements(steve.weapon()));
    EXPECT_EQ(expanded::AttackElementFlags(WEAPON_ELEMENT_NONE), expanded::kChannelFlags);
    EXPECT_TRUE(expanded::ChannelChosen(steve.weapon(), WEAPON_ELEMENT_HOLY));

    expanded::test::Wielding other(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BIG_BUCKS_HAMMER);
    EXPECT_FALSE(expanded::ChannelElements(other.weapon()));
}

TEST(ChannelElements, ItsHitsCarryAllFiveElements) {
    expanded::test::Wielding hammer(ITEM_WEAPON_MAGICAL_HAMMER);
    EXPECT_EQ(expanded::AttackElementFlags(WEAPON_ELEMENT_NONE), expanded::kChannelFlags);
    EXPECT_TRUE(expanded::ChannelHit(expanded::AttackElementFlags(WEAPON_ELEMENT_NONE)));

    expanded::test::Wielding another(ITEM_WEAPON_BIG_BUCKS_HAMMER);
    EXPECT_EQ(expanded::AttackElementFlags(WEAPON_ELEMENT_FIRE), GetWeaponElementAttr(WEAPON_ELEMENT_FIRE));
    EXPECT_FALSE(expanded::ChannelHit(GetWeaponElementAttr(WEAPON_ELEMENT_FIRE)));
}

TEST(ChannelElements, EachElementsMagicPartIsAdded) {
    // 100 after defense, Magic 30, 20 points in each: a magic part of 100 x (0.15 + 0.08) = 23 each.
    const s8  elem[5] = {20, 20, 20, 20, 20};
    const s16 neutral[5] = {100, 100, 100, 100, 100};
    EXPECT_NEAR(expanded::ChannelDamage(100.0f, 30, elem, neutral), 100.0f + 5 * 23.0f, 0.01f);
}

TEST(ChannelElements, AnImmuneElementAddsNothingAndTheWeakestResistanceScalesTheRest) {
    const s8  elem[5] = {20, 20, 20, 20, 20};
    const s16 resist[5] = {0, 150, 100, 100, 50}; // immune to fire, weak to ice
    // 100 x 1.5 (its weakest), plus 23 x (1.5 + 1 + 1 + 0.5).
    EXPECT_NEAR(expanded::ChannelDamage(100.0f, 30, elem, resist), 150.0f + 23.0f * 4.0f, 0.01f);
}

TEST(ChannelElements, ImmuneToAllTakesNothing) {
    const s8  elem[5] = {20, 20, 20, 20, 20};
    const s16 immune[5] = {0, 0, 0, 0, 0};
    EXPECT_FLOAT_EQ(expanded::ChannelDamage(100.0f, 30, elem, immune), 0.0f);
}

TEST(ChannelElements, AnElementWithNoPointsStillAddsItsMagic) {
    const s16 neutral[5] = {100, 100, 100, 100, 100};
    EXPECT_NEAR(expanded::ChannelDamage(100.0f, 20, kNoPoints, neutral), 100.0f + 5 * 10.0f, 0.01f);
}

TEST(ChannelElements, TheStrongestElementIsTheOneWithMostPoints) {
    WEAPON_HAVE hammer{};
    hammer.item_no = ITEM_WEAPON_MAGICAL_HAMMER;
    hammer.elem[WEAPON_ELEMENT_THUNDER] = 12;
    hammer.elem[WEAPON_ELEMENT_HOLY] = 12;
    EXPECT_EQ(expanded::ChannelStrongest(hammer), WEAPON_ELEMENT_THUNDER); // the first of a tie
}

TEST(ChannelElements, TheMenuShowsAllChosenAndRefusesAChange) {
    WEAPON_HAVE hammer{};
    hammer.item_no = ITEM_WEAPON_MAGICAL_HAMMER;
    for (int e = 0; e < 5; e++) {
        EXPECT_TRUE(expanded::ChannelChosen(hammer, e));
    }
    EXPECT_FALSE(expanded::ChannelChosen(hammer, WEAPON_ELEMENT_NONE));
    EXPECT_TRUE(expanded::ChannelRefused(hammer));
}

TEST(ChannelElements, NoOneElementIsHeldChosen) {
    TestFloor    floor;
    CUserStatus &status = floor.status();
    WEAPON_HAVE &hammer = status.chara_weapons[CHARA_GORO][0];
    hammer = {};
    hammer.item_no = ITEM_WEAPON_MAGICAL_HAMMER;
    hammer.best_elem = WEAPON_ELEMENT_WIND;
    WEAPON_HAVE &other = status.chara_weapons[CHARA_GORO][1];
    other = {};
    other.item_no = ITEM_WEAPON_BIG_BUCKS_HAMMER;
    other.best_elem = WEAPON_ELEMENT_WIND;
    WEAPON_HAVE &steve = status.chara_weapons[CHARA_XIAO][0];
    steve = {};
    steve.item_no = ITEM_WEAPON_SUPER_STEVE;
    steve.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    steve.attach[0].sphere_weapon_no = ITEM_WEAPON_MAGICAL_HAMMER;
    steve.best_elem = WEAPON_ELEMENT_COLD;
    expanded::HoldChannelElements(status);
    EXPECT_EQ(hammer.best_elem, WEAPON_ELEMENT_NONE);
    EXPECT_EQ(steve.best_elem, WEAPON_ELEMENT_NONE);
    EXPECT_EQ(other.best_elem, WEAPON_ELEMENT_WIND);
}

TEST(ChannelElements, ItsBuildUpHasNoElementChosen) {
    WEAPON_HAVE built{};
    built.item_no = ITEM_WEAPON_GAIA_HAMMER;
    built.best_elem = WEAPON_ELEMENT_FIRE;
    expanded::ChannelElementsBuiltUp(built, ITEM_WEAPON_MAGICAL_HAMMER);
    EXPECT_EQ(built.best_elem, WEAPON_ELEMENT_NONE);

    built.best_elem = WEAPON_ELEMENT_FIRE;
    expanded::ChannelElementsBuiltUp(built, ITEM_WEAPON_BIG_BUCKS_HAMMER);
    EXPECT_EQ(built.best_elem, WEAPON_ELEMENT_FIRE);
}
