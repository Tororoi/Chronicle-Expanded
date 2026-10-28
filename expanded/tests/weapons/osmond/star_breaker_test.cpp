#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/osmond/star_breaker.hpp"
#include "expanded/weapons/osmond/star_breaker/shooting_stars.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::test::OptionScope;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// chara out with weapon_no (carrying sphere when given), a Gunny in slot 0; the stars of earlier tests gone.
class ShootingStarProc : public ::testing::Test {
protected:
    explicit ShootingStarProc(int chara = CHARA_OSMOND, int weapon_no = ITEM_WEAPON_STAR_BREAKER, int sphere = 0)
        : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        floor_.status().cur_chara = chara_;
        floor_.SetUpModel(0, Monster::GUNNY);
        floor_.Spawn(0, 0);
        expanded::ShootingStarsVisitBegins();
        expanded::SeedModRandom(7);
    }

    void TearDown() override { expanded::ShootingStarsVisitBegins(); }

    // hits fresh hits of owner's landing on the Gunny, taken or blocked: the stars they sent.
    int Stars(int hits, int owner, bool blocked = false) {
        for (int i = 0; i < hits; i++) {
            COLLISION_HIT hit = {};
            hit.owner = owner;
            if (blocked) {
                expanded::MonsterHitBlocked(floor_.unit(), 0, hit);
            } else {
                expanded::MonsterHitTaken(floor_.unit(), 0, hit);
            }
        }
        return expanded::StarsWaiting();
    }

    TestFloor floor_;
    Wielding  wielding_;
    int       chara_;
};

class ShootingStarProcBySuperSteve : public ShootingStarProc {
protected:
    ShootingStarProcBySuperSteve() : ShootingStarProc(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_STAR_BREAKER) {}
};

class ShootingStarProcWithoutIt : public ShootingStarProc {
protected:
    ShootingStarProcWithoutIt() : ShootingStarProc(CHARA_OSMOND, ITEM_WEAPON_SUPERNOVA) {}
};

class ShootingStarProcByAnotherSphere : public ShootingStarProc {
protected:
    ShootingStarProcByAnotherSphere() : ShootingStarProc(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SNAIL) {}
};

} // namespace

TEST_F(ShootingStarProc, OneHitInAThousandSendsAStar) {
    EXPECT_DOUBLE_EQ(expanded::ShootingStarChance(CHARA_OSMOND), 0.001);
    int stars = Stars(100000, CHARA_OSMOND);
    EXPECT_GT(stars, 60);
    EXPECT_LT(stars, 140);
}

TEST_F(ShootingStarProc, ABlockedHitRollsToo) {
    int stars = Stars(100000, CHARA_OSMOND, true);
    EXPECT_GT(stars, 60);
    EXPECT_LT(stars, 140);
}

TEST_F(ShootingStarProc, OnlyTheWieldersHits) {
    EXPECT_DOUBLE_EQ(expanded::ShootingStarChance(CHARA_XIAO), 0.0);
    EXPECT_EQ(Stars(20000, CHARA_XIAO), 0);
    EXPECT_EQ(Stars(20000, -1), 0);
}

// The same hit meeting the Gunny's spheres again, blocked and then taken, and another monster: one roll. The record's
// next hit (CCollisionData::Set) rolls afresh.
TEST_F(ShootingStarProc, AHitRollsOnce) {
    CCollisionData  hits = {};
    CCollisionData *saved = NowColData;
    NowColData = &hits;
    float at[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    int   rolls = 0;
    for (int i = 0; i < 5000; i++) {
        int record = hits.Set(at, 10, 2, 4.0f, 1.0f, 2, 2, 0, 0);
        EXPECT_EQ(hits.hit[record].weapon_flags & expanded::kStarRolledMark, 0);
        hits.hit[record].owner = CHARA_OSMOND;
        hits.hit[record].weapon_flags = 0x40;
        int before = expanded::StarsWaiting();
        for (int sphere = 0; sphere < 3; sphere++) {
            expanded::MonsterHitBlocked(floor_.unit(), 0, hits.hit[record]);
        }
        expanded::MonsterHitTaken(floor_.unit(), 0, hits.hit[record]);
        expanded::MonsterHitTaken(floor_.unit(), 1, hits.hit[record]);
        EXPECT_LE(expanded::StarsWaiting() - before, 1);
        EXPECT_NE(hits.hit[record].weapon_flags & expanded::kStarRolledMark, 0);
        EXPECT_EQ(hits.hit[record].weapon_flags & 0xFFFF, 0x40); // the weapon's own flags kept
        rolls++;
        hits.active[record] = 0;
    }
    NowColData = saved;
    // A roll a hit: about five stars in 5000.
    EXPECT_GT(expanded::StarsWaiting(), 0);
    EXPECT_LT(expanded::StarsWaiting(), 15);
    EXPECT_EQ(rolls, 5000);
}

TEST_F(ShootingStarProc, NotWithTheWeaponAbilitiesOff) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_DOUBLE_EQ(expanded::ShootingStarChance(CHARA_OSMOND), 0.0);
    EXPECT_EQ(Stars(20000, CHARA_OSMOND), 0);
}

TEST_F(ShootingStarProcBySuperSteve, FiveHitsInAThousandSendAStar) {
    EXPECT_DOUBLE_EQ(expanded::ShootingStarChance(CHARA_XIAO), 0.005);
    int stars = Stars(40000, CHARA_XIAO);
    EXPECT_GT(stars, 140);
    EXPECT_LT(stars, 260);
}

TEST_F(ShootingStarProcBySuperSteve, ABlockedPelletRollsToo) {
    int stars = Stars(40000, CHARA_XIAO, true);
    EXPECT_GT(stars, 140);
    EXPECT_LT(stars, 260);
}

TEST_F(ShootingStarProcBySuperSteve, OnlyXiaosHits) {
    EXPECT_EQ(Stars(20000, CHARA_OSMOND), 0);
}

TEST_F(ShootingStarProcWithoutIt, NoStars) {
    EXPECT_EQ(Stars(20000, CHARA_OSMOND), 0);
    EXPECT_EQ(Stars(20000, CHARA_OSMOND, true), 0);
}

TEST_F(ShootingStarProcByAnotherSphere, NoStars) {
    EXPECT_EQ(Stars(20000, CHARA_XIAO), 0);
}
