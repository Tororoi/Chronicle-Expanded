#include <gtest/gtest.h>

#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/toan/gladius.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// A kill of a kind in slot 0, its slot released as Step pays it.
struct Kill {
    expanded::test::TestFloor floor;

    explicit Kill(Monster kind) {
        floor.SetUpModel(0, kind);
        floor.Spawn(0, 0);
        floor.unit().monster[0].state = -1;
    }

    int Abs(int abs) { return expanded::MonsterKillAbs(floor.unit(), 0, abs); }
};

} // namespace

TEST(Gladius, AMasterJacketGivesFourTimesItsAbs) {
    expanded::test::Wielding gladius(ITEM_WEAPON_GLADIUS);
    EXPECT_EQ(Kill(Monster::MASTER_JACKET).Abs(5), 20);
    EXPECT_EQ(Kill(Monster::MASTER_JACKET_ENHANCED).Abs(9), 36);
    EXPECT_EQ(Kill(Monster::SKELETON_SOLDIER).Abs(5), 5);
}

TEST(Gladius, OnlyInHandOrInSuperStevesSphere) {
    {
        expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
        EXPECT_EQ(Kill(Monster::MASTER_JACKET).Abs(5), 5);
    }
    {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_GLADIUS);
        EXPECT_EQ(Kill(Monster::MASTER_JACKET).Abs(5), 20);
    }
    {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
        EXPECT_EQ(Kill(Monster::MASTER_JACKET).Abs(5), 5);
    }
}
