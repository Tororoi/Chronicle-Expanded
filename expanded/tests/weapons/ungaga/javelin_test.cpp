#include <gtest/gtest.h>

#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/ungaga/javelin.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::Monster;

namespace {

// A Gunny (marine) in slot 0 and a Werewolf (beast) in slot 1, Ungaga out.
struct Floor {
    expanded::test::TestFloor floor;

    Floor() {
        floor.SetUpModel(0, Monster::GUNNY);
        floor.SetUpModel(1, Monster::WEREWOLF);
        floor.Spawn(0, 0);
        floor.Spawn(1, 1);
        floor.status().cur_chara = CHARA_UNGAGA;
    }

    CMonstorUnit &unit() { return floor.unit(); }
};

} // namespace

TEST(Javelin, MarineMonstersLoseTheirDefenseAndHardness) {
    Floor floor;
    ASSERT_GT(floor.unit().monster[0].defense, 0);
    {
        expanded::test::Wielding halberd(ITEM_WEAPON_HALBERT);
        EXPECT_EQ(expanded::MonsterDefense(floor.unit(), 0, 12), 12);
        EXPECT_EQ(expanded::MonsterHardness(floor.unit(), 0, 3), 3);
    }
    expanded::test::Wielding javelin(ITEM_WEAPON_JAVELIN);
    EXPECT_EQ(expanded::MonsterDefense(floor.unit(), 0, 12), 0);
    EXPECT_EQ(expanded::MonsterHardness(floor.unit(), 0, 3), 0);
    EXPECT_EQ(expanded::MonsterDefense(floor.unit(), 1, 12), 12);
    EXPECT_EQ(expanded::MonsterHardness(floor.unit(), 1, 3), 3);
}

TEST(Javelin, MarineKillsGiveDoubleAbs) {
    Floor floor;
    {
        expanded::test::Wielding halberd(ITEM_WEAPON_HALBERT);
        EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 0, 7), 7);
    }
    expanded::test::Wielding javelin(ITEM_WEAPON_JAVELIN);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 0, 7), 14);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 1, 7), 7);
}

TEST(Javelin, SuperSteveCarriesMarineHunter) {
    Floor floor;
    floor.floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_JAVELIN);
    EXPECT_EQ(expanded::MonsterDefense(floor.unit(), 0, 12), 0);
    EXPECT_EQ(expanded::MonsterHardness(floor.unit(), 0, 3), 0);
    EXPECT_EQ(expanded::MonsterKillAbs(floor.unit(), 0, 5), 10);
    EXPECT_EQ(expanded::MonsterDefense(floor.unit(), 1, 12), 12);
}
