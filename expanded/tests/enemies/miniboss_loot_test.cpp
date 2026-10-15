#include <gtest/gtest.h>

#include <deque>
#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/enemies/miniboss.hpp"
#include "expanded/enemies/miniboss_loot.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace {

constexpr int kMasterJacket = 1;
constexpr int kHellPockle = 62;
constexpr int kNoTables = 999;

std::deque<int> g_rolls;

// The scripted rolls in turn, then 0.
int ScriptedRoll(int n) {
    if (g_rolls.empty()) {
        return 0;
    }
    int roll = g_rolls.front();
    g_rolls.pop_front();
    EXPECT_LT(roll, n);
    return roll;
}

class MinibossLoot : public ::testing::Test {
protected:
    void SetUp() override { expanded::SetMinibossLootRoll(&ScriptedRoll); }

    void TearDown() override {
        expanded::SetMinibossLootRoll(nullptr);
        g_rolls.clear();
    }

    expanded::MinibossDrop Roll(int dungeon, int floor, int name_no, std::initializer_list<int> rolls) {
        g_rolls = rolls;
        return expanded::RollMinibossDrop(dungeon, floor, name_no);
    }
};

} // namespace

TEST_F(MinibossLoot, RareDropFivePercentWithItsBoost) {
    expanded::MinibossDrop drop = Roll(DUNGEON_DIVINE_BEAST_CAVE, 0, kMasterJacket, {4, 0});
    EXPECT_EQ(drop.item, ITEM_WEAPON_GLADIUS);
    ASSERT_NE(drop.boost, nullptr);
    EXPECT_EQ(drop.boost->attack, 15);
    EXPECT_EQ(drop.boost->elem[4], 50);
    EXPECT_EQ(drop.boost->anti[MONSTER_FAMILY_UNDEAD], 50);
}

TEST_F(MinibossLoot, CommonDropThirtyPercentWithItsOwnBoosts) {
    expanded::MinibossDrop drop = Roll(DUNGEON_DIVINE_BEAST_CAVE, 0, kMasterJacket, {5, 29, 0});
    EXPECT_EQ(drop.item, ITEM_WEAPON_GLADIUS);
    EXPECT_EQ(drop.boost, nullptr);

    drop = Roll(DUNGEON_MOON_SEA, 0, kHellPockle, {5, 0, 0});
    EXPECT_EQ(drop.item, ITEM_WEAPON_POCKLEKUL);
    ASSERT_NE(drop.boost, nullptr);
    EXPECT_EQ(drop.boost->abilities_low, 0x80);
}

TEST_F(MinibossLoot, OtherwiseKeyWeaponAttachmentOrItem) {
    EXPECT_EQ(Roll(DUNGEON_DIVINE_BEAST_CAVE, 0, kMasterJacket, {5, 30, 34}).item, ITEM_TRAM_OIL);
    EXPECT_EQ(Roll(DUNGEON_DEMON_SHAFT, 0, kNoTables, {34}).item, ITEM_CRYSTAL_EYEBALL);
    EXPECT_EQ(Roll(DUNGEON_DIVINE_BEAST_CAVE, 8, kNoTables, {35, 14, 3}).item, ITEM_WEAPON_BONE_RAPIER);
    EXPECT_EQ(Roll(DUNGEON_DIVINE_BEAST_CAVE, 9, kNoTables, {35, 14, 6}).item, ITEM_WEAPON_BONE_SLINGSHOT);
    EXPECT_EQ(Roll(DUNGEON_GALLERY_OF_TIME, 20, kNoTables, {35, 14, 13}).item, ITEM_WEAPON_SWALLOW);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, kNoTables, {35, 15, 79, 59, 11}).item, ITEM_ATTACH_TURQUOISE);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, kNoTables, {35, 15, 79, 60, 18}).item, ITEM_ATTACH_MAGE_SLAYER);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, kNoTables, {35, 15, 80, 59, 1}).item, ITEM_AUTO_REPAIR_POWDER);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, kNoTables, {35, 15, 80, 60, 1}).item, ITEM_DRAN_S_FEATHER);
    EXPECT_EQ(Roll(-1, 0, kMasterJacket, {}).item, -1);
}

TEST_F(MinibossLoot, BoostWritesItsStatsAndAbilityBytes) {
    WEAPON_HAVE weapon = {};
    weapon.attack = 20;
    weapon.durability = 30;
    weapon.durability_f = 12.0f;
    weapon.elem[0] = 5;
    weapon.best_elem = 0;
    weapon.flags = 0x0208;

    expanded::MinibossDrop drop = Roll(DUNGEON_MOON_SEA, 0, 72, {0, 0});
    ASSERT_EQ(drop.item, ITEM_WEAPON_FROZEN_TUNA);
    ASSERT_NE(drop.boost, nullptr);
    expanded::BoostWeapon(weapon, *drop.boost);

    EXPECT_EQ(weapon.attack, 20);
    EXPECT_EQ(weapon.endurance, 99);
    EXPECT_EQ(weapon.durability, 99);
    EXPECT_FLOAT_EQ(weapon.durability_f, 99.0f);
    EXPECT_EQ(weapon.elem[0], 5);
    EXPECT_EQ(weapon.elem[1], 99);
    EXPECT_EQ(weapon.best_elem, 1);
    EXPECT_EQ(weapon.vs_monster[MONSTER_FAMILY_SEA], 99);
    EXPECT_EQ(weapon.flags, 0x0102);
}

TEST_F(MinibossLoot, AMinibossCarriesItsDrop) {
    selectMapNo = DUNGEON_DIVINE_BEAST_CAVE;
    auto unit = std::make_unique<CMonstorUnit>();
    unit->model[0] = MonstorTable[0];
    expanded::EnemyModelSetUp(*unit, 0, 0);
    MONSTOR &jacket = unit->monster[0];
    jacket.state = 2;
    jacket.base_model = 0;
    jacket.name_no = unit->model[0].name_no;
    jacket.drop_item = -1;
    expanded::EnemySpawned(*unit, 0);

    g_rolls = {0, 0};
    expanded::MakeMiniboss(*unit, 0);
    EXPECT_EQ(jacket.drop_item, ITEM_WEAPON_GLADIUS);
    selectMapNo = -1;
}
