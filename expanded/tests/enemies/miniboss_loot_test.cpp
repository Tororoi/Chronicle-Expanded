#include <gtest/gtest.h>

#include <deque>
#include <iterator>
#include <memory>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/enemies/miniboss.hpp"
#include "expanded/enemies/miniboss_loot.hpp"
#include "expanded/enemies/monsters.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace {

using expanded::Monster;
using expanded::MinibossTier;

// The species the loot table covers.
constexpr Monster kLooted[] = {
    Monster::CAVE_BAT, Monster::DASHER, Monster::DRAGON, Monster::GHOST, Monster::MASTER_JACKET, Monster::OPAR,
    Monster::ROCKANOFF, Monster::SKELETON_SOLDIER, Monster::STATUE, Monster::STATUE_DOG, Monster::YAMMICH,
    Monster::CANNIBAL_PLANT, Monster::EARTH_DIGGER, Monster::FLI_FLI, Monster::FRIDAY, Monster::HALEY_HOLEY,
    Monster::HALLOWEEN, Monster::HORNET, Monster::KING_PRICKLY, Monster::MONDAY, Monster::SATURDAY, Monster::SUNDAY,
    Monster::THURSDAY, Monster::TUESDAY, Monster::WEDNESDAY, Monster::WEREWOLF, Monster::WITCH_ILLZA,
    Monster::AUNTIE_MEDU, Monster::CAPTAIN, Monster::CORCEA, Monster::CURSED_ROSE, Monster::GUNNY, Monster::GYON,
    Monster::MASK_OF_PRAJNA, Monster::PIRATES_CHARIOT, Monster::SAM, Monster::BLUE_DRAGON, Monster::BOMBER_HEAD,
    Monster::CRABBY_HERMIT, Monster::DUNE, Monster::GOL, Monster::GOLEM, Monster::MR_BLARE, Monster::MUMMY,
    Monster::PHANTOM, Monster::SIL, Monster::STEEL_GIANT, Monster::TITAN, Monster::ARTHUR, Monster::CRESCENT_BARON,
    Monster::HELL_POCKLE, Monster::MOON_BUG, Monster::MOON_DIGGER, Monster::SPACE_GYON, Monster::VULCAN,
    Monster::WHITE_FANG, Monster::WITCH_HELLZA, Monster::ALEXANDER, Monster::BILLY, Monster::BLACK_DRAGON,
    Monster::BLIZZARD, Monster::CLUB, Monster::CURSE_DANCER, Monster::DARK_FLOWER, Monster::DIAMOND,
    Monster::EVIL_BAT, Monster::HEART, Monster::JOKER, Monster::LICH, Monster::LIVING_ARMOR, Monster::RASH_DASHER,
    Monster::SPADE, Monster::ALEXANDER_ENHANCED, Monster::ARTHUR_ENHANCED, Monster::AUNTIE_MEDU_ENHANCED,
    Monster::BISHOP_Q, Monster::BOMBER_HEAD_ENHANCED, Monster::CAPTAIN_ENHANCED, Monster::CAVE_BAT_ENHANCED,
    Monster::CLUB_ENHANCED, Monster::CORCEA_ENHANCED, Monster::CRABBY_HERMIT_ENHANCED,
    Monster::CRESCENT_BARON_ENHANCED, Monster::CURSED_ROSE_ENHANCED, Monster::DIAMOND_ENHANCED,
    Monster::EVIL_BAT_ENHANCED, Monster::GACIOUS_ENHANCED, Monster::GEMRON_FIRE, Monster::GEMRON_HOLY,
    Monster::GEMRON_ICE, Monster::GEMRON_THUNDER, Monster::GEMRON_WIND, Monster::GOL_ENHANCED, Monster::GYON_ENHANCED,
    Monster::HALLOWEEN_ENHANCED, Monster::HEART_ENHANCED, Monster::HORN_HEAD, Monster::JOKER_ENHANCED,
    Monster::LICH_ENHANCED, Monster::LIVING_ARMOR_ENHANCED, Monster::MASK_OF_PRAJNA_ENHANCED,
    Monster::MASTER_JACKET_ENHANCED, Monster::MUMMY_ENHANCED, Monster::NIKAPOUS, Monster::PIRATES_CHARIOT_ENHANCED,
    Monster::RASH_DASHER_ENHANCED, Monster::ROCKANOFF_ENHANCED, Monster::SIL_ENHANCED, Monster::SILVER_GEAR,
    Monster::SPACE_GYON_ENHANCED, Monster::SPADE_ENHANCED, Monster::STATUE_DOG_ENHANCED,
    Monster::STEEL_GIANT_ENHANCED, Monster::TITAN_ENHANCED, Monster::VULCAN_ENHANCED, Monster::WHITE_FANG_ENHANCED,
    Monster::WITCH_HELLZA_ENHANCED, Monster::YAMMICH_ENHANCED,
};

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

    expanded::MinibossDrop Roll(int dungeon, int floor, Monster species, std::initializer_list<int> rolls) {
        g_rolls = rolls;
        return expanded::RollMinibossDrop(dungeon, floor, species);
    }
};

// The items of a species' tier in a dungeon.
std::vector<int> Items(Monster species, int dungeon, MinibossTier tier) {
    std::vector<int> items;
    for (const expanded::MinibossDrop &drop : expanded::MinibossLoot(species, dungeon, tier)) {
        items.push_back(drop.item);
    }
    return items;
}

} // namespace

TEST_F(MinibossLoot, RareDropFivePercentWithItsBoost) {
    expanded::MinibossDrop drop = Roll(DUNGEON_DIVINE_BEAST_CAVE, 0, Monster::MASTER_JACKET, {4, 0});
    EXPECT_EQ(drop.item, ITEM_WEAPON_GLADIUS);
    ASSERT_NE(drop.boost, nullptr);
    EXPECT_EQ(drop.boost->attack, 15);
    EXPECT_EQ(drop.boost->elem[4], 50);
    EXPECT_EQ(drop.boost->anti[MONSTER_FAMILY_UNDEAD], 50);
}

TEST_F(MinibossLoot, CommonDropThirtyPercent) {
    expanded::MinibossDrop drop = Roll(DUNGEON_DIVINE_BEAST_CAVE, 0, Monster::MASTER_JACKET, {5, 29, 0});
    EXPECT_EQ(drop.item, ITEM_WEAPON_GLADIUS);
    EXPECT_EQ(drop.boost, nullptr);

    EXPECT_EQ(Roll(DUNGEON_MOON_SEA, 0, Monster::VULCAN, {5, 29, 4}).item, ITEM_ATTACH_RUBY);
}

TEST_F(MinibossLoot, LootBelongsToTheSpecies) {
    EXPECT_EQ(Items(Monster::SKELETON_SOLDIER, DUNGEON_SHIPWRECK, MinibossTier::COMMON), (std::vector<int>{ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_BONE_SLINGSHOT}));
    EXPECT_EQ(Items(Monster::GOLEM, DUNGEON_SUN_MOON_TEMPLE, MinibossTier::RARE), (std::vector<int>{ITEM_WEAPON_SUN_SWORD, ITEM_WEAPON_TSUKIKAGE}));
    EXPECT_EQ(Items(Monster::TITAN, DUNGEON_MOON_SEA, MinibossTier::COMMON).size(), 12u);
    EXPECT_EQ(Items(Monster::GEMRON_WIND, DUNGEON_DEMON_SHAFT, MinibossTier::COMMON), (std::vector<int>{ITEM_ATTACH_EMERALD, ITEM_ATTACH_SUN}));
    EXPECT_EQ(Items(Monster::WEDNESDAY, DUNGEON_WISE_OWL_FOREST, MinibossTier::RARE), (std::vector<int>{ITEM_WEAPON_BANDIT_SLINGSHOT}));
    EXPECT_EQ(Items(Monster::KING_MIMIC_DS_ENHANCED, DUNGEON_DEMON_SHAFT, MinibossTier::COMMON).size(), 13u);
    EXPECT_EQ(Items(Monster::MIMIC_WOF, DUNGEON_WISE_OWL_FOREST, MinibossTier::COMMON).size(), 12u);
    EXPECT_TRUE(Items(Monster::DRAN, DUNGEON_DEMON_SHAFT, MinibossTier::RARE).empty());

    EXPECT_EQ(Roll(DUNGEON_SUN_MOON_TEMPLE, 0, Monster::MUMMY, {0, 0}).item, ITEM_WEAPON_CLAYMORE);
    EXPECT_EQ(Roll(DUNGEON_GALLERY_OF_TIME, 0, Monster::HORN_HEAD, {0, 0}).item, ITEM_WEAPON_SATAN_S_AX);
}

TEST_F(MinibossLoot, AnItemDropsFromItsDungeonOn) {
    EXPECT_TRUE(Items(Monster::DASHER, DUNGEON_DIVINE_BEAST_CAVE, MinibossTier::RARE).empty());
    EXPECT_EQ(Items(Monster::DASHER, DUNGEON_WISE_OWL_FOREST, MinibossTier::RARE), (std::vector<int>{ITEM_WEAPON_BATTLE_AX}));
    EXPECT_EQ(Items(Monster::CAPTAIN, DUNGEON_SHIPWRECK, MinibossTier::COMMON), (std::vector<int>{ITEM_GOLD_BULLION}));
    EXPECT_EQ(Items(Monster::CAPTAIN, DUNGEON_MOON_SEA, MinibossTier::COMMON), (std::vector<int>{ITEM_GOLD_BULLION, ITEM_BRAVERY_LAUNCH}));
    EXPECT_TRUE(Items(Monster::NIKAPOUS, DUNGEON_GALLERY_OF_TIME, MinibossTier::RARE).empty());
    EXPECT_EQ(Items(Monster::NIKAPOUS, DUNGEON_DEMON_SHAFT, MinibossTier::RARE), (std::vector<int>{ITEM_WEAPON_MACHO_SWORD}));
}

TEST_F(MinibossLoot, ALaterDungeonReplacesAnItem) {
    EXPECT_TRUE(Items(Monster::CURSED_ROSE, DUNGEON_WISE_OWL_FOREST, MinibossTier::RARE).empty());
    for (int dungeon : {DUNGEON_SHIPWRECK, DUNGEON_MOON_SEA}) {
        std::vector<expanded::MinibossDrop> rare = expanded::MinibossLoot(Monster::CURSED_ROSE, dungeon, MinibossTier::RARE);
        ASSERT_EQ(rare.size(), 1u);
        EXPECT_EQ(rare[0].item, ITEM_WEAPON_THORN_ARMLET);
        EXPECT_EQ(rare[0].boost, nullptr);
    }
    for (int dungeon : {DUNGEON_GALLERY_OF_TIME, DUNGEON_DEMON_SHAFT}) {
        std::vector<expanded::MinibossDrop> rare = expanded::MinibossLoot(Monster::CURSED_ROSE, dungeon, MinibossTier::RARE);
        ASSERT_EQ(rare.size(), 1u);
        EXPECT_EQ(rare[0].item, ITEM_WEAPON_THORN_ARMLET);
        ASSERT_NE(rare[0].boost, nullptr);
        EXPECT_EQ(rare[0].boost->abilities_high, 0x10);
    }

    EXPECT_EQ(Items(Monster::EARTH_DIGGER, DUNGEON_SUN_MOON_TEMPLE, MinibossTier::COMMON), (std::vector<int>{ITEM_WEAPON_PLATE_HAMMER}));
    EXPECT_EQ(Items(Monster::EARTH_DIGGER, DUNGEON_MOON_SEA, MinibossTier::COMMON), (std::vector<int>{ITEM_WEAPON_MAGICAL_HAMMER}));
    EXPECT_EQ(Items(Monster::FRIDAY, DUNGEON_SUN_MOON_TEMPLE, MinibossTier::RARE), (std::vector<int>{ITEM_WEAPON_BANDIT_SLINGSHOT}));
    EXPECT_EQ(Items(Monster::FRIDAY, DUNGEON_MOON_SEA, MinibossTier::RARE), (std::vector<int>{ITEM_WEAPON_POCKLEKUL}));
}

TEST_F(MinibossLoot, TheSameWeaponPlainOrBoosted) {
    expanded::MinibossDrop common = Roll(DUNGEON_GALLERY_OF_TIME, 0, Monster::BISHOP_Q, {5, 0, 0});
    EXPECT_EQ(common.item, ITEM_WEAPON_FLAMINGO);
    EXPECT_EQ(common.boost, nullptr);

    expanded::MinibossDrop rare = Roll(DUNGEON_GALLERY_OF_TIME, 0, Monster::BISHOP_Q, {0, 0});
    EXPECT_EQ(rare.item, ITEM_WEAPON_FLAMINGO);
    ASSERT_NE(rare.boost, nullptr);
    EXPECT_EQ(rare.boost->abilities_high, 0x20);

    ASSERT_EQ(expanded::MinibossLoot(Monster::STEEL_GIANT, DUNGEON_WISE_OWL_FOREST, MinibossTier::RARE).size(), 1u);
    EXPECT_EQ(expanded::MinibossLoot(Monster::STEEL_GIANT, DUNGEON_WISE_OWL_FOREST, MinibossTier::RARE)[0].boost->abilities_high, 0x02);
    EXPECT_EQ(expanded::MinibossLoot(Monster::ARTHUR, DUNGEON_MOON_SEA, MinibossTier::COMMON)[0].boost, nullptr);
}

TEST_F(MinibossLoot, EnhancedSpeciesTakeTheirBasesLoot) {
    for (int dungeon = 0; dungeon < DUNGEON_COUNT; dungeon++) {
        for (MinibossTier tier : {MinibossTier::RARE, MinibossTier::COMMON}) {
            EXPECT_EQ(Items(Monster::CURSED_ROSE_ENHANCED, dungeon, tier), Items(Monster::CURSED_ROSE, dungeon, tier));
            EXPECT_EQ(Items(Monster::STEEL_GIANT_ENHANCED, dungeon, tier), Items(Monster::STEEL_GIANT, dungeon, tier));
        }
    }
    expanded::MinibossDrop drop = Roll(DUNGEON_DEMON_SHAFT, 0, Monster::MASTER_JACKET_ENHANCED, {0, 0});
    EXPECT_EQ(drop.item, ITEM_WEAPON_GLADIUS);
    ASSERT_NE(drop.boost, nullptr);
    EXPECT_EQ(drop.boost->attack, 15);
}

TEST_F(MinibossLoot, DeSangaComesPlain) {
    for (Monster fang : {Monster::WHITE_FANG, Monster::WHITE_FANG_ENHANCED}) {
        expanded::MinibossDrop drop = Roll(DUNGEON_MOON_SEA, 0, fang, {0, 0});
        EXPECT_EQ(drop.item, ITEM_WEAPON_DE_SANGA);
        EXPECT_EQ(drop.boost, nullptr);
    }
}

TEST_F(MinibossLoot, EveryLootedSpeciesHasLoot) {
    EXPECT_EQ(std::size(kLooted), 118u);
    for (Monster species : kLooted) {
        EXPECT_FALSE(expanded::MinibossLoot(species, DUNGEON_DEMON_SHAFT, MinibossTier::RARE).empty() &&
                     expanded::MinibossLoot(species, DUNGEON_DEMON_SHAFT, MinibossTier::COMMON).empty())
            << expanded::No(species);
    }
}

TEST_F(MinibossLoot, AnEmptyTierFallsThrough) {
    EXPECT_EQ(Roll(DUNGEON_DIVINE_BEAST_CAVE, 0, Monster::STEEL_GIANT, {4, 34}).item, ITEM_TRAM_OIL);
    EXPECT_EQ(Roll(DUNGEON_WISE_OWL_FOREST, 0, Monster::STEEL_GIANT, {4, 0}).item, ITEM_WEAPON_STEEL_SLINGSHOT);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, Monster::OPAR, {4, 34}).item, ITEM_FLAPPING_FISH);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, Monster::STATUE_DOG, {5, 29, 34}).item, ITEM_FLAPPING_FISH);
}

TEST_F(MinibossLoot, OtherwiseKeyWeaponAttachmentOrItem) {
    EXPECT_EQ(Roll(DUNGEON_DIVINE_BEAST_CAVE, 0, Monster::MASTER_JACKET, {5, 30, 34}).item, ITEM_TRAM_OIL);
    EXPECT_EQ(Roll(DUNGEON_DEMON_SHAFT, 0, Monster::DRAN, {0, 34}).item, ITEM_CRYSTAL_EYEBALL);
    EXPECT_EQ(Roll(DUNGEON_DIVINE_BEAST_CAVE, 8, Monster::DRAN, {5, 30, 35, 14, 3}).item, ITEM_WEAPON_BONE_RAPIER);
    EXPECT_EQ(Roll(DUNGEON_DIVINE_BEAST_CAVE, 9, Monster::DRAN, {5, 30, 35, 14, 6}).item, ITEM_WEAPON_BONE_SLINGSHOT);
    EXPECT_EQ(Roll(DUNGEON_GALLERY_OF_TIME, 20, Monster::DRAN, {5, 30, 35, 14, 13}).item, ITEM_WEAPON_SWALLOW);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, Monster::DRAN, {5, 30, 35, 15, 79, 59, 11}).item, ITEM_ATTACH_TURQUOISE);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, Monster::DRAN, {5, 30, 35, 15, 79, 60, 18}).item, ITEM_ATTACH_MAGE_SLAYER);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, Monster::DRAN, {5, 30, 35, 15, 80, 59, 1}).item, ITEM_AUTO_REPAIR_POWDER);
    EXPECT_EQ(Roll(DUNGEON_SHIPWRECK, 0, Monster::DRAN, {5, 30, 35, 15, 80, 60, 1}).item, ITEM_DRAN_S_FEATHER);
    EXPECT_EQ(Roll(-1, 0, Monster::MASTER_JACKET, {}).item, -1);
}

TEST_F(MinibossLoot, BoostWritesItsStatsAndAbilityBytes) {
    WEAPON_HAVE weapon = {};
    weapon.attack = 20;
    weapon.durability = 30;
    weapon.durability_f = 12.0f;
    weapon.elem[0] = 5;
    weapon.best_elem = 0;
    weapon.flags = 0x0208;

    expanded::MinibossDrop drop = Roll(DUNGEON_MOON_SEA, 0, Monster::SPACE_GYON, {0, 0});
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

TEST_F(MinibossLoot, AMinibossCarriesItsSpeciesDrop) {
    selectMapNo = DUNGEON_DIVINE_BEAST_CAVE;
    auto unit = std::make_unique<CMonstorUnit>();
    unit->model[0] = MonstorTable[expanded::No(Monster::MASTER_JACKET)];
    expanded::EnemyModelSetUp(*unit, 0, expanded::No(Monster::MASTER_JACKET));
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
