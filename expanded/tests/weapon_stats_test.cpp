#include <gtest/gtest.h>

#include <cstring>

#include "expanded/weapon_stats.hpp"
#include "itemdata.hpp"

extern WEAPON_DATA WeaponList[120];

namespace {

WEAPON_DATA list[120];

WEAPON_DATA &Weapon(int weapon_no) {
    return list[weapon_no - ITEM_WEAPON_START];
}

const WEAPON_DATA &Retail(int weapon_no) {
    return WeaponList[weapon_no - ITEM_WEAPON_START];
}

} // namespace

TEST(WeaponStats, SetsListedStatsAndKeepsTheRest) {
    std::memcpy(list, WeaponList, sizeof(list));
    expanded::WeaponStatsApply(list);

    const WEAPON_DATA &knife = Weapon(ITEM_WEAPON_KITCHEN_KNIFE);
    EXPECT_EQ(knife.durability, 50);
    EXPECT_EQ(knife.attack, 25);
    EXPECT_EQ(knife.endurance, 30);
    EXPECT_EQ(knife.speed, Retail(ITEM_WEAPON_KITCHEN_KNIFE).speed);
    EXPECT_EQ(knife.elem[1], 0);
    EXPECT_EQ(knife.elem[2], 8);
    EXPECT_EQ(knife.vs_monster[2], 90);

    EXPECT_EQ(Weapon(ITEM_WEAPON_CHRONICLE2).attack_max, 999);
    EXPECT_EQ(Weapon(ITEM_WEAPON_DAGGER).attack, Retail(ITEM_WEAPON_DAGGER).attack);
}

TEST(WeaponStats, AddsTheCharacterBonusesBeforeTheWeaponStats) {
    std::memcpy(list, WeaponList, sizeof(list));
    expanded::WeaponStatsApply(list);

    const WEAPON_DATA &stick = Weapon(ITEM_WEAPON_FIGHTING_STICK);
    EXPECT_EQ(stick.attack, Retail(ITEM_WEAPON_FIGHTING_STICK).attack + 10);
    EXPECT_EQ(stick.attack_max, Retail(ITEM_WEAPON_FIGHTING_STICK).attack_max + 10);
    EXPECT_EQ(stick.endurance, Retail(ITEM_WEAPON_FIGHTING_STICK).endurance + 15);
    EXPECT_EQ(Weapon(ITEM_WEAPON_BABEL_S_SPEAR).attack, Retail(ITEM_WEAPON_BABEL_S_SPEAR).attack);
    EXPECT_EQ(Weapon(ITEM_WEAPON_FIGHTING_STICK_BROKEN).attack, Retail(ITEM_WEAPON_FIGHTING_STICK_BROKEN).attack);

    EXPECT_EQ(Weapon(ITEM_WEAPON_SWALLOW).attack, Retail(ITEM_WEAPON_SWALLOW).attack + 15);
    EXPECT_EQ(Weapon(ITEM_WEAPON_SWALLOW).endurance, Retail(ITEM_WEAPON_SWALLOW).endurance);
    EXPECT_EQ(Weapon(ITEM_WEAPON_BLESSING_GUN).attack, Retail(ITEM_WEAPON_BLESSING_GUN).attack + 15);
    EXPECT_EQ(Weapon(ITEM_WEAPON_BLESSING_GUN).attack_max, 87);
}

TEST(WeaponStats, OpensSlotsUpToTheCount) {
    std::memcpy(list, WeaponList, sizeof(list));
    expanded::WeaponStatsApply(list);

    const s8 three[6] = {2, 1, 1, 0, 0, 0};
    const s8 four[6] = {2, 1, 1, 1, 0, 0};
    EXPECT_EQ(std::memcmp(Weapon(ITEM_WEAPON_HEAVEN_S_CLOUD).hole, three, 6), 0);
    EXPECT_EQ(std::memcmp(Weapon(ITEM_WEAPON_FROZEN_TUNA).hole, three, 6), 0);
    EXPECT_EQ(std::memcmp(Weapon(ITEM_WEAPON_BABEL_S_SPEAR).hole, four, 6), 0);
}
