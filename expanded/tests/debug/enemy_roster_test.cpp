#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/debug/enemy_roster.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "itemdata.hpp"
#include "mainselect.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::No;

// Debug mode on, a dungeon floor stood on, and the roster cleared after.
class DebugRoster : public ::testing::Test {
protected:
    void SetUp() override {
        saved_debug_ = DebugMode;
        saved_status_ = UserStatus;
        saved_map_ = selectMapNo;
        DebugMode = 1;
        UserStatus = status_.get();
        expanded::RosterVisitBegun();
    }

    void TearDown() override {
        expanded::SetDebugRoster("");
        expanded::RosterVisitBegun();
        DebugMode = saved_debug_;
        UserStatus = saved_status_;
        selectMapNo = saved_map_;
    }

    // BtLoadMonstor's read of a floor's front.
    BT_ENEMY_FLOOR *Load(int dungeon, int floor) {
        selectMapNo = dungeon;
        UserStatus->cur_floor = static_cast<s8>(floor);
        return expanded::RosterFloorLayout(BtEnemyLayoutList[dungeon] + floor, 0);
    }

    std::unique_ptr<CUserStatus> status_ = std::make_unique<CUserStatus>();
    CUserStatus                 *saved_status_ = nullptr;
    s32                          saved_debug_ = 0;
    s32                          saved_map_ = 0;
};

} // namespace

TEST(DebugRosterSpec, ParsesKindsAndSpawnOnceMarks) {
    expanded::Roster roster;
    ASSERT_TRUE(expanded::ParseRosterSpec(" 20!, 60 ,83", roster));
    EXPECT_EQ(roster.Size(), 3);
    EXPECT_EQ(roster.model_no[0], No(Monster::GYON));
    EXPECT_TRUE(roster.once[0]);
    EXPECT_EQ(roster.model_no[1], No(Monster::CURSED_ROSE));
    EXPECT_FALSE(roster.once[1]);
    // A boss spawns once unmarked.
    EXPECT_EQ(roster.model_no[2], No(Monster::MINOTAUR_JOE));
    EXPECT_TRUE(roster.once[2]);

    ASSERT_TRUE(expanded::ParseRosterSpec("20,", roster));
    EXPECT_EQ(roster.Size(), 1);
    ASSERT_TRUE(expanded::ParseRosterSpec("", roster));
    EXPECT_EQ(roster.Size(), 0);
}

TEST(DebugRosterSpec, RefusesBadSpecs) {
    expanded::Roster roster;
    EXPECT_FALSE(expanded::ParseRosterSpec("20,,60", roster));
    EXPECT_FALSE(expanded::ParseRosterSpec("170", roster));
    EXPECT_FALSE(expanded::ParseRosterSpec("-1", roster));
    EXPECT_FALSE(expanded::ParseRosterSpec("20,20", roster));
    EXPECT_FALSE(expanded::ParseRosterSpec("1,2,3,4,5,6,7,8,9,10", roster));
    EXPECT_FALSE(expanded::ParseRosterSpec("2x", roster));
    EXPECT_FALSE(expanded::SetDebugRoster("abc"));
}

TEST_F(DebugRoster, LaysOutEveryFloorFromTheNextVisit) {
    ASSERT_TRUE(expanded::SetDebugRoster("20!,60"));
    EXPECT_EQ(expanded::DebugRosterSpec(), "20!,60");
    // The visit it was set in keeps its own monsters.
    EXPECT_EQ(Load(0, 1), BtEnemyLayoutList[0] + 1);

    expanded::RosterVisitBegun();
    for (int floor : {1, 14}) {
        BT_ENEMY_FLOOR *layout = Load(0, floor);
        EXPECT_EQ(layout->monster[0].monster_no, No(Monster::GYON));
        EXPECT_EQ(layout->monster[1].monster_no, No(Monster::CURSED_ROSE));
        EXPECT_EQ(layout->monster[2].monster_no, -1);
        ASSERT_NE(expanded::LoadedRoster(), nullptr);
        EXPECT_TRUE(expanded::LoadedRoster()->once[0]);
        EXPECT_FALSE(expanded::LoadedRoster()->once[1]);
    }
    EXPECT_EQ(expanded::StagedFloor(1)->back, expanded::StagedFloor(1)->front);
}

TEST_F(DebugRoster, OnlyInDebugModeAndUntilCleared) {
    ASSERT_TRUE(expanded::SetDebugRoster("20"));
    DebugMode = 0;
    expanded::RosterVisitBegun();
    EXPECT_EQ(Load(0, 2), BtEnemyLayoutList[0] + 2);

    DebugMode = 1;
    ASSERT_TRUE(expanded::SetDebugRoster(""));
    expanded::RosterVisitBegun();
    EXPECT_EQ(Load(0, 2), BtEnemyLayoutList[0] + 2);
}

TEST(DebugRosterSpec, TheMenuRowIsCutToItsRoom) {
    char text[8];
    EXPECT_EQ(expanded::DevMenuRosterDraw(text, sizeof(text), false), 7);
    EXPECT_STREQ(text, " enemy ");
    EXPECT_EQ(expanded::DevMenuRosterDraw(text, 0, false), 0);
}

TEST(DebugRosterSpec, SaysWhyASpecIsRefused) {
    expanded::Roster roster;
    std::string      why;
    EXPECT_FALSE(expanded::ParseRosterSpec("20,,60", roster, &why));
    EXPECT_EQ(why, "a kind is missing between commas");
    EXPECT_FALSE(expanded::ParseRosterSpec("2x", roster, &why));
    EXPECT_EQ(why, "2x is not a kind: kinds are 0 to 169");
    EXPECT_FALSE(expanded::ParseRosterSpec("20,20", roster, &why));
    EXPECT_EQ(why, "20 is listed twice");
    EXPECT_FALSE(expanded::ParseRosterSpec("1,2,3,4,5,6,7,8,9,10", roster, &why));
    EXPECT_EQ(why, "a roster holds at most 9 kinds");
}

TEST(DebugRosterSpec, BossesThatNeedAPartyMemberNeedAnotherKind) {
    std::string why;
    EXPECT_FALSE(expanded::SetDebugRoster("80", &why));
    EXPECT_EQ(why, "80 (Ice Queen) and 101 (Ice Barrier) come only with Ruby in the party: add another kind");
    EXPECT_FALSE(expanded::SetDebugRoster("80,101", &why));
    EXPECT_FALSE(expanded::SetDebugRoster("82", &why));
    EXPECT_EQ(why, "81 (King's Curse's coffin) and 82 (King's Curse) come only with Ungaga in the party: add another kind");
    EXPECT_FALSE(expanded::SetDebugRoster("80,82", &why));
    EXPECT_NE(expanded::DebugRosterSpec(), "80,82");

    // Another kind keeps the floor from being empty; the Black Knight needs no one.
    EXPECT_TRUE(expanded::SetDebugRoster("1,80", &why));
    EXPECT_TRUE(expanded::SetDebugRoster("165", &why));
    EXPECT_TRUE(expanded::SetDebugRoster("166", &why));
    EXPECT_TRUE(expanded::SetDebugRoster("", &why));
}

TEST(DebugRosterSpec, ABossPartnerNeedsRoom) {
    std::string why;
    EXPECT_FALSE(expanded::SetDebugRoster("1,2,3,4,5,6,7,8,80", &why));
    EXPECT_EQ(why, "no room for 101 (Ice Barrier), which comes with 80 (Ice Queen): drop a kind");
    EXPECT_TRUE(expanded::SetDebugRoster("1,2,3,4,5,6,7,80,101", &why));
    // The Black Knight alone needs no room for his mount.
    EXPECT_TRUE(expanded::SetDebugRoster("1,2,3,4,5,6,7,8,165", &why));
    EXPECT_FALSE(expanded::SetDebugRoster("1,2,3,4,5,6,7,8,166", &why));
    EXPECT_EQ(why, "no room for 165 (Black Knight), which comes with 166 (Black Knight's mount): drop a kind");
    EXPECT_TRUE(expanded::SetDebugRoster("", &why));
}

TEST(DebugRosterSpec, AnExpandedKindMustBeBuilt) {
    expanded::test::ScratchDir    empty("roster_no_data");
    expanded::test::DataRootScope root(empty.path());
    std::string                   why;
    EXPECT_FALSE(expanded::SetDebugRoster("1,167", &why));
    EXPECT_EQ(why, "167 is not built yet: run `dcdata expanded <data>`");
}

TEST(DebugRosterStaging, AFloorLeftWithNoKindKeepsItsOwnMonsters) {
    expanded::test::TestFloor floor(0, 2);
    expanded::RosterVisitBegun();
    floor.status().party_size = 1; // no Ruby
    expanded::FloorRosters only_queen;
    only_queen.front.Add(Monster::ICE_QUEEN, true);
    only_queen.back.Add(Monster::ICE_QUEEN, true);
    expanded::StageFloor(2, only_queen);
    EXPECT_EQ(expanded::StagedFloor(2), nullptr);
    EXPECT_EQ(expanded::RosterFloorLayout(BtEnemyLayoutList[0] + 2, 0), BtEnemyLayoutList[0] + 2);

    floor.status().party_size = CHARA_RUBY + 1;
    expanded::StageFloor(2, only_queen);
    ASSERT_NE(expanded::StagedFloor(2), nullptr);
    EXPECT_TRUE(expanded::StagedFloor(2)->front.Contains(Monster::ICE_BARRIER));
    expanded::RosterVisitBegun();
}
