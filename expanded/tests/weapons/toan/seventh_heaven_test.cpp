#include <gtest/gtest.h>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/toan/seventh_heaven.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// The kind a monster's hit of kind reaches the player as, with weapon_no in hand (carrying sphere when given).
int Reaches(int kind, int weapon_no, int sphere = 0) {
    expanded::test::Wielding wielding(weapon_no, sphere);
    COLLISION_HIT            hit = {};
    hit.kind = kind;
    expanded::DivineGuardHit(hit);
    return hit.kind;
}

} // namespace

TEST(SeventhHeaven, AGuardBreakingHitComesInAsAKnockback) {
    EXPECT_EQ(Reaches(3, ITEM_WEAPON_7THHEAVEN), 2);
    EXPECT_EQ(Reaches(2, ITEM_WEAPON_7THHEAVEN), 2);
    EXPECT_EQ(Reaches(4, ITEM_WEAPON_7THHEAVEN), 4);
    EXPECT_EQ(Reaches(3, ITEM_WEAPON_DARK_CLOUD), 3);
}

TEST(SeventhHeaven, SuperStevesSphereGuardsXiaoTheSameWay) {
    EXPECT_EQ(Reaches(3, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_7THHEAVEN), 2);
    EXPECT_EQ(Reaches(3, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER), 3);
}

TEST(SeventhHeaven, InheritsGuardCrush) {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding(ITEM_WEAPON_7THHEAVEN);
    EXPECT_FALSE(expanded::MonsterGuardBlocks(floor.unit(), 0, 0, COLLISION_HIT{}));
}

TEST(SeventhHeaven, ABlockedHitLeavesTheGuardHeld) {
    expanded::test::Wielding wielding(ITEM_WEAPON_7THHEAVEN);
    // The hit has taken the player out of their guard, as any hit does.
    BtActStatus = {};
    expanded::DivineGuardBlocked(true);
    EXPECT_EQ(BtActStatus.action_on, expanded::kGuardAction);
    EXPECT_EQ(BtActStatus.action_no, expanded::kGuardHold);
    EXPECT_EQ(BtActStatus.motion_no, expanded::kGuardHold);
    EXPECT_EQ(BtActStatus.guard_mode, expanded::kGuardHeld);

    // An unguarded hit, or another sword, gets the game's own reaction.
    BtActStatus = {};
    expanded::DivineGuardBlocked(false);
    EXPECT_EQ(BtActStatus.action_on, 0);
    expanded::test::Wielding dark_cloud(ITEM_WEAPON_DARK_CLOUD);
    expanded::DivineGuardBlocked(true);
    EXPECT_EQ(BtActStatus.action_on, 0);
    BtActStatus = {};
}
