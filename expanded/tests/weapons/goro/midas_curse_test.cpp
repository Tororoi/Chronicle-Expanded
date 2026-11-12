#include <gtest/gtest.h>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/goro/midas_curse.hpp"
#include "expanded/weapons/player_state.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::test::TestFloor;

namespace {

// Goro in play with weapon equipped, 100 life of 200; the curse and the gold put back after.
class MidasCurse : public ::testing::Test {
protected:
    void SetUp() override {
        status_.cur_chara = CHARA_GORO;
        status_.hp[CHARA_GORO] = 100;
        status_.max_hp[CHARA_GORO] = 200;
        expanded::MidasCurseReset();
        Equip(ITEM_WEAPON_BIG_BUCKS_HAMMER);
        expanded::CurseStep();
    }

    void TearDown() override {
        status_.equipped_weapon_slot[CHARA_GORO] = -1;
        status_.ailments[CHARA_GORO] = 0;
        expanded::CurseStep();
        expanded::MidasCurseReset();
    }

    void Equip(int weapon) {
        status_.equipped_weapon_slot[CHARA_GORO] = 0;
        status_.chara_weapons[CHARA_GORO][0] = {};
        status_.chara_weapons[CHARA_GORO][0].item_no = static_cast<s16>(weapon);
    }

    s32 &Ailments() { return status_.ailments[CHARA_GORO]; }

    // An item lifting the curse, and the step that notices.
    void Lift() {
        Ailments() &= ~AILMENT_CURSE;
        expanded::CurseStep();
        expanded::MidasCurseStep();
    }

    TestFloor    floor_;
    CUserStatus &status_ = floor_.status();
};

} // namespace

TEST_F(MidasCurse, BigBucksHammerCursesGoro) {
    EXPECT_TRUE(Ailments() & AILMENT_CURSE);
    EXPECT_TRUE(expanded::MidasCursed());

    Equip(ITEM_WEAPON_MAGICAL_HAMMER); // another: freed
    expanded::CurseStep();
    EXPECT_FALSE(Ailments() & AILMENT_CURSE);
    EXPECT_FALSE(expanded::MidasCursed());
}

TEST_F(MidasCurse, LiftingItTurnsGoroGoldAndFrozenWithNoEnd) {
    Lift();
    EXPECT_TRUE(expanded::GoroGold());
    EXPECT_EQ(Ailments(), AILMENT_FREEZE);
    EXPECT_EQ(status_.ailment_frames[CHARA_GORO], 0); // the game's countdown never ends it
    for (int i = 0; i < 10; i++) {
        expanded::CurseStep();
        expanded::MidasCurseStep();
    }
    EXPECT_TRUE(expanded::GoroGold());
    EXPECT_FALSE(Ailments() & AILMENT_CURSE); // not cursed again while gold
}

TEST_F(MidasCurse, HealingTheFreezeEndsTheGoldAndTheCurseStaysOffTheFloor) {
    Lift();
    Ailments() &= ~AILMENT_FREEZE; // an item heals it
    expanded::MidasCurseStep();
    expanded::CurseStep();
    EXPECT_FALSE(expanded::GoroGold());
    EXPECT_FALSE(Ailments() & AILMENT_CURSE);

    floor_.SetUpModel(0, expanded::Monster::CAVE_BAT); // the next floor
    expanded::CurseStep();
    EXPECT_TRUE(Ailments() & AILMENT_CURSE);
}

TEST_F(MidasCurse, TheHitThatWouldKnockHimOutLeavesOneLifeAndTurnsHim) {
    EXPECT_EQ(expanded::MidasPlayerDamage(40), 40); // not lethal: as it is
    EXPECT_FALSE(expanded::GoroGold());
    EXPECT_EQ(expanded::MidasPlayerDamage(150), 99);
    EXPECT_TRUE(expanded::GoroGold());
}

TEST_F(MidasCurse, NotCursedTheHitKnocksHimOut) {
    Equip(ITEM_WEAPON_MAGICAL_HAMMER);
    expanded::CurseStep();
    EXPECT_EQ(expanded::MidasPlayerDamage(150), 150);
    EXPECT_FALSE(expanded::GoroGold());
}

TEST_F(MidasCurse, GoldHeIsGuardedFromEveryHitAndAilment) {
    EXPECT_FALSE(expanded::MidasShrugs());
    Lift();
    EXPECT_TRUE(expanded::MidasShrugs());
    EXPECT_FALSE(expanded::MidasAllowsAilment());
    COLLISION_HIT hit{};
    hit.kind = expanded::kGuardBreakingHit;
    expanded::MidasPlayerHit(hit);
    EXPECT_EQ(hit.kind, expanded::kKnockbackHit); // never broken, as Divine Guard has it

    status_.cur_chara = CHARA_TOAN; // another in play: his own
    EXPECT_FALSE(expanded::MidasShrugs());
    EXPECT_TRUE(expanded::MidasAllowsAilment());
}

TEST_F(MidasCurse, GoldHeStandsAndTheMonsterThatStruckIsThrownBack) {
    floor_.SetUpModel(0, expanded::Monster::SKELETON_SOLDIER);
    floor_.Spawn(0, 0);
    CMonstorUnit &unit = floor_.unit();
    unit.monster[0].state = 2;
    unit.monster[0].knockback[0] = 0.0f;
    unit.monster[0].knockback[2] = 1.0f;
    expanded::CurseStep(); // cursed on the floor the spawn began
    Lift();
    ASSERT_TRUE(expanded::GoroGold());

    CharaMain.pos[0] = 0.0f;
    CharaMain.pos[2] = 0.0f;
    unit.chara[0][0].pos[0] = 20.0f; // in reach
    unit.chara[0][0].pos[2] = 0.0f;
    velo__2[0] = 3.0f; // the guard's push-back
    COLLISION_HIT hit{};
    hit.owner = 200; // monster 0's
    expanded::MidasPlayerReacted(hit);
    EXPECT_FLOAT_EQ(velo__2[0], 0.0f);
    EXPECT_FLOAT_EQ(unit.monster[0].knockback[0], expanded::kGoldRepelSpeed);
    EXPECT_FLOAT_EQ(unit.monster[0].knockback_direction[0], 1.0f); // away from him

    unit.monster[0].knockback[0] = 0.0f;
    unit.chara[0][0].pos[0] = 200.0f; // a shot from afar
    expanded::MidasPlayerReacted(hit);
    EXPECT_FLOAT_EQ(unit.monster[0].knockback[0], 0.0f);
}
