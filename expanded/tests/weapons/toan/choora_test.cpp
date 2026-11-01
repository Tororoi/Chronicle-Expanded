#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/toan/choora.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// A monster of each kind in its own slot, Toan the one swinging.
struct Floor {
    expanded::test::TestFloor floor;

    explicit Floor(std::initializer_list<Monster> kinds) {
        int slot = 0;
        for (Monster kind : kinds) {
            floor.SetUpModel(slot, kind);
            floor.Spawn(slot, slot);
            slot++;
        }
        floor.status().cur_chara = CHARA_TOAN;
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

TEST(Choora, ArmorPiercerDoublesDamageAndAbsOnArmoredMonsters) {
    Floor                    floor({Monster::CRABBY_HERMIT, Monster::LIVING_ARMOR_ENHANCED, Monster::BLACK_KNIGHT, Monster::BLACK_KNIGHT_MOUNT,
                                    Monster::ARTHUR, Monster::GOL_ENHANCED, Monster::BLIZZARD, Monster::NIKAPOUS, Monster::WEREWOLF});
    expanded::test::Wielding choora(ITEM_WEAPON_CHOORA);
    for (int armored = 0; armored < 8; armored++) {
        EXPECT_FLOAT_EQ(floor.Hit(armored, 30.0f), 60.0f) << armored;
        EXPECT_EQ(floor.Abs(armored, 7), 14) << armored;
    }
    EXPECT_FLOAT_EQ(floor.Hit(8, 30.0f), 30.0f);
    EXPECT_EQ(floor.Abs(8, 7), 7);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f, false), 30.0f); // not a weapon hit (an item, a monster)
}

// The Stone monsters left out have no armor to pierce.
TEST(Choora, NotEveryStoneMonster) {
    Floor                    floor({Monster::DUNE, Monster::VULCAN, Monster::ROCKANOFF});
    expanded::test::Wielding choora(ITEM_WEAPON_CHOORA);
    for (int monster = 0; monster < 3; monster++) {
        EXPECT_FLOAT_EQ(floor.Hit(monster, 30.0f), 30.0f) << monster;
    }
}

TEST(Choora, OnlyInHand) {
    Floor                    floor({Monster::LIVING_ARMOR});
    expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 30.0f);
    EXPECT_EQ(floor.Abs(0, 7), 7);
}

TEST(Choora, SuperSteveCarriesArmorPiercer) {
    Floor floor({Monster::LIVING_ARMOR});
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CHOORA);
    EXPECT_FLOAT_EQ(floor.Hit(0, 10.0f), 20.0f);
    EXPECT_EQ(floor.Abs(0, 3), 6);
}
