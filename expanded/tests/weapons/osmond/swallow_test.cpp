#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/kick_back.hpp"
#include "expanded/weapons/osmond/swallow.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;
using expanded::test::Wielding;

namespace {

// A Cave Bat (sky) in slot 0 and a Werewolf (beast) in slot 1, chara the one firing.
struct Floor {
    expanded::test::TestFloor floor;

    explicit Floor(int chara = CHARA_OSMOND) {
        floor.SetUpModel(0, Monster::CAVE_BAT);
        floor.SetUpModel(1, Monster::WEREWOLF);
        floor.Spawn(0, 0);
        floor.Spawn(1, 1);
        floor.status().cur_chara = chara;
    }

    float Hit(int monster, float damage, int owner = CHARA_OSMOND) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = owner;
        hit.vs_monster = table;
        return expanded::MonsterWeaponDamage(floor.unit(), monster, hit, damage);
    }
};

COLLISION_HIT Shot(int owner, int attack) {
    COLLISION_HIT record = {};
    record.owner = owner;
    record.attack_no = attack;
    return record;
}

} // namespace

TEST(Swallow, WingClipperDoublesDamageAndAbsOnFlyersAlone) {
    Floor    floor;
    Wielding gun(ITEM_WEAPON_SWALLOW);
    EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 60.0f);
    EXPECT_FLOAT_EQ(floor.Hit(1, 30.0f), 30.0f);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.floor.unit(), 0, 5), 10);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.floor.unit(), 1, 5), 5);
}

TEST(Swallow, WingClipperStaysWithTheSwallow) {
    Floor floor;
    for (int weapon : {ITEM_WEAPON_JACKAL, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_STAR_BREAKER}) {
        Wielding gun(weapon);
        EXPECT_FLOAT_EQ(floor.Hit(0, 30.0f), 30.0f) << weapon;
    }
}

TEST(Swallow, SuperSteveCarriesWingClipper) {
    Floor    floor(CHARA_XIAO);
    Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SWALLOW);
    EXPECT_FLOAT_EQ(floor.Hit(0, 10.0f, CHARA_XIAO), 20.0f);
}

// A tenth as far as the Baselard's throw, stepping as a monster's knockback does.
TEST(Swallow, StoppingPowerThrowsATenthOfTheBaselard) {
    auto carry = [](expanded::KickBack kick) { return kick.speed * kick.speed / (2 * kick.decay); };
    EXPECT_NEAR(carry(expanded::kLightKickBack), carry(expanded::kHeavyKickBack) / 10, 0.01f);
    EXPECT_FLOAT_EQ(expanded::kLightKickBack.decay, expanded::kHeavyKickBack.decay);
}

TEST(Swallow, StoppingPowerDownTheLine) {
    for (int weapon : {ITEM_WEAPON_SWALLOW, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_STAR_BREAKER}) {
        Wielding gun(weapon);
        EXPECT_TRUE(expanded::StoppingPowerHit(Shot(CHARA_OSMOND, 6))) << weapon;
        EXPECT_FALSE(expanded::StoppingPowerHit(Shot(CHARA_TOAN, 0))) << weapon;
    }
    for (int weapon : {ITEM_WEAPON_MACHINE_GUN, ITEM_WEAPON_JACKAL, ITEM_WEAPON_SNAIL}) {
        Wielding gun(weapon);
        EXPECT_FALSE(expanded::StoppingPowerHit(Shot(CHARA_OSMOND, 6))) << weapon;
    }
}

TEST(Swallow, SuperStevesPelletsWithItsSpheresThrow) {
    for (int sphere : {ITEM_WEAPON_SWALLOW, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_STAR_BREAKER}) {
        Wielding steve(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_TRUE(expanded::StoppingPowerHit(Shot(CHARA_XIAO, 0))) << sphere;
        EXPECT_FALSE(expanded::StoppingPowerHit(Shot(CHARA_XIAO, 3))) << sphere; // not a pellet
    }
    Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_JACKAL);
    EXPECT_FALSE(expanded::StoppingPowerHit(Shot(CHARA_XIAO, 0)));
}

// A landed hit sets the monster's throw from the hit's point, scaled by its own knockback scale.
TEST(Swallow, ALandedShotThrowsTheMonster) {
    Floor    floor;
    Wielding gun(ITEM_WEAPON_SWALLOW);
    MONSTOR &bat = floor.floor.unit().monster[0];
    bat.knockback[2] = 1.0f;
    COLLISION_HIT shot = Shot(CHARA_OSMOND, 6);
    shot.pos[2] = -5.0f;
    expanded::MonsterKnockedBack(floor.floor.unit(), 0, shot);
    EXPECT_FLOAT_EQ(bat.knockback[0], expanded::kLightKickBack.speed);
    EXPECT_FLOAT_EQ(bat.knockback[1], expanded::kLightKickBack.decay);
}
