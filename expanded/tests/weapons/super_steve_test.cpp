#include <gtest/gtest.h>

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

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

// The Weapon Abilities option is on by default; off, nothing is wielded or channelled as Expanded's effects ask, and
// back on after.
TEST(SuperSteve, WeaponAbilitiesOffWieldNothing) {
    WEAPON_HAVE weapon = SuperSteve(ITEM_WEAPON_BASELARD);
    Wielding wielding(weapon);
    EXPECT_TRUE(WeaponAbilitiesOn());
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(kWeaponAbilities, false));
    EXPECT_FALSE(WeaponAbilitiesOn());
    EXPECT_EQ(HeldWeapon(), 0);
    EXPECT_EQ(SphereWeapon(weapon), 0);
    EXPECT_EQ(SuperSteveSphere(), 0);
    EXPECT_FALSE(WeaponWielded(ITEM_WEAPON_SUPER_STEVE));
    EXPECT_FALSE(WeaponActive(ITEM_WEAPON_BASELARD));
    ASSERT_TRUE(options.Set(kWeaponAbilities, true));
    EXPECT_EQ(HeldWeapon(), ITEM_WEAPON_SUPER_STEVE);
    EXPECT_TRUE(WeaponWielded(ITEM_WEAPON_SUPER_STEVE));
    EXPECT_TRUE(WeaponActive(ITEM_WEAPON_BASELARD));
}

// A record carries its own weapon's effect, or a Super Steve its sphere's; nothing with Weapon Abilities off.
TEST(SuperSteve, ARecordCarriesItsWeaponOrItsSpheres) {
    WEAPON_HAVE steve = SuperSteve(ITEM_WEAPON_MANEATER);
    WEAPON_HAVE sword = {};
    sword.item_no = ITEM_WEAPON_MANEATER;
    EXPECT_TRUE(CarriesWeapon(sword, ITEM_WEAPON_MANEATER));
    EXPECT_TRUE(CarriesWeapon(steve, ITEM_WEAPON_MANEATER));
    EXPECT_FALSE(CarriesWeapon(sword, ITEM_WEAPON_EVILCISE));
    EXPECT_FALSE(CarriesWeapon(steve, ITEM_WEAPON_BASELARD)); // its other attachment is no SynthSphere
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(kWeaponAbilities, false));
    EXPECT_FALSE(CarriesWeapon(sword, ITEM_WEAPON_MANEATER));
    EXPECT_FALSE(CarriesWeapon(steve, ITEM_WEAPON_MANEATER));
}

// A member's equipped record, none for a member or a slot out of range.
TEST(SuperSteve, AnEquippedWeaponIsTheMembersSlot) {
    expanded::test::TestFloor floor;
    CUserStatus              &status = floor.status();
    status.equipped_weapon_slot[CHARA_XIAO] = 3;
    EXPECT_EQ(EquippedWeapon(status, CHARA_XIAO), &status.chara_weapons[CHARA_XIAO][3]);
    EXPECT_EQ(EquippedWeapon(status, 6), nullptr);
    EXPECT_EQ(EquippedWeapon(status, -1), nullptr);
    status.equipped_weapon_slot[CHARA_XIAO] = 11;
    EXPECT_EQ(EquippedWeapon(status, CHARA_XIAO), nullptr);
}
