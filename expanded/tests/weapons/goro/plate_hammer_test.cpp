#include <gtest/gtest.h>

#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/goro/plate_hammer.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "menu_save.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;

// weapon_no (carrying sphere when given) as chara's equipped record, max WHP max and whp left, with Auto Repair Powder
// (three) in the first quick slot.
struct Breaking {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding;

    Breaking(int chara, int weapon_no, int max, float whp, int sphere = 0) : wielding(weapon_no, sphere) {
        CUserStatus &status = floor.status();
        status.cur_chara = static_cast<s8>(chara);
        status.equipped_weapon_slot[chara] = 1;
        WEAPON_HAVE &record = Record();
        record = wielding.weapon();
        record.durability = static_cast<s16>(max);
        record.durability_f = whp;
        wielding.weapon().durability = static_cast<s16>(max);
        status.item_pack.quick_item_slot[0] = ITEM_AUTO_REPAIR_POWDER;
        status.item_pack.quick_item_qty[0] = 3;
    }

    WEAPON_HAVE &Record() {
        CUserStatus &status = floor.status();
        return status.chara_weapons[status.cur_chara][1];
    }
};

} // namespace

TEST(PlateHammer, AutoRepairTempersItOnePoint) {
    Breaking hammer(CHARA_GORO, ITEM_WEAPON_PLATE_HAMMER, 40, 0.5f);
    EXPECT_EQ(BattleSubWeaponDmg(10.0f, 0), 0);
    EXPECT_EQ(hammer.Record().durability, 41);
    EXPECT_FLOAT_EQ(hammer.Record().durability_f, 41.0f); // filled to its new max
    EXPECT_EQ(hammer.wielding.weapon().durability, 41);
    EXPECT_EQ(hammer.floor.status().item_pack.quick_item_qty[0], 2); // a powder spent

    // Broken and mended again: one more.
    hammer.Record().durability_f = 0.5f;
    BattleSubWeaponDmg(10.0f, 0);
    EXPECT_EQ(hammer.Record().durability, 42);
}

TEST(PlateHammer, WearShortOfBreakingTempersNothing) {
    Breaking hammer(CHARA_GORO, ITEM_WEAPON_PLATE_HAMMER, 40, 30.0f);
    BattleSubWeaponDmg(1.0f, 0);
    EXPECT_EQ(hammer.Record().durability, 40);
    EXPECT_EQ(hammer.floor.status().item_pack.quick_item_qty[0], 3);
}

TEST(PlateHammer, TemperingStopsAtNinetyNine) {
    Breaking hammer(CHARA_GORO, ITEM_WEAPON_PLATE_HAMMER, expanded::kTemperingMost, 0.5f);
    BattleSubWeaponDmg(10.0f, 0);
    EXPECT_EQ(hammer.Record().durability, expanded::kTemperingMost);
    EXPECT_FLOAT_EQ(hammer.Record().durability_f, static_cast<float>(expanded::kTemperingMost));
}

TEST(PlateHammer, OtherHammersAreMendedAsTheyWere) {
    Breaking hammer(CHARA_GORO, ITEM_WEAPON_TRIAL_HAMMER, 40, 0.5f);
    BattleSubWeaponDmg(10.0f, 0);
    EXPECT_EQ(hammer.Record().durability, 40);
    EXPECT_FLOAT_EQ(hammer.Record().durability_f, 40.0f);
}

TEST(PlateHammer, SuperSteveCarryingItsSphereIsTempered) {
    Breaking steve(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, 40, 0.5f, ITEM_WEAPON_PLATE_HAMMER);
    BattleSubWeaponDmg(10.0f, 0);
    EXPECT_EQ(steve.Record().durability, 41);
}

TEST(PlateHammer, OffWithTheWeaponAbilities) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Breaking hammer(CHARA_GORO, ITEM_WEAPON_PLATE_HAMMER, 40, 0.5f);
    BattleSubWeaponDmg(10.0f, 0);
    EXPECT_EQ(hammer.Record().durability, 40);
    EXPECT_FLOAT_EQ(hammer.Record().durability_f, 40.0f);
}
