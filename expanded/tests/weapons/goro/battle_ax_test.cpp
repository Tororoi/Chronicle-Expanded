#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/goro/battle_ax.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;
using expanded::test::OptionScope;

namespace {

// A Cannibal Plant (plant) in slot 0 and a Werewolf (beast) in slot 1.
struct Floor {
    expanded::test::TestFloor floor;

    explicit Floor(int chara = CHARA_GORO) {
        floor.SetUpModel(0, Monster::CANNIBAL_PLANT);
        floor.SetUpModel(1, Monster::WEREWOLF);
        floor.Spawn(0, 0);
        floor.Spawn(1, 1);
        floor.status().cur_chara = chara;
    }

    // A hit of the active character's weapon on monster.
    float Hit(int monster, float damage, bool weapon = true) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = floor.status().cur_chara;
        hit.vs_monster = weapon ? table : nullptr;
        return expanded::MonsterWeaponDamage(floor.unit(), monster, hit, damage);
    }

    int Abs(int monster, int abs) { return expanded::MonsterKillAbs(floor.unit(), monster, abs); }
};

} // namespace

TEST(BattleAx, WoodcutterDoublesDamageAndAbsOnPlantsAlone) {
    Floor                    floor;
    expanded::test::Wielding ax(ITEM_WEAPON_BATTLE_AX);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 60.0f);
    EXPECT_FLOAT_EQ(floor.Hit(1, 30.0f), 30.0f);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f, false), 30.0f); // not a weapon hit (an item, a monster)
    EXPECT_EQ(floor.Abs(0, 7), 14);
    EXPECT_EQ(floor.Abs(1, 7), 7);
}

TEST(BattleAx, AnotherHammerFellsNothing) {
    Floor                    floor;
    expanded::test::Wielding hammer(ITEM_WEAPON_TURTLE_SHELL);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 30.0f);
    EXPECT_EQ(floor.Abs(0, 7), 7);
}

TEST(BattleAx, SuperSteveCarriesWoodcutter) {
    Floor                    floor(CHARA_XIAO);
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BATTLE_AX);
    EXPECT_FLOAT_EQ(floor.Hit(0, 10.0f), 20.0f);
    EXPECT_EQ(floor.Abs(0, 3), 6);
}

TEST(BattleAx, OffWithTheWeaponAbilities) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Floor                    floor;
    expanded::test::Wielding ax(ITEM_WEAPON_BATTLE_AX);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 30.0f);
    EXPECT_EQ(floor.Abs(0, 7), 7);
}
