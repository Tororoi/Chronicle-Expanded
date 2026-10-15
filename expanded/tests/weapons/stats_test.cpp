#include <gtest/gtest.h>

#include <cstring>

#include "expanded/weapons/stats.hpp"
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

    const WEAPON_DATA &branch = Weapon(ITEM_WEAPON_7BRANCH_SWORD);
    EXPECT_EQ(branch.durability, 47);
    EXPECT_EQ(branch.endurance, 47);
    EXPECT_EQ(branch.attack, 67);
    EXPECT_EQ(branch.speed, 77);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(branch.elem[i], 17) << i;
    }
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(branch.vs_monster[i], 7) << i;
    }

    EXPECT_EQ(Weapon(ITEM_WEAPON_CHRONICLE2).attack_max, 999);
    EXPECT_EQ(Weapon(ITEM_WEAPON_POCKLEKUL).attack, 28);
    EXPECT_EQ(Weapon(ITEM_WEAPON_POCKLEKUL).elem[4], Retail(ITEM_WEAPON_POCKLEKUL).elem[4]);
    EXPECT_EQ(Weapon(ITEM_WEAPON_FROZEN_TUNA).durability, Retail(ITEM_WEAPON_FROZEN_TUNA).durability);
    EXPECT_EQ(Weapon(ITEM_WEAPON_BLESSING_GUN).attack_max, 87);
    EXPECT_EQ(Weapon(ITEM_WEAPON_SKUNK).magic_max, 105);
    EXPECT_EQ(Weapon(ITEM_WEAPON_SKUNK).attack, Retail(ITEM_WEAPON_SKUNK).attack);
    EXPECT_EQ(Weapon(ITEM_WEAPON_PLATINUM_RING).elem[4], 20);
    EXPECT_EQ(Weapon(ITEM_WEAPON_PLATINUM_RING).elem[1], 0);
    EXPECT_EQ(Weapon(ITEM_WEAPON_SAND_BREAKER).durability, Retail(ITEM_WEAPON_SAND_BREAKER).durability);
}

TEST(WeaponStats, LeavesTheOtherWeaponsRetail) {
    std::memcpy(list, WeaponList, sizeof(list));
    expanded::WeaponStatsApply(list);

    for (int weapon : {ITEM_WEAPON_KITCHEN_KNIFE, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_GOLD_RING, ITEM_WEAPON_FIGHTING_STICK,
                       ITEM_WEAPON_MACHINE_GUN, ITEM_WEAPON_SWALLOW}) {
        EXPECT_EQ(std::memcmp(&Weapon(weapon), &Retail(weapon), sizeof(WEAPON_DATA)), 0) << weapon;
    }
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
