#include <gtest/gtest.h>

#include <vector>

#include "expanded/weapons/buildups.hpp"
#include "itemdata.hpp"

namespace {

// The weapons a weapon builds up into, from its owner's default weapon to the end of the chain.
std::vector<int> Into(int weapon_no, int default_weapon_no, int last_weapon_no) {
    const WEAPON_DATA *data = GetWeaponData(weapon_no);
    std::vector<int>   into;

    for (int position = 2; position <= last_weapon_no - default_weapon_no; position++) {
        if (expanded::BuildsUpInto(weapon_no, data, position)) {
            into.push_back(default_weapon_no + position);
        }
    }

    return into;
}

} // namespace

TEST(WeaponBuildUps, ChangedPathsReplaceRetails) {
    EXPECT_EQ(Into(ITEM_WEAPON_CHOORA, ITEM_WEAPON_DAGGER_BROKEN, ITEM_WEAPON_CHRONICLE2), std::vector<int>{ITEM_WEAPON_MANEATER});
    EXPECT_EQ(Into(ITEM_WEAPON_HARDSHOOTER, ITEM_WEAPON_WOODENSLINGSHOT_BROKEN, ITEM_WEAPON_ANGE_GEAR), std::vector<int>{ITEM_WEAPON_DOUBLE_IMPACT});
    EXPECT_EQ(Into(ITEM_WEAPON_PLATINUM_RING, ITEM_WEAPON_GOLD_RING_BROKEN, ITEM_WEAPON_SECRET_ARMLET),
              (std::vector<int>{ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_THORN_ARMLET}));
    EXPECT_EQ(Into(ITEM_WEAPON_POCKLEKUL, ITEM_WEAPON_GOLD_RING_BROKEN, ITEM_WEAPON_SECRET_ARMLET),
              (std::vector<int>{ITEM_WEAPON_GODDESS_RING, ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_SATAN_S_RING}));
    EXPECT_EQ(Into(ITEM_WEAPON_JACKAL, ITEM_WEAPON_MACHINE_GUN_BROKEN, ITEM_WEAPON_SWALLOW), std::vector<int>{ITEM_WEAPON_SWALLOW});
}

TEST(WeaponBuildUps, FinalFormsBuildUpIntoNothing) {
    EXPECT_FALSE(expanded::BuildsUp(ITEM_WEAPON_HEAVEN_S_CLOUD, GetWeaponData(ITEM_WEAPON_HEAVEN_S_CLOUD)));
    EXPECT_FALSE(expanded::BuildsUp(ITEM_WEAPON_FROZEN_TUNA, GetWeaponData(ITEM_WEAPON_FROZEN_TUNA)));
    EXPECT_FALSE(expanded::BuildsUp(ITEM_WEAPON_SKUNK, GetWeaponData(ITEM_WEAPON_SKUNK)));
    EXPECT_TRUE(Into(ITEM_WEAPON_HEAVEN_S_CLOUD, ITEM_WEAPON_DAGGER_BROKEN, ITEM_WEAPON_CHRONICLE2).empty());
}

TEST(WeaponBuildUps, OtherWeaponsKeepRetails) {
    EXPECT_TRUE(expanded::BuildsUp(ITEM_WEAPON_AGA_S_SWORD, GetWeaponData(ITEM_WEAPON_AGA_S_SWORD)));
    EXPECT_EQ(Into(ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DAGGER_BROKEN, ITEM_WEAPON_CHRONICLE2), std::vector<int>{ITEM_WEAPON_ATLAMILLIA_SWORD});
    EXPECT_EQ(Into(ITEM_WEAPON_BANDIT_S_RING, ITEM_WEAPON_GOLD_RING_BROKEN, ITEM_WEAPON_SECRET_ARMLET),
              (std::vector<int>{ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_POCKLEKUL}));
}
