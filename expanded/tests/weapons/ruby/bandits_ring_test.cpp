#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/weapons/ruby/bandits_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "menu_dungeon.hpp"
#include "menu_misc.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::No;
using expanded::Species;
using expanded::test::OptionScope;
using expanded::test::TestFloor;
using expanded::test::Wielding;

constexpr int kSteal = 0x80; // the Steal ability on a weapon's flags and its hits'

// A floor with a Master Jacket out (a Repair Powder to steal) and chara out holding weapon_no, carrying sphere's
// SynthSphere when given.
class DoubleTake : public ::testing::Test {
protected:
    explicit DoubleTake(int chara = CHARA_RUBY, int weapon_no = ITEM_WEAPON_BANDIT_S_RING, int sphere = 0) : chara_(chara), wielding_(weapon_no, sphere) {}

    void SetUp() override {
        floor_.status().cur_chara = static_cast<s8>(chara_);
        floor_.SetUpModel(0, Monster::MASTER_JACKET);
        SpawnJacket();
        ASSERT_EQ(item_, ITEM_REPAIR_POWDER);
    }

    // SetupViewMonstor: the monster out on the floor with its kind's item.
    void SpawnJacket() {
        floor_.Spawn(0, 0);
        floor_.unit().monster[0].steal_item = floor_.unit().model[0].steal_item;
    }

    // CheckDmg on a hit of owner's (flags its weapon's) reaching the monster, the steal's roll and bag check passed: the
    // item taken, or -1.
    int Hit(int owner, int flags = kSteal) {
        COLLISION_HIT hit{};
        hit.owner = owner;
        hit.monster_no = -1;
        hit.weapon_flags = flags;
        expanded::MonsterHitTaken(floor_.unit(), 0, hit);
        MONSTOR &live = floor_.unit().monster[0];
        if (live.steal_item == -1 || (flags & kSteal) == 0) {
            return -1;
        }
        int item = live.steal_item;
        expanded::MonsterItemStolen(floor_.unit(), 0, item);
        live.steal_item = -1;
        return item;
    }

    int Hit() { return Hit(chara_); }

    int       chara_;
    TestFloor floor_;
    Wielding  wielding_;
    const int item_ = Species(No(Monster::MASTER_JACKET))->steal_item;
};

class DoubleTakeWithAnotherRing : public DoubleTake {
protected:
    DoubleTakeWithAnotherRing() : DoubleTake(CHARA_RUBY, ITEM_WEAPON_GOLD_RING) {}
};

class DoubleTakeBySuperSteve : public DoubleTake {
protected:
    DoubleTakeBySuperSteve() : DoubleTake(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BANDIT_S_RING) {}
};

class DoubleTakeBySuperSteveWithTheSlingshot : public DoubleTake {
protected:
    DoubleTakeBySuperSteveWithTheSlingshot() : DoubleTake(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BANDIT_SLINGSHOT) {}
};

} // namespace

TEST_F(DoubleTake, TakesTheItemASecondTime) {
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(expanded::MonsterSteals(0), 1);
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(expanded::MonsterSteals(0), 2);
}

TEST_F(DoubleTake, NeverAThirdTime) {
    ASSERT_EQ(Hit(), item_);
    ASSERT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(), -1);
    EXPECT_EQ(Hit(), -1);
    EXPECT_EQ(expanded::MonsterSteals(0), expanded::kMostSteals);
}

TEST_F(DoubleTake, OnlyAHitThatSteals) {
    ASSERT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(chara_, 0), -1);
    EXPECT_EQ(Hit(), item_);
}

TEST_F(DoubleTake, AfterAnotherWeaponsSteal) {
    {
        floor_.status().cur_chara = CHARA_XIAO;
        Wielding slingshot(ITEM_WEAPON_BANDIT_SLINGSHOT);
        ASSERT_EQ(Hit(CHARA_XIAO), item_);
        EXPECT_EQ(Hit(CHARA_XIAO), -1);
    }
    floor_.status().cur_chara = CHARA_RUBY;
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(expanded::MonsterSteals(0), 2);
}

TEST_F(DoubleTake, TheRingsMissLeavesNothingForAnotherWeapon) {
    ASSERT_EQ(Hit(), item_);
    COLLISION_HIT missed{};
    missed.owner = CHARA_RUBY;
    missed.weapon_flags = kSteal;
    expanded::MonsterHitTaken(floor_.unit(), 0, missed); // its roll lost: the item stays readied
    ASSERT_EQ(floor_.unit().monster[0].steal_item, item_);

    floor_.status().cur_chara = CHARA_XIAO;
    Wielding slingshot(ITEM_WEAPON_BANDIT_SLINGSHOT);
    EXPECT_EQ(Hit(CHARA_XIAO), -1);
}

TEST_F(DoubleTake, NotForAnotherMembersHit) {
    ASSERT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(CHARA_TOAN), -1);
}

TEST_F(DoubleTake, AMonsterSetUpAgainStartsUnrobbed) {
    ASSERT_EQ(Hit(), item_);
    ASSERT_EQ(Hit(), item_);
    SpawnJacket();
    EXPECT_EQ(expanded::MonsterSteals(0), 0);
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(), item_);
}

TEST_F(DoubleTake, OffWithTheWeaponAbilitiesOff) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(), -1);
}

TEST_F(DoubleTakeWithAnotherRing, TakesOnce) {
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(), -1);
    EXPECT_EQ(expanded::MonsterSteals(0), 1);
}

TEST_F(DoubleTakeBySuperSteve, TakesTheItemASecondTime) {
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(), -1);
}

TEST_F(DoubleTakeBySuperSteveWithTheSlingshot, TakesOnce) {
    EXPECT_EQ(Hit(), item_);
    EXPECT_EQ(Hit(), -1);
}

// The ring has the Steal ability, its SynthSphere takes it, and Super Steve's hits carry the sphere's.
TEST(DoubleTakeSphere, CarriesTheRingsSteal) {
    int ring = DefaultWeaponOptionSet(ITEM_WEAPON_BANDIT_S_RING);
    EXPECT_NE(ring & kSteal, 0);

    WEAPON_HAVE steve{};
    steve.item_no = ITEM_WEAPON_SUPER_STEVE;
    steve.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    steve.attach[0].sphere_weapon_no = ITEM_WEAPON_BANDIT_S_RING;
    steve.attach[0].sphere_flags = static_cast<s16>(ring);
    WEAPON_HAVE fights{};
    WeaponAllValueSet(&steve, &fights, 0);
    EXPECT_NE(fights.flags & kSteal, 0);
}
