#include <gtest/gtest.h>

#include "expanded/weapons/abilities.hpp"
#include "itemdata.hpp"

TEST(WeaponAbilities, FixedAbilitiesAddToTheTableFlags) {
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_MACHO_SWORD, 0, 99), 0x2000);
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_EVILCISE, 0x0200, 99), 0x0204);
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_DE_SANGA, 0, 0), 0x0400);
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_ANGE_GEAR, 0, 99), 0x0800);
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_DAGGER, 0x0008, 0), 0x0008);
}

TEST(WeaponAbilities, ChanceAbilitiesFollowTheRoll) {
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_HARDSHOOTER, 0, 49), 0x0100);
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_HARDSHOOTER, 0, 50), 0);
    EXPECT_EQ(expanded::WeaponAbilities(ITEM_WEAPON_BONE_SLINGSHOT, 0, 0), 0x0100);
}

TEST(WeaponAbilities, RollsGiveFragileToAboutHalf) {
    int fragile = 0;

    for (int i = 0; i < 1000; i++) {
        fragile += (expanded::WeaponAbilities(ITEM_WEAPON_BONE_SLINGSHOT, 0) & 0x0100) != 0;
    }

    EXPECT_GT(fragile, 400);
    EXPECT_LT(fragile, 600);
}
