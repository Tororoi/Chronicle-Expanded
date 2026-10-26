#include <gtest/gtest.h>

#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/ungaga/de_sanga.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

constexpr int kSlot = 3;

// A Gunny in slot 0, and chara's weapon_no in their bag's slot kSlot, equipped and in hand at whp of max_whp.
struct Floor {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding;

    Floor(int chara, int weapon_no, float whp, int max_whp = 45, int sphere = 0) : wielding(weapon_no, sphere) {
        floor.SetUpModel(0, Monster::GUNNY);
        floor.Spawn(0, 0);
        CUserStatus &status = floor.status();
        status.cur_chara = static_cast<s8>(chara);
        status.equipped_weapon_slot[chara] = kSlot;
        WEAPON_HAVE &record = Weapon();
        record = wielding.weapon();
        record.durability = static_cast<s16>(max_whp);
        record.durability_f = whp;
    }

    WEAPON_HAVE &Weapon() {
        CUserStatus &status = floor.status();
        int          chara = status.cur_chara;
        return status.chara_weapons[chara][kSlot];
    }

    // CheckDmg's death of the Gunny, the kill credited to killer.
    void Kill(int killer) {
        MONSTOR &gunny = floor.unit().monster[0];
        gunny.last_attacker = killer;
        gunny.hp = 0;
        expanded::MonsterKilled(floor.unit(), 0);
    }
};

} // namespace

TEST(DeSanga, EveryKillRestoresFiveWhp) {
    Floor floor(CHARA_UNGAGA, ITEM_WEAPON_DE_SANGA, 20.0f);
    floor.Kill(CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 25.0f);
    floor.Kill(CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 30.0f);
}

TEST(DeSanga, TheHealStopsAtTheMax) {
    Floor floor(CHARA_UNGAGA, ITEM_WEAPON_DE_SANGA, 42.5f);
    floor.Kill(CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 45.0f);
    floor.Kill(CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 45.0f);

    floor.Weapon().durability_f = 50.0f;
    floor.Kill(CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 50.0f);
}

TEST(DeSanga, OnlyTheActiveCharactersKillsHeal) {
    Floor floor(CHARA_UNGAGA, ITEM_WEAPON_DE_SANGA, 20.0f);
    floor.Kill(-1);
    floor.Kill(CHARA_TOAN);
    floor.Kill(CHARA_XIAO);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 20.0f);
}

TEST(DeSanga, OtherWeaponsDoNotHeal) {
    Floor floor(CHARA_UNGAGA, ITEM_WEAPON_HALBERT, 20.0f);
    floor.Kill(CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 20.0f);
}

TEST(DeSanga, SuperSteveCarriesVampire) {
    Floor floor(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, 10.0f, 60, ITEM_WEAPON_DE_SANGA);
    floor.Kill(CHARA_XIAO);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 15.0f);
    floor.Kill(CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 15.0f);
}

TEST(DeSanga, SuperSteveWithAnotherSphereDoesNotHeal) {
    Floor floor(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, 10.0f, 60, ITEM_WEAPON_JAVELIN);
    floor.Kill(CHARA_XIAO);
    EXPECT_FLOAT_EQ(floor.Weapon().durability_f, 10.0f);
}
