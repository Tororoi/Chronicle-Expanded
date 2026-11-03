#include <gtest/gtest.h>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/goro/turtle_shell.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "menu_save.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;

// chara (Goro unless given) with weapon_no equipped, carrying sphere's SynthSphere when one is given, at 40 WHP.
struct Equipped {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding;

    explicit Equipped(int weapon_no, int sphere = 0, int chara = CHARA_GORO, int flags = 0) : wielding(weapon_no, sphere) {
        CUserStatus &status = floor.status();
        status.cur_chara = static_cast<s8>(chara);
        status.equipped_weapon_slot[chara] = 1;
        wielding.weapon().flags = static_cast<u16>(flags);
        Record() = wielding.weapon();
        Record().durability = 40;
        Record().durability_f = 40.0f;
    }

    WEAPON_HAVE &Record() {
        CUserStatus &status = floor.status();
        return status.chara_weapons[status.cur_chara][1];
    }

    // The WHP a swing's wear of amount takes off.
    float Worn(float amount) {
        float was = Record().durability_f;
        BattleSubWeaponDmg(amount, 0);
        return was - Record().durability_f;
    }
};

} // namespace

TEST(TurtleShell, WearsHalfAsMuchAsAnotherHammer) {
    float hammer = Equipped(ITEM_WEAPON_TRIAL_HAMMER).Worn(2.0f);
    EXPECT_FLOAT_EQ(Equipped(ITEM_WEAPON_TURTLE_SHELL).Worn(2.0f), hammer * expanded::kHardShellWear);
}

TEST(TurtleShell, DurableAndFragileStillApplyOnTop) {
    float durable = Equipped(ITEM_WEAPON_TRIAL_HAMMER, 0, CHARA_GORO, WEAPONFLAG_DURABLE).Worn(2.0f);
    EXPECT_FLOAT_EQ(Equipped(ITEM_WEAPON_TURTLE_SHELL, 0, CHARA_GORO, WEAPONFLAG_DURABLE).Worn(2.0f), durable * expanded::kHardShellWear);
    float fragile = Equipped(ITEM_WEAPON_TRIAL_HAMMER, 0, CHARA_GORO, WEAPONFLAG_FRAGILE).Worn(2.0f);
    EXPECT_FLOAT_EQ(Equipped(ITEM_WEAPON_TURTLE_SHELL, 0, CHARA_GORO, WEAPONFLAG_FRAGILE).Worn(2.0f), fragile * expanded::kHardShellWear);
}

TEST(TurtleShell, GivesGoroFiveDefense) {
    Equipped shell(ITEM_WEAPON_TURTLE_SHELL);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 10), 15);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 40), 45); // on top of what food has raised it to
    EXPECT_EQ(expanded::PlayerDefense(CHARA_TOAN, 10), 10);
    Equipped hammer(ITEM_WEAPON_TRIAL_HAMMER);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 10), 10);
}

TEST(TurtleShell, SuperSteveCarryingItsSphereHasBoth) {
    float    steve = Equipped(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER, CHARA_XIAO).Worn(2.0f);
    Equipped shelled(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_TURTLE_SHELL, CHARA_XIAO);
    EXPECT_FLOAT_EQ(shelled.Worn(2.0f), steve * expanded::kHardShellWear);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_XIAO, 10), 15);
}

TEST(TurtleShell, OffWithTheWeaponAbilities) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    float    hammer = Equipped(ITEM_WEAPON_TRIAL_HAMMER).Worn(2.0f);
    Equipped shell(ITEM_WEAPON_TURTLE_SHELL);
    EXPECT_FLOAT_EQ(shell.Worn(2.0f), hammer);
    EXPECT_EQ(expanded::PlayerDefense(CHARA_GORO, 10), 10);
}
