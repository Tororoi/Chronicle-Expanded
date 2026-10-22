#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/revive.hpp"
#include "expanded/weapons/toan/cross_hinder.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// A Mummy (undead) in slot 0 and a Werewolf (beast) in slot 1, Toan the one swinging.
struct Floor {
    expanded::test::TestFloor floor;

    Floor() {
        floor.SetUpModel(0, Monster::MUMMY);
        floor.SetUpModel(1, Monster::WEREWOLF);
        floor.Spawn(0, 0);
        floor.Spawn(1, 1);
        floor.status().cur_chara = CHARA_TOAN;
    }

    // A hit of the active character's weapon on monster.
    float Hit(int monster, float damage, bool weapon = true) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = CHARA_TOAN;
        hit.vs_monster = weapon ? table : nullptr;
        return expanded::MonsterWeaponDamage(floor.unit(), monster, hit, damage);
    }
};

} // namespace

TEST(CrossHinder, SanctifierDoublesDamageAndAbsOnUndeadAlone) {
    Floor                    floor;
    expanded::test::Wielding sword(ITEM_WEAPON_CROSS_HINDER);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 60.0f);
    EXPECT_FLOAT_EQ(floor.Hit(1, 30.0f), 30.0f);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f, false), 30.0f); // not a weapon hit (an item, a monster)
    EXPECT_EQ(expanded::MonsterKillAbs(floor.floor.unit(), 0, 7), 14);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.floor.unit(), 1, 7), 7);
}

TEST(CrossHinder, SanctifiedUndeadStayDown) {
    Floor floor;
    {
        expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
        EXPECT_FALSE(expanded::StaysDown(floor.floor.unit(), 0));
    }
    expanded::test::Wielding sword(ITEM_WEAPON_CROSS_HINDER);
    EXPECT_TRUE(expanded::StaysDown(floor.floor.unit(), 0));
    EXPECT_FALSE(expanded::StaysDown(floor.floor.unit(), 1));
}

TEST(CrossHinder, SuperSteveCarriesSanctifier) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CROSS_HINDER);
    COLLISION_HIT            hit = {};
    char                     table[10] = {};
    hit.owner = CHARA_XIAO;
    hit.vs_monster = table;
    EXPECT_FLOAT_EQ(expanded::MonsterWeaponDamage(floor.floor.unit(), 0, hit, 10.0f), 20.0f);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.floor.unit(), 0, 3), 6);
}

TEST(CrossHinder, HolyReachDoublesTheLockOnRangeDownTheLine) {
    expanded::test::TestFloor floor;
    for (int weapon : {ITEM_WEAPON_CROSS_HINDER, ITEM_WEAPON_BIG_BANG, ITEM_WEAPON_SWORD_OF_ZEUS}) {
        expanded::test::Wielding sword(weapon);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.2f), 2.4f) << weapon;
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, weapon);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.4f), 2.8f) << weapon;
    }
    expanded::test::Wielding sun(ITEM_WEAPON_SUN_SWORD);
    EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.2f), 1.2f);
}
