#include <gtest/gtest.h>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/goro/midas_touch.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

using expanded::Monster;

namespace {

constexpr int kGoroOwner = 2;

// Goro out with weapon_no in his hand, a Skeleton Soldier of 75 life in slot 0.
class MidasTouch : public ::testing::Test {
protected:
    explicit MidasTouch(int weapon_no = ITEM_WEAPON_BIG_BUCKS_HAMMER) : wielding_(weapon_no) {}

    void SetUp() override {
        floor_.status().cur_chara = CHARA_GORO;
        saved_weapon_ = NowWeapon;
        NowWeapon = &MainWeapon;
        floor_.SetUpModel(0, Monster::SKELETON_SOLDIER);
        floor_.Spawn(0, 0);
        MONSTOR &live = floor_.unit().monster[0];
        live.state = 2;
        live.revealed = -1;
        live.hp = 75;
        floor_.unit().chara[0][0].frame = nullptr;
        expanded::SeedModRandom(3);
        expanded::MidasReset();
    }

    void TearDown() override {
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

    // Hits until one turns it to gold (at most tries): the one that did, or none.
    bool HitUntilGold(COLLISION_HIT &turned, int tries = 400) {
        for (int i = 0; i < tries; i++) {
            turned = Hit();
            expanded::MidasHitTaken(floor_.unit(), 0, turned);
            if (expanded::MidasStatue(floor_.unit(), 0)) {
                return true;
            }
        }
        return false;
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

TEST_F(MidasTouch, AboutOneHitInTwentyTurnsAMonsterToGold) {
    int turned = 0;
    for (int i = 0; i < 2000; i++) {
        COLLISION_HIT hit = Hit();
        expanded::MidasHitTaken(floor_.unit(), 0, hit);
        if (expanded::MidasStatue(floor_.unit(), 0)) {
            turned++;
            expanded::MidasReset();
        }
    }
    EXPECT_NEAR(turned, 2000 * expanded::kMidasChance / 100, 40);
}

TEST_F(MidasTouch, TheTurningHitKillsItAndOnlyIt) {
    COLLISION_HIT hit;
    ASSERT_TRUE(HitUntilGold(hit));
    EXPECT_GE(hit.damage, 75 * 10); // far past its life
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
    float light[3] = {};
    expanded::MidasLight(floor_.unit().chara[0][0], light);
    EXPECT_FLOAT_EQ(light[0], expanded::kMidasShine[0]);
}

TEST_F(MidasTouch, ABossIsNeverTurned) {
    floor_.unit().monster[0].kind = MONSTER_KIND_NO_LOCK_ON;
    COLLISION_HIT hit;
    EXPECT_FALSE(HitUntilGold(hit));
}

TEST_F(MidasTouch, AnotherCharactersHitNeverTurns) {
    for (int i = 0; i < 400; i++) {
        COLLISION_HIT hit = Hit();
        hit.owner = 0;
        expanded::MidasHitTaken(floor_.unit(), 0, hit);
    }
    EXPECT_FALSE(expanded::MidasStatue(floor_.unit(), 0));
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
