#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/toan/baselard.hpp"
#include "expanded/weapons/kick_back.hpp"
#include "itemdata.hpp"

using namespace expanded;

namespace {

// Points NowWeaponHave at a weapon for one case.
struct Wielding {
    WEAPON_HAVE *saved = NowWeaponHave;
    explicit Wielding(WEAPON_HAVE &weapon) { NowWeaponHave = &weapon; }
    ~Wielding() { NowWeaponHave = saved; }
};

COLLISION_HIT Hit(int owner, int attack) {
    COLLISION_HIT record = {};
    record.owner = owner;
    record.attack_no = attack;
    return record;
}

} // namespace

// The throw carries about 17 units, stepping as a monster's knockback does.
TEST(Baselard, HeavyKickBackCarriesAbout17Units) {
    float distance = 0;

    for (float speed = kHeavyKickBack.speed; speed > 0; speed -= kHeavyKickBack.decay) {
        distance += speed;
    }

    EXPECT_NEAR(kHeavyKickBack.speed * kHeavyKickBack.speed / (2 * kHeavyKickBack.decay), 17.0f, 0.1f);
    EXPECT_NEAR(distance, 17.0f, 1.5f);
}

// Toan's swings throw while he wields the Baselard; nobody else's hits do.
TEST(Baselard, ToansSwingsWithItThrow) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = ITEM_WEAPON_BASELARD;
    Wielding wielding(weapon);

    EXPECT_TRUE(HeavyHandHit(Hit(0, 0)));
    EXPECT_TRUE(HeavyHandHit(Hit(0, 3)));
    EXPECT_FALSE(HeavyHandHit(Hit(1, 0)));
    EXPECT_FALSE(HeavyHandHit(Hit(-1, -1)));

    weapon.item_no = ITEM_WEAPON_BASELARD + 1;
    EXPECT_FALSE(HeavyHandHit(Hit(0, 0)));
}

// Super Steve's pellets throw with a Baselard sphere attached.
TEST(Baselard, SuperStevesPelletsWithItsSphereThrow) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = ITEM_WEAPON_SUPER_STEVE;
    weapon.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    weapon.attach[0].sphere_weapon_no = ITEM_WEAPON_BASELARD;
    Wielding wielding(weapon);

    EXPECT_TRUE(HeavyHandHit(Hit(1, 0)));
    EXPECT_FALSE(HeavyHandHit(Hit(0, 0)));

    weapon.attach[0].sphere_weapon_no = ITEM_WEAPON_DAGGER;
    EXPECT_FALSE(HeavyHandHit(Hit(1, 0)));
}
