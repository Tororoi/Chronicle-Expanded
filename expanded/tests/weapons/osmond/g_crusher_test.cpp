#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/osmond/g_crusher.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;
using expanded::test::Wielding;

namespace {

// Osmond (or chara) out, with a Werewolf in slot 0, a Golem in 1, a Rockanoff in 2, a Cannibal Plant in 3 and an Opar in 4.
struct Floor {
    expanded::test::TestFloor floor;

    explicit Floor(int chara = CHARA_OSMOND) {
        const Monster kinds[] = {Monster::WEREWOLF, Monster::GOLEM, Monster::ROCKANOFF, Monster::CANNIBAL_PLANT, Monster::OPAR};
        for (int i = 0; i < 5; i++) {
            floor.SetUpModel(i, kinds[i]);
            floor.Spawn(i, i);
        }
        floor.status().cur_chara = chara;
    }

    float Hit(int monster, float damage, int owner = CHARA_OSMOND) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = owner;
        hit.vs_monster = table;
        return expanded::MonsterWeaponDamage(floor.unit(), monster, hit, damage);
    }

    // Of hits fresh hits of owner's, how many break through the guard window.
    int Broken(int hits, int owner = CHARA_OSMOND) {
        int broken = 0;
        for (int i = 0; i < hits; i++) {
            COLLISION_HIT hit = {};
            hit.owner = owner;
            broken += expanded::MonsterGuardBroken(floor.unit(), 0, hit) ? 1 : 0;
        }
        return broken;
    }
};

} // namespace

TEST(GCrusher, GuardCrusherBreaksOneGuardInTen) {
    for (int weapon : {ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_STAR_BREAKER}) {
        Floor    floor;
        Wielding gun(weapon);
        expanded::SeedModRandom(7);
        int broken = floor.Broken(4000);
        EXPECT_GT(broken, 330) << weapon;
        EXPECT_LT(broken, 470) << weapon;
        EXPECT_EQ(floor.Broken(100, CHARA_TOAN), 0) << weapon; // not the wielder's
    }
    for (int weapon : {ITEM_WEAPON_SWALLOW, ITEM_WEAPON_JACKAL}) {
        Floor    floor;
        Wielding gun(weapon);
        EXPECT_EQ(floor.Broken(1000), 0) << weapon;
    }
}

// A hit rolls once: every window or sphere it meets after answers as the first did.
TEST(GCrusher, AHitRollsOnce) {
    Floor    floor;
    Wielding gun(ITEM_WEAPON_G_CRUSHER);
    expanded::SeedModRandom(3);
    for (int i = 0; i < 200; i++) {
        COLLISION_HIT hit = {};
        hit.owner = CHARA_OSMOND;
        bool first = expanded::MonsterGuardBroken(floor.floor.unit(), 0, hit);
        EXPECT_NE(hit.weapon_flags & expanded::kGuardRolledMark, 0);
        for (int again = 0; again < 5; again++) {
            EXPECT_EQ(expanded::MonsterGuardBroken(floor.floor.unit(), 0, hit), first);
        }
    }
}

TEST(GCrusher, SuperSteveCarriesGuardCrusher) {
    for (int sphere : {ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_STAR_BREAKER}) {
        Floor    floor(CHARA_XIAO);
        Wielding steve(ITEM_WEAPON_SUPER_STEVE, sphere);
        expanded::SeedModRandom(7);
        EXPECT_GT(floor.Broken(1000, CHARA_XIAO), 60) << sphere;
    }
}

// Up to twice by how little a hit throws the monster; plants are not heavy.
TEST(GCrusher, GiantCrusherByWeight) {
    Floor    floor;
    Wielding gun(ITEM_WEAPON_G_CRUSHER);
    EXPECT_FLOAT_EQ(floor.Hit(0, 40.0f), 40.0f); // Werewolf, thrown fully
    EXPECT_FLOAT_EQ(floor.Hit(1, 40.0f), 60.0f); // Golem, half
    EXPECT_FLOAT_EQ(floor.Hit(2, 40.0f), 48.0f); // Rockanoff, 0.8
    EXPECT_FLOAT_EQ(floor.Hit(3, 40.0f), 40.0f); // Cannibal Plant, rooted
    EXPECT_FLOAT_EQ(floor.Hit(4, 40.0f), 80.0f); // Opar, never moved
    EXPECT_FLOAT_EQ(floor.Hit(1, 40.0f, CHARA_TOAN), 40.0f);
}

TEST(GCrusher, GiantCrusherStaysWithTheGCrusher) {
    Floor floor;
    for (int weapon : {ITEM_WEAPON_SWALLOW, ITEM_WEAPON_STAR_BREAKER}) {
        Wielding gun(weapon);
        EXPECT_FLOAT_EQ(floor.Hit(1, 40.0f), 40.0f) << weapon;
    }
    Floor    xiao(CHARA_XIAO);
    Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_G_CRUSHER);
    EXPECT_FLOAT_EQ(xiao.Hit(1, 40.0f, CHARA_XIAO), 60.0f);
}
