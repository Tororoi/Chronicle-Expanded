#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/goro/midas_curse.hpp"
#include "expanded/weapons/goro/midas_touch.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

using expanded::Monster;

namespace {

constexpr int kGoroOwner = 2;

// Goro out with weapon_no in his hand (and equipped: Big Bucks Hammer's curse on him), a Skeleton Soldier of 75 life
// (of 75) in slot 0.
class MidasTouch : public ::testing::Test {
protected:
    explicit MidasTouch(int weapon_no = ITEM_WEAPON_BIG_BUCKS_HAMMER) : wielding_(weapon_no) {}

    void SetUp() override {
        floor_.status().cur_chara = CHARA_GORO;
        floor_.status().equipped_weapon_slot[CHARA_GORO] = 0;
        floor_.status().chara_weapons[CHARA_GORO][0] = {};
        floor_.status().chara_weapons[CHARA_GORO][0].item_no = static_cast<s16>(wielding_.weapon().item_no);
        floor_.status().hp[CHARA_GORO] = 100;
        expanded::MidasCurseReset();
        expanded::CurseStep();
        saved_weapon_ = NowWeapon;
        NowWeapon = &MainWeapon;
        floor_.SetUpModel(0, Monster::SKELETON_SOLDIER);
        floor_.Spawn(0, 0);
        MONSTOR &live = floor_.unit().monster[0];
        live.state = 2;
        live.revealed = -1;
        live.max_hp = 75;
        live.hp = 75;
        floor_.unit().chara[0][0].frame = nullptr;
        expanded::SeedModRandom(3);
        expanded::MidasReset();
        expanded::CurseStep(); // cursed on the floor the spawn began
    }

    void TearDown() override {
        floor_.status().equipped_weapon_slot[CHARA_GORO] = -1;
        expanded::CurseStep();
        expanded::MidasCurseReset();
        expanded::MidasReset();
        NowWeapon = saved_weapon_;
    }

    // One of Goro's hits of 40 on it.
    COLLISION_HIT Hit() {
        COLLISION_HIT hit{};
        hit.owner = kGoroOwner;
        hit.damage = 40;
        return hit;
    }

    // A hit on it with 10 of its 75 life left, under a quarter: whether it turned it to gold.
    bool HitUntilGold(COLLISION_HIT &turned) {
        floor_.unit().monster[0].hp = 10;
        turned = Hit();
        expanded::MidasHitTaken(floor_.unit(), 0, turned);
        return expanded::MidasStatue(floor_.unit(), 0);
    }

    // Goro's hit on it at full life, and the game's damage check after it: whether that left it a statue.
    bool StatueAfter(MonsterDamageResult result) {
        COLLISION_HIT hit = Hit();
        expanded::MidasHitTaken(floor_.unit(), 0, hit);
        EXPECT_FALSE(expanded::MidasStatue(floor_.unit(), 0)); // not yet: not under a quarter of its life
        EXPECT_EQ(hit.damage, 40);                             // and the hit left as it was
        floor_.unit().current_monster = 0;
        expanded::MidasCheckDmg(floor_.unit(), result);
        return expanded::MidasStatue(floor_.unit(), 0);
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  wielding_;
    CCharacter               *saved_weapon_ = nullptr;
};

class MidasTouchWithAnother : public MidasTouch {
protected:
    MidasTouchWithAnother() : MidasTouch(ITEM_WEAPON_MAGICAL_HAMMER) {}
};

} // namespace

TEST_F(MidasTouch, AHitUnderAQuarterOfItsLifeTurnsIt) {
    COLLISION_HIT hit;
    EXPECT_TRUE(HitUntilGold(hit));
}

TEST_F(MidasTouch, AHitAtAQuarterOfItsLifeOrMoreDoesNot) {
    floor_.unit().monster[0].hp = 75 * expanded::kMidasLowLife / 100 + 1;
    COLLISION_HIT hit = Hit();
    expanded::MidasHitTaken(floor_.unit(), 0, hit);
    EXPECT_FALSE(expanded::MidasStatue(floor_.unit(), 0));
}

TEST_F(MidasTouch, TheHitThatKillsItTurnsIt) {
    EXPECT_TRUE(StatueAfter(MonsterDamageResult::Killed));
    EXPECT_EQ(floor_.unit().monster[0].state, -1); // its kill ended at once, as a statue's
}

TEST_F(MidasTouch, AHitThatOnlyHurtsItDoesNot) {
    EXPECT_FALSE(StatueAfter(MonsterDamageResult::Hit));
    floor_.unit().current_monster = 0;
    EXPECT_EQ(expanded::MidasCheckDmg(floor_.unit(), MonsterDamageResult::Killed), MonsterDamageResult::Killed); // a later kill
}

TEST_F(MidasTouch, TheTurningHitKillsItAndOnlyIt) {
    COLLISION_HIT hit;
    ASSERT_TRUE(HitUntilGold(hit));
    EXPECT_GE(hit.damage, 10 * 10); // far past its life
    floor_.unit().monster[0].hp = 0;
    expanded::MidasDamaged(floor_.unit(), 0, hit);
    EXPECT_EQ(hit.damage, 40); // its own again for any other it reaches
    EXPECT_TRUE(expanded::MidasStatue(floor_.unit(), 0));
}

TEST_F(MidasTouch, OneItCouldNotKillIsNoStatue) {
    COLLISION_HIT hit;
    ASSERT_TRUE(HitUntilGold(hit));
    expanded::MidasDamaged(floor_.unit(), 0, hit); // its element's no effect: still alive
    EXPECT_FALSE(expanded::MidasStatue(floor_.unit(), 0));
}

TEST_F(MidasTouch, ItStaysAStatueOnceTheGameIsDoneWithIt) {
    COLLISION_HIT hit;
    ASSERT_TRUE(HitUntilGold(hit));
    floor_.unit().monster[0].hp = 0;
    expanded::MidasDamaged(floor_.unit(), 0, hit);
    floor_.unit().monster[0].state = -1;
    expanded::MidasStep();
    EXPECT_TRUE(expanded::MidasStatue(floor_.unit(), 0));

    // Its turning flash on its light, fading out over its frames, and the shine after.
    float flashing[3] = {};
    expanded::MidasLight(floor_.unit().chara[0][0], flashing);
    EXPECT_GT(flashing[0], expanded::kMidasShine[0]);
    for (int i = 0; i < expanded::kMidasFlashFrames; i++) {
        expanded::MidasStep();
    }
    float light[3] = {};
    expanded::MidasLight(floor_.unit().chara[0][0], light);
    EXPECT_FLOAT_EQ(light[0], expanded::kMidasShine[0]);
}

TEST_F(MidasTouch, AStatuesDropIsSometimesAGoldBar) {
    ASSERT_TRUE(StatueAfter(MonsterDamageResult::Killed));
    int bars = 0;
    for (int i = 0; i < 2000; i++) {
        bars += expanded::MidasDropItem(floor_.unit(), 0, -1) == ITEM_GOLD_BULLION;
    }
    EXPECT_NEAR(bars, 2000 * expanded::kMidasBarChance / 100, 50);
}

TEST_F(MidasTouch, AKeyItCarriesIsNeverReplaced) {
    ASSERT_TRUE(StatueAfter(MonsterDamageResult::Killed));
    for (int i = 0; i < 200; i++) {
        EXPECT_EQ(expanded::MidasDropItem(floor_.unit(), 0, ITEM_TREASURE_KEY), ITEM_TREASURE_KEY);
    }
}

TEST_F(MidasTouch, AnotherMonstersDropIsItsOwn) {
    for (int i = 0; i < 200; i++) {
        EXPECT_EQ(expanded::MidasDropItem(floor_.unit(), 0, -1), -1);
    }
}

TEST_F(MidasTouch, ABossIsNeverTurned) {
    floor_.unit().monster[0].kind = MONSTER_KIND_NO_LOCK_ON;
    COLLISION_HIT hit;
    EXPECT_FALSE(HitUntilGold(hit));
}

TEST_F(MidasTouch, OnlyWhileGoroIsCursed) {
    floor_.status().ailments[CHARA_GORO] &= ~AILMENT_CURSE; // lifted: he turns to gold instead
    expanded::CurseStep();
    COLLISION_HIT hit;
    EXPECT_FALSE(HitUntilGold(hit));
}

TEST_F(MidasTouch, XiaosHitWithItsSphereOnSuperSteveTurns) {
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BIG_BUCKS_HAMMER);
    CUserStatus             &status = floor_.status();
    status.cur_chara = CHARA_XIAO;
    status.hp[CHARA_XIAO] = 100;
    status.equipped_weapon_slot[CHARA_XIAO] = 0;
    status.chara_weapons[CHARA_XIAO][0] = steve.weapon();
    expanded::CurseStep();
    ASSERT_TRUE(expanded::MidasTouchWielded());

    floor_.unit().monster[0].hp = 10; // under a quarter
    COLLISION_HIT hit = Hit();
    hit.owner = CHARA_XIAO; // her pellet's
    expanded::MidasHitTaken(floor_.unit(), 0, hit);
    EXPECT_TRUE(expanded::MidasStatue(floor_.unit(), 0));
    status.equipped_weapon_slot[CHARA_XIAO] = -1;
    expanded::CurseStep();
}

TEST_F(MidasTouch, AnotherCharactersHitNeverTurns) {
    floor_.unit().monster[0].hp = 10;
    COLLISION_HIT hit = Hit();
    hit.owner = 0;
    expanded::MidasHitTaken(floor_.unit(), 0, hit);
    floor_.unit().current_monster = 0;
    expanded::MidasCheckDmg(floor_.unit(), MonsterDamageResult::Killed);
    EXPECT_FALSE(expanded::MidasStatue(floor_.unit(), 0));
}

TEST_F(MidasTouch, AStatueStaysSolid) {
    EXPECT_FALSE(expanded::MidasSolid(floor_.unit(), 0));
    ASSERT_TRUE(StatueAfter(MonsterDamageResult::Killed));
    EXPECT_TRUE(expanded::MidasSolid(floor_.unit(), 0));
}

TEST_F(MidasTouch, WhatDropsInsideAStatueSlidesToGoroWhileHeIsAgainstIt) {
    CMonstorUnit &unit = floor_.unit();
    unit.monster[0].body_radius = 13.0f;
    unit.effect3[0].count = 0;
    unit.chara[0][0].pos[0] = 100.0f;
    unit.chara[0][0].pos[2] = 50.0f;
    float saved[4];
    std::copy(std::begin(CharaMain.pos), std::end(CharaMain.pos), saved);
    float drop[4] = {110.0f, 0.0f, 50.0f, 1.0f};

    CharaMain.pos[0] = 100.0f - 6.0f - 13.0f; // against it, on the far side from the drop
    CharaMain.pos[2] = 50.0f;
    expanded::MidasPullDrop(drop);
    EXPECT_FLOAT_EQ(drop[0], 110.0f); // no statue yet

    ASSERT_TRUE(StatueAfter(MonsterDamageResult::Killed));
    expanded::MidasPullDrop(drop);
    EXPECT_FLOAT_EQ(drop[0], 110.0f - expanded::kMidasPull);
    for (int i = 0; i < 100; i++) {
        expanded::MidasPullDrop(drop);
    }
    EXPECT_LE(std::abs(drop[0] - CharaMain.pos[0]), 5.0f); // out to where he picks it up

    float away[4] = {110.0f, 0.0f, 50.0f, 1.0f};
    CharaMain.pos[0] = 40.0f; // not against it
    expanded::MidasPullDrop(away);
    EXPECT_FLOAT_EQ(away[0], 110.0f);
    std::copy(std::begin(saved), std::end(saved), CharaMain.pos);
}

TEST_F(MidasTouchWithAnother, OnlyBigBucksHammerTurns) {
    COLLISION_HIT hit;
    EXPECT_FALSE(HitUntilGold(hit));
}

TEST_F(MidasTouch, AStatuesKillEndsAtOnceAsItsDeathScriptWouldEndIt) {
    COLLISION_HIT hit;
    ASSERT_TRUE(HitUntilGold(hit));
    CMonstorUnit &unit = floor_.unit();
    unit.current_monster = 0;
    unit.alive_count = 3;
    EXPECT_EQ(expanded::MidasCheckDmg(unit, MonsterDamageResult::Killed), MonsterDamageResult::None);
    EXPECT_EQ(unit.monster[0].state, -1);
    EXPECT_EQ(unit.alive_count, 2);
}

TEST_F(MidasTouch, AnotherKillIsLeftToTheGame) {
    CMonstorUnit &unit = floor_.unit();
    unit.current_monster = 0;
    EXPECT_EQ(expanded::MidasCheckDmg(unit, MonsterDamageResult::Killed), MonsterDamageResult::Killed);
    EXPECT_EQ(unit.monster[0].state, 2);
}
