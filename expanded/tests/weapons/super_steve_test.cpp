#include <gtest/gtest.h>

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

using namespace expanded;

namespace {

// Super Steve with a SynthSphere of sphere_weapon in its second hole, behind an attack gem.
WEAPON_HAVE SuperSteve(int sphere_weapon) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = ITEM_WEAPON_SUPER_STEVE;
    weapon.attach[0].item_no = ITEM_ATTACH_START;
    weapon.attach[0].sphere_weapon_no = ITEM_WEAPON_BASELARD;
    weapon.attach[1].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    weapon.attach[1].sphere_weapon_no = sphere_weapon;
    return weapon;
}

// Points NowWeaponHave at a weapon for one case.
struct Wielding {
    WEAPON_HAVE *saved = NowWeaponHave;
    explicit Wielding(WEAPON_HAVE &weapon) { NowWeaponHave = &weapon; }
    ~Wielding() { NowWeaponHave = saved; }
};

} // namespace

// The sphere's weapon comes from the SynthSphere's entry, not any other attachment's.
TEST(SuperSteve, SphereWeaponIsTheSynthSpheres) {
    WEAPON_HAVE weapon = SuperSteve(ITEM_WEAPON_MATADOR);
    EXPECT_EQ(SphereWeapon(weapon), ITEM_WEAPON_MATADOR);

    weapon.attach[1].item_no = 0;
    EXPECT_EQ(SphereWeapon(weapon), 0);
}

// Super Steve's sphere makes its weapon active, but not wielded; another weapon's sphere does nothing.
TEST(SuperSteve, OnlySuperSteveChannelsItsSphere) {
    WEAPON_HAVE weapon = SuperSteve(ITEM_WEAPON_BASELARD);
    Wielding wielding(weapon);

    EXPECT_EQ(SuperSteveSphere(), ITEM_WEAPON_BASELARD);
    EXPECT_TRUE(WeaponActive(ITEM_WEAPON_BASELARD));
    EXPECT_FALSE(WeaponWielded(ITEM_WEAPON_BASELARD));
    EXPECT_TRUE(WeaponActive(ITEM_WEAPON_SUPER_STEVE));
    EXPECT_FALSE(WeaponActive(0));

    weapon.item_no = ITEM_WEAPON_ANGE_GEAR;
    EXPECT_EQ(SuperSteveSphere(), 0);
    EXPECT_FALSE(WeaponActive(ITEM_WEAPON_BASELARD));
}

// A slingshot's sphere lends its pellet; any other sphere leaves Super Steve's.
TEST(SuperSteve, SlingshotSphereLendsItsPellet) {
    WEAPON_HAVE weapon = SuperSteve(ITEM_WEAPON_MATADOR);
    Wielding wielding(weapon);
    EXPECT_EQ(PelletWeapon(), ITEM_WEAPON_MATADOR);

    weapon.attach[1].sphere_weapon_no = ITEM_WEAPON_ANGE_GEAR;
    EXPECT_EQ(PelletWeapon(), ITEM_WEAPON_ANGE_GEAR);

    weapon.attach[1].sphere_weapon_no = ITEM_WEAPON_BASELARD;
    EXPECT_EQ(PelletWeapon(), ITEM_WEAPON_SUPER_STEVE);
}
