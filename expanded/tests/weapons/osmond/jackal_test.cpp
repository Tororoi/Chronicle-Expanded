#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/osmond/jackal.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;
using expanded::test::Wielding;

namespace {

// A Werewolf (beast) in slot 0 and a Mummy (undead) in slot 1, chara the one firing.
struct Floor {
    expanded::test::TestFloor floor;

    explicit Floor(int chara = CHARA_OSMOND) {
        floor.SetUpModel(0, Monster::WEREWOLF);
        floor.SetUpModel(1, Monster::MUMMY);
        floor.Spawn(0, 0);
        floor.Spawn(1, 1);
        floor.status().cur_chara = chara;
    }

    // A hit of owner's weapon on monster.
    float Hit(int monster, float damage, int owner = CHARA_OSMOND, bool weapon = true) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = owner;
        hit.vs_monster = weapon ? table : nullptr;
        return expanded::MonsterWeaponDamage(floor.unit(), monster, hit, damage);
    }

    int Abs(int monster, int abs) { return expanded::MonsterKillAbs(floor.unit(), monster, abs); }
};

} // namespace

TEST(Jackal, PredatorDoublesDamageAndAbsOnBeastsAlone) {
    Floor    floor;
    Wielding gun(ITEM_WEAPON_JACKAL);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 60.0f);
    EXPECT_FLOAT_EQ(floor.Hit(1, 30.0f), 30.0f);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f, CHARA_OSMOND, false), 30.0f); // not a weapon hit (an item, a monster)
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f, CHARA_TOAN), 30.0f);          // not the wielder's
    EXPECT_EQ(floor.Abs(0, 7), 14);
    EXPECT_EQ(floor.Abs(1, 7), 7);
}

TEST(Jackal, OtherGunsLeaveBeastsAsTheyAre) {
    Floor floor;
    for (int weapon : {ITEM_WEAPON_MACHINE_GUN, ITEM_WEAPON_SWALLOW, ITEM_WEAPON_G_CRUSHER}) {
        Wielding gun(weapon);
        EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 30.0f) << weapon;
        EXPECT_EQ(floor.Abs(0, 7), 7) << weapon;
    }
}

TEST(Jackal, SuperSteveCarriesPredator) {
    Floor    floor(CHARA_XIAO);
    Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_JACKAL);
    EXPECT_FLOAT_EQ(floor.Hit(0, 10.0f, CHARA_XIAO), 20.0f);
    EXPECT_EQ(floor.Abs(0, 3), 6);
}
