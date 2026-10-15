#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/enemies/randomize.hpp"
#include "expanded/enemies/roster.hpp"
#include "monstorunit.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"
#include "userstatus.hpp"

namespace {

constexpr int kDivineBeastCave = 0;
constexpr int kDemonShaft = 6;
constexpr int kMimicDbc = 30;
constexpr int kKingMimicDbc = 29;
constexpr int kGaciousEnhanced = 155;

bool InPool(int kind) {
    std::span<const int> pool = expanded::RandomizerPool();
    return std::find(pool.begin(), pool.end(), kind) != pool.end();
}

// The Randomize Enemies toggle set, a dungeon floor stood on, and both put back after.
class RandomizeFloors : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = std::filesystem::temp_directory_path() / ("dc_randomize_" + std::to_string(dc::test::ProcessId()));
        std::filesystem::create_directories(root_);
        PathsSetSaveRoot(root_);
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
        std::filesystem::remove_all(root_);
    }

    static void Randomize(bool on) {
        Config config = ConfigGet();
        ASSERT_TRUE(expanded::SetOption(config.expanded_options, expanded::kRandomizeEnemies, on));
        ASSERT_TRUE(ConfigChange(config));
    }

    // BtLoadMonstor's read of a floor side.
    BT_ENEMY_FLOOR *Load(int dungeon, int floor, int ura) {
        selectMapNo = dungeon;
        UserStatus->cur_floor = static_cast<s8>(floor);
        BT_ENEMY_FLOOR *layout = (ura != 0 ? BtUraEnemyLayoutList : BtEnemyLayoutList)[dungeon] + floor;
        return expanded::RosterFloorLayout(layout, ura);
    }

    std::filesystem::path        root_;
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
    std::span<const int> pool = expanded::RandomizerPool();
    EXPECT_EQ(pool.size(), 119u);
    for (int kind : pool) {
        EXPECT_EQ(MonstorTable[kind].kind, MONSTER_KIND_NORMAL) << kind;
        EXPECT_EQ(MonstorTable[kind].model_name[0][0], 'e') << kind;
        EXPECT_NE(MonstorTable[kind].attachment_kind, MONSTER_FAMILY_MIMIC) << kind;
    }
    EXPECT_EQ(std::count(pool.begin(), pool.end(), kGaciousEnhanced), 2);
    EXPECT_FALSE(InPool(105)); // Gacious, a boss record
    EXPECT_FALSE(InPool(91));  // Wine Keg
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
    EXPECT_EQ(expanded::MimicsOf(kDivineBeastCave, 5).mimic, kMimicDbc);
    EXPECT_EQ(expanded::MimicsOf(kDivineBeastCave, 5).king, kKingMimicDbc);
    EXPECT_EQ(expanded::MimicsOf(kDemonShaft, 39).mimic, 130);
    EXPECT_EQ(expanded::MimicsOf(kDemonShaft, 40).king, 142);
    EXPECT_EQ(expanded::MimicsOf(kDemonShaft, 98).mimic, 163);
}

TEST(EnemyRandomize, AMixIsNineKindsWithTheDungeonsMimicsByChance) {
    std::mt19937  random(7);
    int           mimics = 0, kings = 0;
    constexpr int kRolls = 4000;
    for (int roll = 0; roll < kRolls; roll++) {
        expanded::Roster roster = expanded::RandomMix(kDivineBeastCave, 0, random);
        ASSERT_EQ(roster.Size(), expanded::kRosterSize);
        for (int i = 0; i < roster.Size(); i++) {
            int kind = roster.model_no[i];
            ASSERT_EQ(std::count(roster.model_no, roster.model_no + expanded::kRosterSize, kind), 1);
            if (kind == kMimicDbc) {
                mimics++;
                EXPECT_FALSE(roster.once[i]);
            } else if (kind == kKingMimicDbc) {
                kings++;
                EXPECT_TRUE(roster.once[i]);
            } else {
                EXPECT_TRUE(InPool(kind)) << kind;
                EXPECT_FALSE(roster.once[i]);
            }
        }
    }
    EXPECT_NEAR(mimics / static_cast<double>(kRolls), expanded::kMimicChance, 0.03);
    EXPECT_NEAR(kings / static_cast<double>(kRolls), expanded::kKingMimicChance, 0.03);
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
    rosters.front.Add(1, false);
    rosters.front.Add(kKingMimicDbc, false);
    rosters.front.Add(3, true);
    expanded::StageFloor(5, rosters);
    Load(kDivineBeastCave, 5, 0);

    auto unit = std::make_unique<CMonstorUnit>();
    unit->model[1] = MonstorTable[kKingMimicDbc];
    EXPECT_FALSE(expanded::RosterSpawnsOnce(*unit, 0));
    EXPECT_FALSE(expanded::RosterSpawnsOnce(*unit, 1));
    EXPECT_TRUE(expanded::RosterSpawnsOnce(*unit, 2));

    // A floor of its own goes by the kind.
    Load(kDivineBeastCave, 14, 0);
    EXPECT_TRUE(expanded::RosterSpawnsOnce(*unit, 1));
}
