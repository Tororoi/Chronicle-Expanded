#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/osmond/snail.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// Osmond (or Xiao) out with weapon_no (carrying sphere when given), a Gunny in slot 0 that ailments can take, with 500 life.
class SlimeTrail : public ::testing::Test {
protected:
    explicit SlimeTrail(int chara = CHARA_OSMOND, int weapon_no = ITEM_WEAPON_SNAIL, int sphere = 0) : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        floor_.status().cur_chara = chara_;
        floor_.SetUpModel(0, Monster::GUNNY);
        floor_.Spawn(0, 0);
        Gunny().status_chance = 100;
    }

    MONSTOR &Gunny() { return floor_.unit().monster[0]; }

    // A hit of owner's taking damage off the Gunny, as CheckDmg takes it: whether it was gooed, its goo then washed off and
    // its life put back for the next.
    bool Hit(int owner, int damage = 10) {
        COLLISION_HIT hit = {};
        hit.owner = owner;
        Gunny().hp -= damage;
        expanded::MonsterDamaged(floor_.unit(), 0, hit, damage);
        bool gooed = Gunny().slow_timer > 0;
        Gunny().slow_timer = 0;
        Gunny().hp = 500;
        return gooed;
    }

    int Gooed(int hits, int owner) {
        int gooed = 0;
        for (int i = 0; i < hits; i++) {
            gooed += Hit(owner) ? 1 : 0;
        }
        return gooed;
    }

    TestFloor floor_;
    Wielding  wielding_;
    int       chara_;
};

class SlimeTrailBySuperSteve : public SlimeTrail {
protected:
    SlimeTrailBySuperSteve() : SlimeTrail(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SNAIL) {}
};

class SlimeTrailWithoutTheSnail : public SlimeTrail {
protected:
    SlimeTrailWithoutTheSnail() : SlimeTrail(CHARA_OSMOND, ITEM_WEAPON_MACHINE_GUN) {}
};

} // namespace

TEST_F(SlimeTrail, OneHitInTwentyGoos) {
    expanded::SeedModRandom(5);
    int gooed = Gooed(4000, CHARA_OSMOND);
    EXPECT_GT(gooed, 140);
    EXPECT_LT(gooed, 260);
}

TEST_F(SlimeTrail, TheGooIsTheGamesOwn) {
    expanded::SeedModRandom(5);
    for (int i = 0; i < 1000 && Gunny().slow_timer == 0; i++) {
        COLLISION_HIT hit = {};
        hit.owner = CHARA_OSMOND;
        Gunny().hp -= 10;
        expanded::MonsterDamaged(floor_.unit(), 0, hit, 10);
    }
    EXPECT_EQ(Gunny().slow_timer, 180);
}

TEST_F(SlimeTrail, OnlyTheWieldersHits) {
    expanded::SeedModRandom(5);
    EXPECT_EQ(Gooed(2000, CHARA_XIAO), 0);
}

TEST_F(SlimeTrail, NotAKillingHit) {
    expanded::SeedModRandom(5);
    int gooed = 0;
    for (int i = 0; i < 2000; i++) {
        gooed += Hit(CHARA_OSMOND, 500) ? 1 : 0;
    }
    EXPECT_EQ(gooed, 0);
}

TEST_F(SlimeTrail, NotAMonsterImmuneOrAlreadyAiling) {
    expanded::SeedModRandom(5);
    Gunny().status_chance = 0;
    EXPECT_EQ(Gooed(2000, CHARA_OSMOND), 0);
    Gunny().status_chance = 100;
    Gunny().poison_timer = 180;
    EXPECT_EQ(Gooed(2000, CHARA_OSMOND), 0);
}

TEST_F(SlimeTrailBySuperSteve, XiaosPelletsGoo) {
    expanded::SeedModRandom(5);
    EXPECT_GT(Gooed(4000, CHARA_XIAO), 140);
}

TEST_F(SlimeTrailWithoutTheSnail, NothingGoos) {
    expanded::SeedModRandom(5);
    EXPECT_EQ(Gooed(2000, CHARA_OSMOND), 0);
}
