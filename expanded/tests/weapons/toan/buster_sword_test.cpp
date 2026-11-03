#include <gtest/gtest.h>

#include <cstring>
#include <initializer_list>
#include <utility>

#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/buster_sword.hpp"
#include "itemdata.hpp"
#include "menu_inventory.hpp"
#include "test_support.hpp"
#include "weaponlevelup.hpp"

namespace {

// An attachment entry as the board holds one: the item's own data.
ATTACH_LIST Entry(int item_no) {
    ATTACH_LIST entry = {};
    std::memcpy(&entry, GetAttachData(item_no), sizeof(entry));
    return entry;
}

WEAPON_HAVE Weapon(int item_no) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = static_cast<s16>(item_no);
    return weapon;
}

} // namespace

TEST(BusterSword, AnAntiAttachmentIsWorthOneMoreOnIt) {
    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    for (int item = ITEM_ATTACH_DINOSLAYER; item <= ITEM_ATTACH_MAGE_SLAYER; item++) {
        int         anti = item - ITEM_ATTACH_DINOSLAYER;
        ATTACH_LIST entry = Entry(item);
        int         own = entry.vs_monster[anti];
        expanded::BusterBoostPlaced(&buster, entry);
        EXPECT_EQ(entry.vs_monster[anti], own + 1) << item;
        expanded::BusterBoostPlaced(&buster, entry); // placed again: still one more
        EXPECT_EQ(entry.vs_monster[anti], own + 1) << item;
        expanded::BusterBoostPlaced(nullptr, entry); // off the sword
        EXPECT_EQ(entry.vs_monster[anti], own) << item;
    }
}

TEST(BusterSword, OtherWeaponsAndAttachmentsKeepTheirValues) {
    WEAPON_HAVE dagger = Weapon(ITEM_WEAPON_DAGGER);
    ATTACH_LIST anti = Entry(ITEM_ATTACH_DINOSLAYER);
    int         own = anti.vs_monster[0];
    expanded::BusterBoostPlaced(&dagger, anti);
    EXPECT_EQ(anti.vs_monster[0], own);

    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    ATTACH_LIST fire = Entry(ITEM_ATTACH_FIRE);
    ATTACH_LIST before = fire;
    expanded::BusterBoostPlaced(&buster, fire);
    EXPECT_EQ(std::memcmp(&fire, &before, sizeof(fire)), 0);
}

TEST(BusterSword, ASwapBoostsThePlacedAndRestoresTheTaken) {
    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    ATTACH_LIST placed = Entry(ITEM_ATTACH_SKY_HUNTER);
    ATTACH_LIST taken = Entry(ITEM_ATTACH_MAGE_SLAYER);
    expanded::BusterBoostPlaced(&buster, taken);
    expanded::BusterBoostSwapped(buster, placed, taken);
    EXPECT_EQ(placed.vs_monster[ITEM_ATTACH_SKY_HUNTER - ITEM_ATTACH_DINOSLAYER],
              GetAttachData(ITEM_ATTACH_SKY_HUNTER)->vs_monster[ITEM_ATTACH_SKY_HUNTER - ITEM_ATTACH_DINOSLAYER] + 1);
    EXPECT_EQ(taken.vs_monster[9], GetAttachData(ITEM_ATTACH_MAGE_SLAYER)->vs_monster[9]);
}

namespace {

ATTACH_LIST Sphere(int weapon_no, std::initializer_list<int> anti) {
    ATTACH_LIST sphere = {};
    sphere.item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    sphere.sphere_weapon_no = static_cast<s16>(weapon_no);
    int i = 0;
    for (int value : anti) {
        sphere.vs_monster[i++] = static_cast<s8>(value);
    }
    return sphere;
}

// A Dagger (a sphere slot and a regular one) with the Buster Sword's sphere on, Fire in its own regular slot and Ice
// in the extra one.
WEAPON_HAVE FullDagger() {
    WEAPON_HAVE dagger = Weapon(ITEM_WEAPON_DAGGER);
    dagger.attach[0] = Sphere(ITEM_WEAPON_BUSTER_SWORD, {});
    dagger.attach[1] = Entry(ITEM_ATTACH_FIRE);
    dagger.attach[2] = Entry(ITEM_ATTACH_ICE);
    return dagger;
}

// The weapon menu taking slot hole off into the hand.
ATTACH_LIST TakeOff(WEAPON_HAVE &weapon, int hole) {
    ATTACH_LIST taken = weapon.attach[hole];
    weapon.attach[hole] = {};
    expanded::BusterSphereLeft(weapon, taken, hole);
    return taken;
}

} // namespace

TEST(BusterSword, EveryAntiStatAnAttachmentRaisesIsWorthOneMore) {
    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    ATTACH_LIST garnet = Entry(ITEM_ATTACH_GARNET), sun = Entry(ITEM_ATTACH_SUN);
    expanded::BusterBoostPlaced(&buster, garnet);
    expanded::BusterBoostPlaced(&buster, sun);
    for (int anti = 0; anti < 10; anti++) {
        int own = GetAttachData(ITEM_ATTACH_GARNET)->vs_monster[anti];
        EXPECT_EQ(garnet.vs_monster[anti], own > 0 ? own + 1 : own) << anti;
        EXPECT_EQ(sun.vs_monster[anti], GetAttachData(ITEM_ATTACH_SUN)->vs_monster[anti] + 1) << anti;
    }
}

TEST(BusterSword, ASpheresAntiStatsGainThePointOnce) {
    ATTACH_LIST sphere = Sphere(ITEM_WEAPON_DAGGER, {5, 0, 2});
    expanded::BusterBoostEntry(sphere, true);
    expanded::BusterBoostEntry(sphere, true);
    EXPECT_EQ(sphere.vs_monster[0], 6);
    EXPECT_EQ(sphere.vs_monster[1], 0);
    EXPECT_EQ(sphere.vs_monster[2], 3);
    expanded::BusterBoostEntry(sphere, false);
    EXPECT_EQ(sphere.vs_monster[0], 5);
    EXPECT_EQ(sphere.vs_monster[2], 2);
    EXPECT_EQ(sphere.unk_1F, 0);
}

TEST(BusterSword, SuperSteveInheritsTheBoostWithItsSphere) {
    WEAPON_HAVE steve = Weapon(ITEM_WEAPON_SUPER_STEVE);
    steve.attach[0] = Sphere(ITEM_WEAPON_BUSTER_SWORD, {4});
    steve.attach[1] = Entry(ITEM_ATTACH_DINOSLAYER);
    expanded::BusterBoostWeapon(steve);
    EXPECT_EQ(steve.attach[0].vs_monster[0], 5);
    EXPECT_EQ(steve.attach[1].vs_monster[0], GetAttachData(ITEM_ATTACH_DINOSLAYER)->vs_monster[0] + 1);

    // Another weapon's sphere: no boost.
    ATTACH_LIST buster_sphere = TakeOff(steve, 0);
    EXPECT_EQ(steve.attach[1].vs_monster[0], GetAttachData(ITEM_ATTACH_DINOSLAYER)->vs_monster[0]);
    expanded::BusterBoostEntry(buster_sphere, false);
    EXPECT_EQ(buster_sphere.vs_monster[0], 4);
    steve.attach[0] = Sphere(ITEM_WEAPON_DAGGER, {4});
    expanded::BusterBoostWeapon(steve);
    EXPECT_EQ(steve.attach[0].vs_monster[0], 4);
    EXPECT_FALSE(expanded::BusterBoosted(&steve));
}

TEST(BusterSword, ItsSphereOpensAnExtraRegularSlot) {
    WEAPON_HAVE dagger = Weapon(ITEM_WEAPON_DAGGER);
    int         own = GetWeaponHoleNum(ITEM_WEAPON_DAGGER);
    EXPECT_EQ(expanded::BusterHoles(dagger, own), own);
    dagger.attach[0] = Sphere(ITEM_WEAPON_BUSTER_SWORD, {});
    EXPECT_EQ(expanded::BusterHoles(dagger, own), own + 1);
    EXPECT_EQ(expanded::BusterHoleKind(dagger, own, 0), 1);
    EXPECT_EQ(expanded::BusterHoleKind(dagger, 0, 2), 2);
    expanded::BusterBoostWeapon(dagger);
    EXPECT_EQ(dagger.attach_kind[own], 0);

    dagger.attach[0] = Sphere(ITEM_WEAPON_DAGGER, {});
    EXPECT_EQ(expanded::BusterHoles(dagger, own), own);
}

TEST(BusterSword, OnSuperSteveTheExtraSlotDoubles) {
    WEAPON_HAVE steve = Weapon(ITEM_WEAPON_SUPER_STEVE);
    int         own = GetWeaponHoleNum(ITEM_WEAPON_SUPER_STEVE);
    steve.attach[0] = Sphere(ITEM_WEAPON_BUSTER_SWORD, {});
    expanded::BusterBoostWeapon(steve);
    EXPECT_EQ(steve.attach_kind[own], 3);
    TakeOff(steve, 0);
    EXPECT_EQ(steve.attach_kind[own], 0);
}

TEST(BusterSword, TakingTheSphereOffMovesTheExtraSlotsAttachment) {
    // Every slot full: into the slot the sphere left.
    WEAPON_HAVE dagger = FullDagger();
    TakeOff(dagger, 0);
    EXPECT_EQ(dagger.attach[0].item_no, ITEM_ATTACH_ICE);
    EXPECT_EQ(dagger.attach[2].item_no, 0);
    EXPECT_EQ(expanded::BusterHoles(dagger, 2), 2);

    // An empty regular slot: into that.
    dagger = FullDagger();
    dagger.attach[1] = {};
    TakeOff(dagger, 0);
    EXPECT_EQ(dagger.attach[1].item_no, ITEM_ATTACH_ICE);
    EXPECT_EQ(dagger.attach[0].item_no, 0);
}

TEST(BusterSword, ASwapThatLeavesTheExtraSlotsAttachmentNowhereIsRefused) {
    WEAPON_HAVE dagger = FullDagger();
    EXPECT_TRUE(expanded::BusterSwapRefused(dagger, 0, Entry(ITEM_ATTACH_THUNDER)));
    EXPECT_FALSE(expanded::BusterSwapRefused(dagger, 0, Sphere(ITEM_WEAPON_BUSTER_SWORD, {})));
    EXPECT_FALSE(expanded::BusterSwapRefused(dagger, 1, Entry(ITEM_ATTACH_THUNDER)));
    dagger.attach[1] = {};
    EXPECT_FALSE(expanded::BusterSwapRefused(dagger, 0, Entry(ITEM_ATTACH_THUNDER)));
}

TEST(BusterSword, PuttingTheSphereBackRestoresTheExtraSlot) {
    WEAPON_HAVE dagger = FullDagger();
    ATTACH_LIST held = TakeOff(dagger, 0);
    // The menu's cancel: the sphere back into its slot, what had moved there into the hand.
    std::swap(dagger.attach[0], held);
    short held_no = held.item_no;
    EXPECT_TRUE(expanded::BusterCancelled(dagger, held, held_no));
    EXPECT_EQ(held_no, 0);
    EXPECT_EQ(dagger.attach[0].sphere_weapon_no, ITEM_WEAPON_BUSTER_SWORD);
    EXPECT_EQ(dagger.attach[2].item_no, ITEM_ATTACH_ICE);
}

TEST(BusterSword, ALevelUpAbsorbsTheExtraSlot) {
    WEAPON_HAVE with = FullDagger(), without = FullDagger(), dst = {};
    without.attach[2] = {};
    with.level = without.level = 1;
    WeaponLevelUpValueCalc(&with, &dst, 1, 0);
    int ice_with = dst.elem[1];
    WeaponLevelUpValueCalc(&without, &dst, 1, 0);
    EXPECT_GT(ice_with, dst.elem[1]);
}

TEST(BusterSword, OffWithTheWeaponAbilities) {
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    ATTACH_LIST anti = Entry(ITEM_ATTACH_DINOSLAYER);
    int         own = anti.vs_monster[0];
    expanded::BusterBoostPlaced(&buster, anti);
    EXPECT_EQ(anti.vs_monster[0], own);
    EXPECT_FALSE(expanded::BusterBoosted(&buster));
}
