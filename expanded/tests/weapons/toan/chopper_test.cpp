#include <gtest/gtest.h>

#include <initializer_list>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/toan/chopper.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// One monster of each kind in its own slot, Toan the one swinging.
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

    CMonstorUnit &unit() { return floor.unit(); }

    // A hit of the active character's weapon on monster.
    float Hit(int monster, float damage, bool weapon = true) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = floor.status().cur_chara;
        hit.vs_monster = weapon ? table : nullptr;
        return expanded::MonsterWeaponDamage(unit(), monster, hit, damage);
    }
};

} // namespace

TEST(Chopper, BeastsPlantsGyonsAndTheOparAreOrganic) {
    Floor floor({Monster::WEREWOLF, Monster::CANNIBAL_PLANT, Monster::GYON, Monster::SPACE_GYON_ENHANCED, Monster::OPAR, Monster::GUNNY,
                 Monster::CRABBY_HERMIT, Monster::MUMMY, Monster::STATUE});
    for (int organic = 0; organic < 5; organic++) {
        EXPECT_TRUE(expanded::Organic(floor.unit(), organic)) << organic;
    }
    for (int other = 5; other < 9; other++) {
        EXPECT_FALSE(expanded::Organic(floor.unit(), other)) << other;
    }
}

TEST(Chopper, ButchersEdgeRaisesDamageAndAbsOnOrganicAlone) {
    Floor                    floor({Monster::WEREWOLF, Monster::GUNNY});
    expanded::test::Wielding chopper(ITEM_WEAPON_CHOPPER);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 45.0f);
    EXPECT_FLOAT_EQ(floor.Hit(1, 30.0f), 30.0f);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f, false), 30.0f); // not a weapon hit (an item, a monster)
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 0, 6), 9);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 0, 3), 5); // 4.5, rounded
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 1, 6), 6);
}

TEST(Chopper, ButchersEdgeNeedsTheChopper) {
    Floor                    floor({Monster::WEREWOLF});
    expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 30.0f);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 0, 6), 6);
}

TEST(Chopper, SuperSteveCarriesButchersEdge) {
    Floor floor({Monster::CANNIBAL_PLANT});
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CHOPPER);
    EXPECT_FLOAT_EQ(floor.Hit(0, 10.0f), 15.0f);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 0, 4), 6);
}
