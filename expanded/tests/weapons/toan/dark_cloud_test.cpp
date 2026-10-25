#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/toan/dark_cloud.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// Whether a guarding monster in slot 0 blocks a plain hit with weapon_no in hand (carrying sphere when given).
bool Blocks(int weapon_no, int sphere = 0) {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding(weapon_no, sphere);
    return expanded::MonsterGuardBlocks(floor.unit(), 0, 0, COLLISION_HIT{});
}

} // namespace

TEST(DarkCloud, ItsHitsPassEveryGuard) {
    EXPECT_TRUE(Blocks(ITEM_WEAPON_DAGGER));
    EXPECT_FALSE(Blocks(ITEM_WEAPON_DARK_CLOUD));
    EXPECT_FALSE(Blocks(ITEM_WEAPON_7THHEAVEN)); // which inherits it
}

TEST(DarkCloud, SuperStevesPelletsPassEveryGuardWithItsSphere) {
    EXPECT_FALSE(Blocks(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DARK_CLOUD));
    EXPECT_FALSE(Blocks(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_7THHEAVEN));
    EXPECT_TRUE(Blocks(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER));
}
