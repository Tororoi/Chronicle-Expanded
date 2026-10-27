#include "dungeon.h"

#include <gtest/gtest.h>

#include <set>

#include "expanded/core/random.hpp"
#include "expanded/enemies/atla_gemron_loot.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

constexpr int kRolls = 4000;

// The share of a dungeon's drops that pass a test, from a seeded roll.
template <class Test>
double Share(int dungeon, Test test) {
    int passed = 0;
    for (int i = 0; i < kRolls; i++) {
        passed += test(expanded::RollAtlaGemronDrop(dungeon)) ? 1 : 0;
    }
    return passed / static_cast<double>(kRolls);
}

bool PlainSword(const expanded::AtlaDrop &drop) {
    return drop.weapon == ITEM_WEAPON_ATLAMILLIA_SWORD && !drop.sphere;
}

bool SwordSphere(const expanded::AtlaDrop &drop) {
    return drop.weapon == ITEM_WEAPON_ATLAMILLIA_SWORD && drop.sphere;
}

// A party of everyone, the drops rolled from a fixed seed.
class AtlaGemronLoot : public ::testing::Test {
protected:
    void SetUp() override {
        expanded::SeedModRandom(7);
        floor_.status().party_size = 6;
    }

    expanded::test::TestFloor floor_;
};

} // namespace

TEST_F(AtlaGemronLoot, TheSwordFromSunAndMoonTempleOnItsSphereAnywhere) {
    for (int dungeon = 0; dungeon < DUNGEON_COUNT; dungeon++) {
        double plain = Share(dungeon, PlainSword);
        if (dungeon < DUNGEON_SUN_MOON_TEMPLE) {
            EXPECT_EQ(plain, 0.0) << dungeon;
        } else {
            EXPECT_NEAR(plain, 0.05, 0.015) << dungeon;
        }
        EXPECT_NEAR(Share(dungeon, SwordSphere), 0.15, 0.02) << dungeon;
        // Everything but the plain sword is a SynthSphere.
        EXPECT_EQ(Share(dungeon, [](const expanded::AtlaDrop &drop) { return !drop.sphere && !PlainSword(drop); }), 0.0) << dungeon;
    }
}

TEST_F(AtlaGemronLoot, TheFeaturedWeaponsSphereButInDemonShaft) {
    auto featured = [](int weapon) {
        return [weapon](const expanded::AtlaDrop &drop) { return drop.weapon == weapon && drop.sphere; };
    };
    // Its own 10 %, and the tier draws that land on it.
    EXPECT_GE(Share(DUNGEON_DIVINE_BEAST_CAVE, featured(ITEM_WEAPON_BONE_RAPIER)), 0.09);
    EXPECT_GE(Share(DUNGEON_MOON_SEA, featured(ITEM_WEAPON_7BRANCH_SWORD)), 0.09);
    EXPECT_LT(Share(DUNGEON_DEMON_SHAFT, featured(ITEM_WEAPON_DARK_CLOUD)), 0.05);
}

TEST_F(AtlaGemronLoot, AWeaponIsOfTheDungeonsTiersAndOfAMemberWhoHasJoined) {
    // Divine Beast Cave draws its own tier only.
    std::set<int> cave;
    for (int i = 0; i < kRolls; i++) {
        cave.insert(expanded::RollAtlaGemronDrop(DUNGEON_DIVINE_BEAST_CAVE).weapon);
    }
    EXPECT_FALSE(cave.contains(ITEM_WEAPON_BUSTER_SWORD));
    EXPECT_TRUE(cave.contains(ITEM_WEAPON_DAGGER));

    // A later dungeon draws the tiers below it too.
    std::set<int> forest;
    for (int i = 0; i < kRolls; i++) {
        forest.insert(expanded::RollAtlaGemronDrop(DUNGEON_WISE_OWL_FOREST).weapon);
    }
    EXPECT_TRUE(forest.contains(ITEM_WEAPON_BUSTER_SWORD));
    EXPECT_TRUE(forest.contains(ITEM_WEAPON_DAGGER));

    // With Toan alone, only his weapons and the Atlamillia's.
    floor_.status().party_size = 1;
    for (int i = 0; i < kRolls; i++) {
        expanded::AtlaDrop drop = expanded::RollAtlaGemronDrop(DUNGEON_DEMON_SHAFT);
        const WEAPON_DATA *data = GetWeaponData(drop.weapon);
        ASSERT_NE(data, nullptr) << drop.weapon;
        EXPECT_EQ(data->owner, 0) << drop.weapon;
    }
}

TEST_F(AtlaGemronLoot, DungeonsPastTheSevenAreTheNearestOne) {
    EXPECT_EQ(Share(-3, PlainSword), 0.0);
    EXPECT_NEAR(Share(DUNGEON_COUNT + 2, PlainSword), 0.05, 0.015);
}
