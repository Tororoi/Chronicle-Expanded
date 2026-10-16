#include <gtest/gtest.h>

#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/debug/enemy_roster.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "mainselect.hpp"
#include "monstorunit.hpp"
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
    EXPECT_FALSE(expanded::ParseRosterSpec("167", roster));
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
