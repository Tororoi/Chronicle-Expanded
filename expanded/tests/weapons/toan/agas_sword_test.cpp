#include <gtest/gtest.h>

#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/toan/agas_sword.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// A character (Toan unless given) with weapon_no equipped in their first slot, carrying sphere's SynthSphere when one is given.
struct Equipped {
    expanded::test::TestFloor floor;

    explicit Equipped(int weapon_no, int sphere = 0, int chara = CHARA_TOAN) {
        CUserStatus &status = floor.status();
        status.equipped_weapon_slot[chara] = 0;
        WEAPON_HAVE &weapon = status.chara_weapons[chara][0];
        weapon = {};
        weapon.item_no = static_cast<s16>(weapon_no);
        if (sphere != 0) {
            weapon.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
            weapon.attach[0].sphere_weapon_no = static_cast<s16>(sphere);
        }
    }
};

} // namespace

TEST(AgasSword, GivesItsWielderFifteenDefense) {
    Equipped aga(ITEM_WEAPON_AGA_S_SWORD);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_TOAN, 10), 25);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_TOAN, 40), 55); // on top of what food has raised it to
    EXPECT_EQ(expanded::PlayerDefense(CHARA_XIAO, 10), 10);
}

TEST(AgasSword, OnlyEquippedOrInSuperStevesSphere) {
    {
        Equipped dagger(ITEM_WEAPON_DAGGER);
        EXPECT_EQ(expanded::PlayerDefense(CHARA_TOAN, 10), 10);
    }
    {
        Equipped steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_AGA_S_SWORD, CHARA_XIAO);
        EXPECT_EQ(expanded::PlayerDefense(CHARA_XIAO, 10), 25);
    }
    {
        Equipped steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER, CHARA_XIAO);
        EXPECT_EQ(expanded::PlayerDefense(CHARA_XIAO, 10), 10);
    }
}
