#include <gtest/gtest.h>

#include "expanded/weapons/toan/chronicle_two.hpp"
#include "itemdata.hpp"
#include "menu_dungeon.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"
#include "weaponlevelup.hpp"

namespace {

// A Baselard at its max attack and magic, a SynthSphere of attack and magic in its first slot and an attack attachment
// in its second.
WEAPON_HAVE Baselard(int sphere_attack, int sphere_magic) {
    const WEAPON_DATA *data = GetWeaponData(ITEM_WEAPON_BASELARD);
    WEAPON_HAVE        weapon = {};
    weapon.item_no = ITEM_WEAPON_BASELARD;
    weapon.attack = data->attack_max;
    weapon.magic = data->magic_max;
    weapon.endurance = weapon.speed = 10;
    weapon.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    weapon.attach[0].status[0] = static_cast<s16>(sphere_attack);
    weapon.attach[0].status[3] = static_cast<s16>(sphere_magic);
    weapon.attach[1].item_no = ITEM_ATTACH_ATTACK;
    weapon.attach[1].status[0] = 3;
    return weapon;
}

struct Owning {
    expanded::test::TestSave save;

    explicit Owning(bool chronicle_two) {
        if (chronicle_two) {
            save.status().chara_weapons[CHARA_TOAN][4].item_no = ITEM_WEAPON_CHRONICLE2;
        }
    }
};

} // namespace

TEST(ChronicleTwo, WithoutItTheMaxHolds) {
    Owning      owning(false);
    WEAPON_HAVE weapon = Baselard(20, 10);
    EXPECT_EQ(expanded::ChronicleTwoLimit(weapon, expanded::kStatAttack, 50), 50);
}

TEST(ChronicleTwo, OwnedTheAttachedSpheresGoPastTheMax) {
    Owning      owning(true);
    WEAPON_HAVE weapon = Baselard(20, 10);
    EXPECT_EQ(expanded::ChronicleTwoLimit(weapon, expanded::kStatAttack, 50), 70); // the attack attachment adds nothing past it
    EXPECT_EQ(expanded::ChronicleTwoLimit(weapon, expanded::kStatMagic, 40), 50);
    EXPECT_EQ(expanded::ChronicleTwoLimit(Baselard(990, 0), expanded::kStatAttack, 50), expanded::kStatMost);
}

TEST(ChronicleTwo, TheWeaponFightsWithIt) {
    const WEAPON_DATA *data = GetWeaponData(ITEM_WEAPON_BASELARD);
    WEAPON_HAVE        weapon = Baselard(20, 10);
    weapon.durability = 50;
    weapon.durability_f = 50.0f;
    WEAPON_HAVE fights = {};
    {
        Owning owning(false);
        WeaponAllValueSet(&weapon, &fights, 1);
        EXPECT_EQ(fights.attack, data->attack_max);
    }
    Owning owning(true);
    WeaponAllValueSet(&weapon, &fights, 1);
    EXPECT_EQ(fights.attack, data->attack_max + 20);
    EXPECT_EQ(fights.magic, data->magic_max + 10);
}

TEST(ChronicleTwo, ALevelUpFoldsTheSpheresInOnlyToTheMax) {
    const WEAPON_DATA *data = GetWeaponData(ITEM_WEAPON_BASELARD);
    Owning             owning(true);
    WEAPON_HAVE        weapon = Baselard(20, 10);
    WEAPON_HAVE        leveled = {};
    WeaponLevelUpValueCalc(&weapon, &leveled, 1, 0);
    EXPECT_EQ(leveled.attack, data->attack_max);
    EXPECT_EQ(leveled.magic, data->magic_max);
}
