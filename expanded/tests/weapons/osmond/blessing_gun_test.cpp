#include <gtest/gtest.h>

#include <cstring>
#include <initializer_list>

#include "expanded/weapons/attach_boost.hpp"
#include "expanded/weapons/osmond/blessing_gun.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/buster_sword.hpp"
#include "itemdata.hpp"
#include "menu_inventory.hpp"
#include "stockitem.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestSave;

} // namespace

TEST(BlessedBait, WithoutTheGunTheRollTakesTheBait) {
    TestSave save;
    save.status().chara_weapons[CHARA_OSMOND][0].item_no = ITEM_WEAPON_SKUNK;
    EXPECT_FALSE(expanded::BlessedBaitKept());
}

TEST(BlessedBait, OwnedTheBaitStaysOnTheHook) {
    {
        TestSave save;
        save.status().chara_weapons[CHARA_OSMOND][4].item_no = ITEM_WEAPON_BLESSING_GUN;
        EXPECT_TRUE(expanded::BlessedBaitKept());
    }
    {
        TestSave save;
        save.stock().weapons[12].item_no = ITEM_WEAPON_BLESSING_GUN;
        EXPECT_TRUE(expanded::BlessedBaitKept());
    }
}

TEST(BlessedBait, OffWithTheWeaponAbilities) {
    TestSave    save;
    OptionScope options;
    save.status().chara_weapons[CHARA_OSMOND][0].item_no = ITEM_WEAPON_BLESSING_GUN;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_FALSE(expanded::BlessedBaitKept());
}

namespace {

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

ATTACH_LIST Sphere(int weapon_no, std::initializer_list<int> elements) {
    ATTACH_LIST sphere = {};
    sphere.item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    sphere.sphere_weapon_no = static_cast<s16>(weapon_no);
    int i = 0;
    for (int element : elements) {
        sphere.elem[i++] = static_cast<s8>(element);
    }
    return sphere;
}

} // namespace

TEST(BlessingBoost, AnElementAttachmentIsWorthOneMoreOnIt) {
    WEAPON_HAVE gun = Weapon(ITEM_WEAPON_BLESSING_GUN);
    for (int item = ITEM_ATTACH_FIRE; item <= ITEM_ATTACH_HOLY; item++) {
        int         element = item - ITEM_ATTACH_FIRE;
        ATTACH_LIST entry = Entry(item);
        int         own = entry.elem[element];
        ASSERT_GT(own, 0) << item;
        expanded::AttachBoostPlaced(&gun, entry);
        EXPECT_EQ(entry.elem[element], own + 1) << item;
        expanded::AttachBoostPlaced(&gun, entry); // placed again: still one more
        EXPECT_EQ(entry.elem[element], own + 1) << item;
        expanded::AttachBoostPlaced(nullptr, entry); // off the gun
        EXPECT_EQ(entry.elem[element], own) << item;
    }
}

TEST(BlessingBoost, EveryElementAGemRaisesIsWorthOneMore) {
    WEAPON_HAVE gun = Weapon(ITEM_WEAPON_BLESSING_GUN);
    for (int item : {ITEM_ATTACH_GARNET, ITEM_ATTACH_SUN}) {
        ATTACH_LIST gem = Entry(item);
        expanded::AttachBoostPlaced(&gun, gem);
        for (int element = 0; element < 5; element++) {
            int own = GetAttachData(item)->elem[element];
            EXPECT_EQ(gem.elem[element], own > 0 ? own + 1 : own) << item << " " << element;
        }
    }
}

TEST(BlessingBoost, OtherWeaponsAndStatsKeepTheirValues) {
    WEAPON_HAVE dagger = Weapon(ITEM_WEAPON_DAGGER);
    ATTACH_LIST fire = Entry(ITEM_ATTACH_FIRE);
    ATTACH_LIST before = fire;
    expanded::AttachBoostPlaced(&dagger, fire);
    EXPECT_EQ(std::memcmp(&fire, &before, sizeof(fire)), 0);

    // On the gun an anti attachment is as it was.
    WEAPON_HAVE gun = Weapon(ITEM_WEAPON_BLESSING_GUN);
    ATTACH_LIST anti = Entry(ITEM_ATTACH_DINOSLAYER);
    before = anti;
    expanded::AttachBoostPlaced(&gun, anti);
    EXPECT_EQ(std::memcmp(&anti, &before, sizeof(anti)), 0);
}

TEST(BlessingBoost, ASwapBoostsThePlacedAndRestoresTheTaken) {
    WEAPON_HAVE gun = Weapon(ITEM_WEAPON_BLESSING_GUN);
    ATTACH_LIST placed = Entry(ITEM_ATTACH_ICE), taken = Entry(ITEM_ATTACH_FIRE);
    expanded::AttachBoostPlaced(&gun, taken);
    expanded::AttachBoostSwapped(gun, placed, taken);
    EXPECT_EQ(placed.elem[1], GetAttachData(ITEM_ATTACH_ICE)->elem[1] + 1);
    EXPECT_EQ(taken.elem[0], GetAttachData(ITEM_ATTACH_FIRE)->elem[0]);
}

TEST(BlessingBoost, ASpheresElementsGainThePointOnce) {
    ATTACH_LIST sphere = Sphere(ITEM_WEAPON_DAGGER, {5, 0, 2});
    expanded::BlessingBoostEntry(sphere, true);
    expanded::BlessingBoostEntry(sphere, true);
    EXPECT_EQ(sphere.elem[0], 6);
    EXPECT_EQ(sphere.elem[1], 0);
    EXPECT_EQ(sphere.elem[2], 3);
    expanded::BlessingBoostEntry(sphere, false);
    EXPECT_EQ(sphere.elem[0], 5);
    EXPECT_EQ(sphere.elem[2], 2);
    EXPECT_EQ(sphere.unk_1F, 0);
}

TEST(BlessingBoost, ASphereCanHoldBothBoosts) {
    ATTACH_LIST sphere = Sphere(ITEM_WEAPON_DAGGER, {5});
    sphere.vs_monster[0] = 4;
    expanded::BusterBoostEntry(sphere, true);
    expanded::BlessingBoostEntry(sphere, true);
    EXPECT_EQ(sphere.elem[0], 6);
    EXPECT_EQ(sphere.vs_monster[0], 5);
    expanded::BusterBoostEntry(sphere, false);
    EXPECT_EQ(sphere.elem[0], 6);
    EXPECT_EQ(sphere.vs_monster[0], 4);
    expanded::BlessingBoostEntry(sphere, false);
    EXPECT_EQ(sphere.elem[0], 5);
    EXPECT_EQ(sphere.unk_1F, 0);
}

TEST(BlessingBoost, SuperSteveInheritsTheBoostWithItsSphere) {
    WEAPON_HAVE steve = Weapon(ITEM_WEAPON_SUPER_STEVE);
    steve.attach[0] = Sphere(ITEM_WEAPON_BLESSING_GUN, {4});
    steve.attach[1] = Entry(ITEM_ATTACH_FIRE);
    expanded::AttachBoostWeapon(steve);
    EXPECT_TRUE(expanded::BlessingBoosted(&steve));
    EXPECT_EQ(steve.attach[0].elem[0], 5);
    EXPECT_EQ(steve.attach[1].elem[0], GetAttachData(ITEM_ATTACH_FIRE)->elem[0] + 1);

    // Another weapon's sphere: no boost.
    steve.attach[0] = Sphere(ITEM_WEAPON_DAGGER, {4});
    expanded::AttachBoostWeapon(steve);
    EXPECT_FALSE(expanded::BlessingBoosted(&steve));
    EXPECT_EQ(steve.attach[0].elem[0], 4);
    EXPECT_EQ(steve.attach[1].elem[0], GetAttachData(ITEM_ATTACH_FIRE)->elem[0]);
}

TEST(BlessingBoost, OffWithTheWeaponAbilities) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    WEAPON_HAVE gun = Weapon(ITEM_WEAPON_BLESSING_GUN);
    ATTACH_LIST fire = Entry(ITEM_ATTACH_FIRE);
    int         own = fire.elem[0];
    expanded::AttachBoostPlaced(&gun, fire);
    EXPECT_EQ(fire.elem[0], own);
}
