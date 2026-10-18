#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/randomize.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/species.hpp"
#include "monstorunit.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::No;

constexpr int kDivineBeastCave = 0;
constexpr int kDemonShaft = 6;

// A regular kind: an 'e' model of a normal kind but the mimics, Wine Keg and the boss-record Gacious, or an Expanded
// kind.
bool Regular(int kind) {
    const MONSTOR_MODEL &species = *expanded::Species(kind);
    if (kind >= expanded::kRetailSpecies) {
        return true;
    }
    return species.kind == MONSTER_KIND_NORMAL && species.model_name[0][0] == 'e' && species.attachment_kind != MONSTER_FAMILY_MIMIC &&
           kind != No(Monster::WINE_KEG) && kind != No(Monster::GACIOUS);
}

bool InPool(Monster kind) {
    std::span<const Monster> pool = expanded::RandomizerPool();
    return std::find(pool.begin(), pool.end(), kind) != pool.end();
}

// The Randomize Enemies toggle set, a dungeon floor stood on, and both put back after.
class RandomizeFloors : public ::testing::Test {
protected:
    void SetUp() override {
        expanded::SeedModRandom(1);
        saved_status_ = UserStatus;
        UserStatus = status_.get();
        saved_map_ = selectMapNo;
        Randomize(true);
        expanded::RosterVisitBegun();
    }

    void TearDown() override {
        Randomize(false);
        expanded::RosterVisitBegun();
        UserStatus = saved_status_;
        selectMapNo = saved_map_;
    }

    void Randomize(bool on) { ASSERT_TRUE(options_.Set(expanded::kRandomizeEnemies, on)); }

    // BtLoadMonstor's read of a floor side.
    BT_ENEMY_FLOOR *Load(int dungeon, int floor, int ura) {
        selectMapNo = dungeon;
        UserStatus->cur_floor = static_cast<s8>(floor);
        BT_ENEMY_FLOOR *layout = (ura != 0 ? BtUraEnemyLayoutList : BtEnemyLayoutList)[dungeon] + floor;
        return expanded::RosterFloorLayout(layout, ura);
    }

    expanded::test::OptionScope  options_;
    std::unique_ptr<CUserStatus> status_ = std::make_unique<CUserStatus>();
    CUserStatus                 *saved_status_ = nullptr;
    s32                          saved_map_ = 0;
};

} // namespace

TEST(EnemyRandomize, IsAnExpandedToggleOffByDefault) {
    EXPECT_FALSE(expanded::RandomizeEnemies());
    bool listed = false;
    for (const expanded::Toggle &toggle : expanded::Toggles()) {
        listed = listed || std::string(toggle.key) == expanded::kRandomizeEnemies;
    }
    EXPECT_TRUE(listed);
}

TEST(EnemyRandomize, ThePoolIsEveryRegularMonster) {
    std::span<const Monster> pool = expanded::RandomizerPool();
    for (int kind = 0; kind < expanded::kSpeciesCount; kind++) {
        EXPECT_EQ(InPool(static_cast<Monster>(kind)), Regular(kind)) << kind;
    }
    for (Monster monster : pool) {
        const MONSTOR_MODEL *species = expanded::Species(No(monster));
        ASSERT_NE(species, nullptr) << No(monster);
        EXPECT_EQ(species->kind, MONSTER_KIND_NORMAL) << No(monster);
        EXPECT_EQ(species->model_name[0][0], 'e') << No(monster);
        EXPECT_NE(species->attachment_kind, MONSTER_FAMILY_MIMIC) << No(monster);
    }
    EXPECT_EQ(std::count(pool.begin(), pool.end(), Monster::GACIOUS_ENHANCED), 2);
    EXPECT_FALSE(InPool(Monster::GACIOUS)); // a boss record
    EXPECT_FALSE(InPool(Monster::WINE_KEG));
}

TEST(EnemyRandomize, ThePoolKeepsItsIndexes) {
    std::vector<int> indexes;
    auto             range = [&](int first, int last) {
        for (int kind = first; kind <= last; kind++) {
            indexes.push_back(kind);
        }
    };
    range(0, 28);
    range(35, 69);
    range(77, 77);
    range(94, 100);
    range(111, 129);
    range(132, 140);
    range(143, 151);
    range(154, 162);
    range(155, 155);
    range(167, 169);
    std::span<const Monster> pool = expanded::RandomizerPool();
    ASSERT_EQ(pool.size(), indexes.size());
    for (size_t i = 0; i < pool.size(); i++) {
        EXPECT_EQ(No(pool[i]), indexes[i]) << i;
    }
}

TEST(EnemyRandomize, BossAndEventFloorsKeepTheirMonsters) {
    EXPECT_TRUE(expanded::RandomizedFloor(kDivineBeastCave, 0));
    EXPECT_FALSE(expanded::RandomizedFloor(kDivineBeastCave, 3));
    EXPECT_FALSE(expanded::RandomizedFloor(kDivineBeastCave, 14));
    EXPECT_FALSE(expanded::RandomizedFloor(5, 24));
    EXPECT_FALSE(expanded::RandomizedFloor(kDemonShaft, 99));
    EXPECT_TRUE(expanded::RandomizedFloor(kDemonShaft, 98));
}

TEST(EnemyRandomize, DemonShaftsMimicsFollowItsBands) {
    EXPECT_EQ(expanded::MimicsOf(kDivineBeastCave, 5)->mimic, Monster::MIMIC_DBC);
    EXPECT_EQ(expanded::MimicsOf(kDivineBeastCave, 5)->king, Monster::KING_MIMIC_DBC);
    EXPECT_EQ(expanded::MimicsOf(kDemonShaft, 39)->mimic, Monster::MIMIC_DS);
    EXPECT_EQ(expanded::MimicsOf(kDemonShaft, 40)->king, Monster::KING_MIMIC_DS_ENHANCED);
    EXPECT_EQ(expanded::MimicsOf(kDemonShaft, 98)->mimic, Monster::MIMIC_DS_ENHANCED_THRICE);
    EXPECT_FALSE(expanded::MimicsOf(7, 0).has_value());
}

TEST(EnemyRandomize, TheMimicsKeepTheirIndexes) {
    // The six dungeons' and Demon Shaft's, then its bands' from floor 40, 60 and 80.
    const int mimics[] = {30, 71, 73, 32, 34, 75, 130, 141, 152, 163};
    const int kings[] = {29, 70, 72, 31, 33, 74, 131, 142, 153, 164};
    for (int i = 0; i < 10; i++) {
        int dungeon = std::min(i, kDemonShaft);
        int floor = i <= kDemonShaft ? 0 : 40 + (i - kDemonShaft - 1) * 20;
        EXPECT_EQ(No(expanded::MimicsOf(dungeon, floor)->mimic), mimics[i]) << i;
        EXPECT_EQ(No(expanded::MimicsOf(dungeon, floor)->king), kings[i]) << i;
    }
}

TEST(EnemyRandomize, AMixIsDistinctKindsWithTheDungeonsMimicsByChance) {
    expanded::SeedModRandom(1);
    std::mt19937 &random = expanded::ModRandom();
    int           mimics = 0, kings = 0;
    constexpr int kRolls = 4000;
    for (int roll = 0; roll < kRolls; roll++) {
        expanded::Roster roster = expanded::RandomMix(kDivineBeastCave, 0, random);
        ASSERT_GE(roster.Size(), 1);
        ASSERT_LE(roster.Size(), expanded::kRosterSize);
        for (int i = 0; i < roster.Size(); i++) {
            int kind = roster.model_no[i];
            ASSERT_EQ(std::count(roster.model_no, roster.model_no + expanded::kRosterSize, kind), 1);
            if (kind == No(Monster::MIMIC_DBC)) {
                mimics++;
                EXPECT_FALSE(roster.once[i]);
            } else if (kind == No(Monster::KING_MIMIC_DBC)) {
                kings++;
                EXPECT_TRUE(roster.once[i]);
            } else {
                EXPECT_TRUE(InPool(static_cast<Monster>(kind))) << kind;
                EXPECT_FALSE(roster.once[i]);
            }
        }
    }
    EXPECT_NEAR(mimics / static_cast<double>(kRolls), expanded::kMimicChance, 0.03);
    EXPECT_NEAR(kings / static_cast<double>(kRolls), expanded::kKingMimicChance, 0.03);
}

TEST(EnemyRandomize, TheKindCountWeightsAreTheTable) {
    // The table as published, in rounded percents; the weights are its exact form.
    const std::array<double, 9> table = {1.00, 0.14, 0.31, 0.55, 9.61, 13.84, 18.82, 24.59, 31.14};
    double                      sum = 0.0;
    for (int i = 0; i < expanded::kRosterSize; i++) {
        EXPECT_NEAR(expanded::kKindCountWeights[i] * 100.0, table[i], 0.015) << i + 1;
        sum += expanded::kKindCountWeights[i];
    }
    EXPECT_NEAR(sum, 1.0, 1e-12);
    EXPECT_DOUBLE_EQ(expanded::kKindCountWeights[0], 0.01);
    EXPECT_DOUBLE_EQ(expanded::kKindCountWeights[1] + expanded::kKindCountWeights[2] + expanded::kKindCountWeights[3], 0.01);
}

TEST(EnemyRandomize, TheKindCountFollowsTheTable) {
    expanded::SeedModRandom(1);
    constexpr int       kRolls = 200000;
    std::array<int, 10> seen = {};
    for (int roll = 0; roll < kRolls; roll++) {
        seen[expanded::RollKindCount(expanded::kRosterSize, expanded::ModRandom())]++;
    }
    EXPECT_EQ(seen[0], 0);
    for (int n = 1; n <= expanded::kRosterSize; n++) {
        EXPECT_NEAR(seen[n] / static_cast<double>(kRolls), expanded::kKindCountWeights[n - 1], 0.004) << n;
    }
}

TEST(EnemyRandomize, BothMimicsRescaleTheCountOverOneToSeven) {
    expanded::SeedModRandom(1);
    // Two mimics leave room for seven regular kinds, the weights of one to seven rescaled.
    double kept = 0.0;
    for (int n = 1; n <= 7; n++) {
        kept += expanded::kKindCountWeights[n - 1];
    }
    constexpr int       kRolls = 100000;
    std::array<int, 10> seen = {};
    int                 both = 0;
    for (int roll = 0; roll < kRolls; roll++) {
        expanded::Roster roster = expanded::RandomMix(kDivineBeastCave, 0, expanded::ModRandom());
        if (roster.Contains(No(Monster::MIMIC_DBC)) && roster.Contains(No(Monster::KING_MIMIC_DBC))) {
            both++;
            seen[roster.Size() - 2]++;
        }
    }
    ASSERT_GT(both, 7500);
    EXPECT_EQ(seen[8] + seen[9], 0);
    for (int n = 1; n <= 7; n++) {
        EXPECT_NEAR(seen[n] / static_cast<double>(both), expanded::kKindCountWeights[n - 1] / kept, 0.008) << n;
    }
    EXPECT_EQ(expanded::RollKindCount(0, expanded::ModRandom()), 0);
}

TEST(EnemyRandomize, MimicsAreNeverCrowdedOut) {
    for (unsigned seed = 0; seed < 2000; seed++) {
        std::mt19937                           random(seed);
        std::mt19937                           replay(seed);
        std::uniform_real_distribution<double> chance(0.0, 1.0);
        bool                                   mimic = chance(replay) < expanded::kMimicChance;
        bool                                   king = chance(replay) < expanded::kKingMimicChance;
        expanded::Roster                       roster = expanded::RandomMix(kDivineBeastCave, 0, random);
        EXPECT_EQ(roster.Contains(No(Monster::MIMIC_DBC)), mimic) << seed;
        EXPECT_EQ(roster.Contains(No(Monster::KING_MIMIC_DBC)), king) << seed;
        EXPECT_GE(roster.Size(), 1 + mimic + king) << seed;
    }
}

TEST_F(RandomizeFloors, AFloorIsRolledOnceAVisit) {
    BT_ENEMY_FLOOR *front = Load(kDivineBeastCave, 1, 0);
    ASSERT_NE(front, BtEnemyLayoutList[kDivineBeastCave] + 1);
    const expanded::FloorRosters *staged = expanded::StagedFloor(1);
    ASSERT_NE(staged, nullptr);
    expanded::FloorRosters first = *staged;
    for (int i = 0; i < expanded::kRosterSize; i++) {
        EXPECT_EQ(front->monster[i].monster_no, first.front.model_no[i]);
    }
    EXPECT_EQ(expanded::LoadedRoster(), &staged->front);

    // The back side and a return to the floor keep what was rolled.
    BT_ENEMY_FLOOR *back = Load(kDivineBeastCave, 1, 1);
    for (int i = 0; i < expanded::kRosterSize; i++) {
        EXPECT_EQ(back->monster[i].monster_no, first.back.model_no[i]);
    }
    Load(kDivineBeastCave, 2, 0);
    Load(kDivineBeastCave, 1, 0);
    EXPECT_EQ(expanded::StagedFloor(1)->front, first.front);
    EXPECT_EQ(expanded::StagedFloor(1)->back, first.back);

    // A new visit rolls again.
    expanded::RosterVisitBegun();
    EXPECT_EQ(expanded::StagedFloor(1), nullptr);
    EXPECT_EQ(expanded::LoadedRoster(), nullptr);
}

TEST_F(RandomizeFloors, BossFloorsAndTheToggleOffLoadTheirOwn) {
    BT_ENEMY_FLOOR *boss = BtEnemyLayoutList[kDivineBeastCave] + 14;
    EXPECT_EQ(Load(kDivineBeastCave, 14, 0), boss);
    EXPECT_EQ(expanded::LoadedRoster(), nullptr);

    Randomize(false);
    BT_ENEMY_FLOOR *own = BtEnemyLayoutList[kDivineBeastCave] + 4;
    EXPECT_EQ(Load(kDivineBeastCave, 4, 0), own);
    EXPECT_EQ(expanded::StagedFloor(4), nullptr);
}

TEST_F(RandomizeFloors, ArrangementSpawnsOnceWhatTheRosterSays) {
    expanded::FloorRosters rosters;
    rosters.front.Add(Monster::SKELETON_SOLDIER, false);
    rosters.front.Add(Monster::KING_MIMIC_DBC, false);
    rosters.front.Add(Monster::DASHER, true);
    expanded::StageFloor(5, rosters);
    Load(kDivineBeastCave, 5, 0);

    auto unit = std::make_unique<CMonstorUnit>();
    unit->model[1] = MonstorTable[No(Monster::KING_MIMIC_DBC)];
    EXPECT_FALSE(expanded::RosterSpawnsOnce(*unit, 0));
    EXPECT_FALSE(expanded::RosterSpawnsOnce(*unit, 1));
    EXPECT_TRUE(expanded::RosterSpawnsOnce(*unit, 2));

    // A floor of its own goes by the kind.
    Load(kDivineBeastCave, 14, 0);
    EXPECT_TRUE(expanded::RosterSpawnsOnce(*unit, 1));
}
