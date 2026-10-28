#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "character.hpp"
#include "collisiondata.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/weapons/goro/frozen_tuna.hpp"
#include "expanded/weapons/goro/frozen_tuna/ice_prisons.hpp"
#include "expanded/weapons/goro/frozen_tuna/prison_scales.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::frozen_tuna::MemberPrison;
using expanded::frozen_tuna::MonsterPrison;
using expanded::frozen_tuna::PrisonScale;
using expanded::frozen_tuna::PrisonStage;
using expanded::test::StepsUnpaused;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// The prison's clips as kori.chr's info.cfg gives them: forming 10-30, shattering 40-55, a quarter frame a step.
std::vector<MOTION_INFO> g_clips = {
    {.start = 10, .end = 30, .speed = 0.25f},
    {.start = 40, .end = 55, .speed = 0.25f}
};
CFrameVu1                g_prison_frame;
std::vector<std::string> g_loaded;

bool FakePrison(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    g_prison_frame.Initialize();
    chara.frame = &g_prison_frame;
    chara.motion_type.motion_info = g_clips.data();
    g_loaded.emplace_back(path);
    return true;
}

// Steps from the prison's start to formed, and from the shatter's start to gone.
constexpr int kFormSteps = 80;
constexpr int kShatterSteps = 28;

// Goro (or Xiao) out with weapon_no (carrying sphere when given) at 50 of 60 WHP, with 100 of 200 life; a Golem as
// monster 0 and a Skeleton Soldier as monster 1, both in play, with 500 life. The Golem's kind goes in model slot 1: kinds
// stay set up after a test, and a monster on slot 0 of a test that sets up none would be taken for a Golem.
class ColdStorage : public ::testing::Test {
protected:
    explicit ColdStorage(int chara = CHARA_GORO, int weapon_no = ITEM_WEAPON_FROZEN_TUNA, int sphere = 0)
        : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        g_loaded.clear();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakePrison);
        expanded::ColdStorageVisitBegins();
        expanded::SeedModRandom(7);
        Out(chara_, wielding_.weapon());
        floor_.SetUpModel(0, Monster::SKELETON_SOLDIER);
        floor_.SetUpModel(1, Monster::GOLEM);
        Place(0, 1);
        Place(1, 0);
    }

    void TearDown() override {
        expanded::ColdStorageCleared();
        expanded::ColdStorageVisitBegins();
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
    }

    // chara out with weapon as its equipped record, at 50 of 60 WHP and 100 of 200 life.
    void Out(int chara, const WEAPON_HAVE &weapon) {
        CUserStatus &status = floor_.status();
        status.cur_chara = chara;
        status.equipped_weapon_slot[chara] = 0;
        WEAPON_HAVE &record = status.chara_weapons[chara][0];
        record = weapon;
        record.durability = 60;
        record.durability_f = 50.0f;
        status.max_hp[chara] = 200;
        status.hp[chara] = 100;
        status.life_step[chara] = 0;
    }

    // A monster of model_slot's kind in play in slot monster, with 500 life.
    void Place(int monster, int model_slot) {
        floor_.Spawn(monster, model_slot);
        Live(monster).state = 2;
        Live(monster).revealed = -1;
        Live(monster).hp = Live(monster).max_hp = 500;
    }

    MONSTOR &Live(int monster) { return floor_.unit().monster[monster]; }

    CUserStatus &Status() { return floor_.status(); }

    WEAPON_HAVE &Weapon() { return Status().chara_weapons[Status().cur_chara][0]; }

    // The member's life, the gauge's target while it moves.
    int Life() {
        const CUserStatus &status = Status();
        int                chara = status.cur_chara;
        return status.life_step[chara] != 0 ? status.next_hp[chara] : status.hp[chara];
    }

    void Hurt(int amount) {
        Status().hp[Status().cur_chara] = static_cast<s16>(Life() - amount);
        Status().life_step[Status().cur_chara] = 0;
    }

    void Step(int steps = 1) {
        for (int i = 0; i < steps; i++) {
            expanded::DungeonMonstersTick();
        }
    }

    // A hit of owner's taking 10 life off monster, as CheckDmg takes it.
    void Hit(int owner, int monster = 0) {
        COLLISION_HIT hit = {};
        hit.owner = owner;
        Live(monster).hp -= 10;
        expanded::MonsterDamaged(floor_.unit(), monster, hit, 10);
    }

    bool Frozen() { return (Status().ailments[Status().cur_chara] & AILMENT_FREEZE) != 0; }

    // Hits monster 0 until one stops the floor: whether one did within tries.
    bool HitUntilStopped(int tries = 2000) {
        for (int i = 0; i < tries; i++) {
            Hit(Status().cur_chara);
            Live(0).hp = 500;
            if (Frozen()) {
                return true;
            }
        }
        return false;
    }

    // The floor thawed: every freeze put out, the prisons left as they stand.
    void Thaw() {
        for (MONSTOR &live : floor_.unit().monster) {
            live.stop_timer = 0;
        }
        Status().ailments[Status().cur_chara] = 0;
        Status().ailment_frames[Status().cur_chara] = 0;
    }

    TestFloor     floor_;
    Wielding      wielding_;
    StepsUnpaused stepping_;
    int           chara_;
};

class ColdStorageBySuperSteve : public ColdStorage {
protected:
    ColdStorageBySuperSteve() : ColdStorage(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_FROZEN_TUNA) {}
};

class ColdStorageWithoutTheTuna : public ColdStorage {
protected:
    ColdStorageWithoutTheTuna() : ColdStorage(CHARA_GORO, ITEM_WEAPON_BATTLE_AX) {}
};

} // namespace

TEST_F(ColdStorage, EachWhpLostBanksTwoLife) {
    Step();
    Weapon().durability_f -= 1.5f;
    Step();
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 3.0f);
    Weapon().durability_f -= 4.0f;
    Step();
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 11.0f);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(true), 0.0f);
    EXPECT_EQ(Life(), 100);
}

TEST_F(ColdStorage, DamageDrainsThePoolALifeEveryThirtyFrames) {
    Step();
    Weapon().durability_f -= 2.0f;
    Step();
    Hurt(10);
    Step(expanded::kColdStorageDrainFrames - 1);
    EXPECT_EQ(Life(), 90);
    Step();
    EXPECT_EQ(Life(), 91);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 3.0f);
    Step(expanded::kColdStorageDrainFrames * 3);
    EXPECT_EQ(Life(), 94);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
    Step(expanded::kColdStorageDrainFrames * 3);
    EXPECT_EQ(Life(), 94);
}

TEST_F(ColdStorage, NothingDrainsBeforeDamage) {
    Step();
    Weapon().durability_f -= 5.0f;
    Step(expanded::kColdStorageDrainFrames * 4);
    EXPECT_EQ(Life(), 100);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 10.0f);
}

TEST_F(ColdStorage, DamageWithNothingBankedStartsNoDrain) {
    Step();
    Hurt(10);
    Step();
    Weapon().durability_f -= 5.0f;
    Step(expanded::kColdStorageDrainFrames * 4);
    EXPECT_EQ(Life(), 90);
    Hurt(1);
    Step(expanded::kColdStorageDrainFrames);
    EXPECT_EQ(Life(), 90);
}

TEST_F(ColdStorage, TheDrainPausesAtFullLifeAndKeepsThePool) {
    Status().hp[CHARA_GORO] = 199;
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    Hurt(1);
    Step(expanded::kColdStorageDrainFrames * 4);
    EXPECT_EQ(Life(), 200);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 8.0f);
    Hurt(1);
    Step(expanded::kColdStorageDrainFrames);
    EXPECT_EQ(Life(), 200);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 7.0f);
}

TEST_F(ColdStorage, TheDrainCountsOnlyFramesTheMonstersStep) {
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    Hurt(10);
    driveStepHold = 1;
    Step(expanded::kColdStorageDrainFrames * 3);
    driveStepHold = 0;
    EXPECT_EQ(Life(), 90);
    Step(expanded::kColdStorageDrainFrames);
    EXPECT_EQ(Life(), 91);
}

TEST_F(ColdStorage, ARepairEmptiesThePool) {
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    Hurt(10);
    Step();
    Weapon().durability_f = 60.0f;
    Step(expanded::kColdStorageDrainFrames * 2);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
    EXPECT_EQ(Life(), 90);
}

TEST_F(ColdStorage, PuttingTheTunaAwayEmptiesThePool) {
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    {
        Wielding other(ITEM_WEAPON_BATTLE_AX);
        Step();
        EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
    }
    Step();
    Hurt(10);
    Step(expanded::kColdStorageDrainFrames * 2);
    EXPECT_EQ(Life(), 90);
}

TEST_F(ColdStorage, ANewDungeonVisitEmptiesThePool) {
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    expanded::DungeonStatusReady();
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
}

TEST_F(ColdStorage, GorosPoolAndSuperStevesAreTheirOwn) {
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    ASSERT_FLOAT_EQ(expanded::ColdStorageBanked(false), 10.0f);

    // Xiao out with Super Steve and the Tuna's sphere: Goro's pool goes with him, hers starts empty.
    {
        Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_FROZEN_TUNA);
        Out(CHARA_XIAO, steve.weapon());
        Step();
        EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
        EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(true), 0.0f);
        Weapon().durability_f -= 2.0f;
        Step();
        EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(true), 4.0f);
        EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
    }

    // Goro back: hers goes, his starts empty.
    Out(CHARA_GORO, wielding_.weapon());
    Step();
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(true), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
}

TEST_F(ColdStorageBySuperSteve, XiaosPoolHealsHer) {
    Step();
    Weapon().durability_f -= 1.0f;
    Step();
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(true), 2.0f);
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
    Hurt(10);
    Step(expanded::kColdStorageDrainFrames * 2);
    EXPECT_EQ(Life(), 92);
}

TEST_F(ColdStorageWithoutTheTuna, NoPoolAndNoStop) {
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
    EXPECT_FALSE(HitUntilStopped());
}

TEST_F(ColdStorage, WithWeaponAbilitiesOffNoPoolAndNoStop) {
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Step();
    Weapon().durability_f -= 5.0f;
    Step();
    EXPECT_FLOAT_EQ(expanded::ColdStorageBanked(false), 0.0f);
    EXPECT_FALSE(HitUntilStopped());
}

TEST_F(ColdStorage, OneHitInTwentyStopsTheFloor) {
    int stops = 0;
    for (int i = 0; i < 4000; i++) {
        Hit(CHARA_GORO);
        Live(0).hp = 500;
        if (Frozen()) {
            stops++;
            Thaw();
        }
    }
    EXPECT_GT(stops, 140);
    EXPECT_LT(stops, 260);
}

TEST_F(ColdStorage, OnlyTheWieldersHitsStop) {
    for (int i = 0; i < 2000; i++) {
        Hit(CHARA_XIAO);
        Live(0).hp = 500;
    }
    EXPECT_FALSE(Frozen());
    EXPECT_EQ(Live(0).stop_timer, 0);
}

TEST_F(ColdStorage, TheStopFreezesEveryMonsterInPlayButTheIceKinds) {
    floor_.SetUpModel(2, Monster::BLIZZARD);
    floor_.SetUpModel(3, Monster::SAM);
    floor_.SetUpModel(4, Monster::GEMRON_ICE);
    for (int monster = 2; monster <= 4; monster++) {
        Place(monster, monster);
    }
    Place(5, 1);
    Live(5).state = 1; // not stepping, away from the player
    Place(6, 1);
    Live(6).revealed = 0; // in its chest
    Place(7, 1);
    Live(7).hp = 0; // dying
    Live(1).poison_timer = 100;
    Live(1).slow_timer = 100;

    ASSERT_TRUE(HitUntilStopped());
    EXPECT_EQ(Live(0).stop_timer, expanded::kColdStorageMonsterFrames);
    EXPECT_EQ(Live(1).stop_timer, expanded::kColdStorageMonsterFrames);
    EXPECT_EQ(Live(1).poison_timer, 0);
    EXPECT_EQ(Live(1).slow_timer, 0);
    EXPECT_FLOAT_EQ(Live(1).movement_speed, 0.0f);
    for (int monster = 2; monster <= 7; monster++) {
        EXPECT_EQ(Live(monster).stop_timer, 0) << monster;
        EXPECT_EQ(MonsterPrison(monster).stage, PrisonStage::kNone) << monster;
    }
}

TEST_F(ColdStorage, TheStopFreezesGoroTooForThreeSeconds) {
    Status().ailments[CHARA_GORO] = AILMENT_POISON;
    ASSERT_TRUE(HitUntilStopped());
    EXPECT_EQ(Status().ailments[CHARA_GORO], AILMENT_FREEZE);
    EXPECT_EQ(Status().ailment_frames[CHARA_GORO], expanded::kColdStorageWielderFrames);
}

TEST_F(ColdStorage, FrozenGorosHitsLandNothing) {
    COLLISION_HIT swing = {};
    swing.owner = CHARA_GORO;
    EXPECT_FALSE(expanded::MonsterHitRefused(floor_.unit(), 0, swing));
    ASSERT_TRUE(HitUntilStopped());
    EXPECT_TRUE(expanded::MonsterHitRefused(floor_.unit(), 0, swing));
    COLLISION_HIT other = {};
    other.owner = -1;
    EXPECT_FALSE(expanded::MonsterHitRefused(floor_.unit(), 0, other));
    Thaw();
    EXPECT_FALSE(expanded::MonsterHitRefused(floor_.unit(), 0, swing));
}

TEST_F(ColdStorage, EachFrozenBodyStandsInAPrisonOfItsSize) {
    ASSERT_TRUE(HitUntilStopped());
    ASSERT_EQ(g_loaded.size(), 1u);
    EXPECT_EQ(g_loaded[0], expanded::kIcePrisonFile);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kForming);
    EXPECT_FLOAT_EQ(MonsterPrison(0).time, 10.0f);
    EXPECT_FLOAT_EQ(MonsterPrison(0).scale, PrisonScale(Monster::GOLEM));
    EXPECT_EQ(MonsterPrison(1).stage, PrisonStage::kForming);
    EXPECT_FLOAT_EQ(MonsterPrison(1).scale, PrisonScale(Monster::SKELETON_SOLDIER));
    EXPECT_GT(PrisonScale(Monster::GOLEM), PrisonScale(Monster::SKELETON_SOLDIER));
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kForming);
    EXPECT_EQ(MemberPrison().body, CHARA_GORO);
    EXPECT_FLOAT_EQ(MemberPrison().scale, PrisonScale(CHARA_GORO));
    EXPECT_EQ(MonsterPrison(2).stage, PrisonStage::kNone);
}

TEST_F(ColdStorage, APrisonFormsHoldsWhileFrozenAndShattersAsTheFreezeEnds) {
    ASSERT_TRUE(HitUntilStopped());
    Step(kFormSteps - 1);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kForming);
    EXPECT_FLOAT_EQ(MonsterPrison(0).time, 10.0f + 0.25f * (kFormSteps - 1));
    Step();
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kFormed);
    EXPECT_FLOAT_EQ(MonsterPrison(0).time, 30.0f - 0.01f);

    // Held while the freeze lasts.
    Step(100);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kFormed);
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kFormed);

    // Goro thaws first; his shatters, the monsters' hold.
    Status().ailments[CHARA_GORO] = 0;
    Step();
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kShattering);
    EXPECT_FLOAT_EQ(MemberPrison().time, 40.0f);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kFormed);

    // The Golem's freeze runs out: its prison shatters, and is gone at the shatter's end.
    Live(0).stop_timer = 0;
    Step();
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kShattering);
    EXPECT_EQ(MonsterPrison(1).stage, PrisonStage::kFormed);
    Step(kShatterSteps - 1);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kShattering);
    EXPECT_FLOAT_EQ(MonsterPrison(0).time, 40.0f + expanded::kIcePrisonShatterSpeed * (kShatterSteps - 1));
    Step();
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kNone);
    EXPECT_EQ(MonsterPrison(1).stage, PrisonStage::kFormed);
}

TEST_F(ColdStorage, AFreezeEndedWhileFormingShattersAtOnce) {
    ASSERT_TRUE(HitUntilStopped());
    Step(10);
    Live(1).stop_timer = 0;
    Step();
    EXPECT_EQ(MonsterPrison(1).stage, PrisonStage::kShattering);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kForming);
}

TEST_F(ColdStorage, AHitOnFrozenGoroBreaksHisFreezeAndHisPrison) {
    ASSERT_TRUE(HitUntilStopped());
    Step(kFormSteps);
    COLLISION_HIT hit = {};
    expanded::PlayerHitReached(hit);
    EXPECT_FALSE(Frozen());
    Step();
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kShattering);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kFormed);
}

TEST_F(ColdStorage, AKilledMonstersPrisonShatters) {
    ASSERT_TRUE(HitUntilStopped());
    Step(kFormSteps);
    Live(1).hp = 0;
    Step();
    EXPECT_EQ(MonsterPrison(1).stage, PrisonStage::kShattering);
}

TEST_F(ColdStorage, AnotherStopHoldsTheFormedPrisonsAndFormsNewOnes) {
    ASSERT_TRUE(HitUntilStopped());
    Step(kFormSteps);
    Status().ailments[CHARA_GORO] = 0;
    Live(1).stop_timer = 0;
    Step(5);
    ASSERT_EQ(MonsterPrison(1).stage, PrisonStage::kShattering);
    ASSERT_TRUE(HitUntilStopped());
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kFormed);
    EXPECT_EQ(MonsterPrison(1).stage, PrisonStage::kForming);
    EXPECT_FLOAT_EQ(MonsterPrison(1).time, 10.0f);
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kForming);
}

TEST_F(ColdStorage, EveryMonsterOnTheFloorCanStandInOne) {
    for (int monster = 2; monster < expanded::kFloorMonsters; monster++) {
        Place(monster, monster % 2);
    }
    ASSERT_TRUE(HitUntilStopped());
    for (int monster = 0; monster < expanded::kFloorMonsters; monster++) {
        EXPECT_EQ(MonsterPrison(monster).stage, PrisonStage::kForming) << monster;
    }
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kForming);
    EXPECT_EQ(g_loaded.size(), 1u);
}

TEST_F(ColdStorage, TheFloorsMonstersPutBackTakeThePrisons) {
    ASSERT_TRUE(HitUntilStopped());
    expanded::DungeonMonstersCleared(false);
    EXPECT_EQ(MonsterPrison(0).stage, PrisonStage::kNone);
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kNone);
}

TEST_F(ColdStorageBySuperSteve, XiaosHitsStopTheFloorAndFreezeHerInAPrison) {
    ASSERT_TRUE(HitUntilStopped());
    EXPECT_EQ(Live(0).stop_timer, expanded::kColdStorageMonsterFrames);
    EXPECT_EQ(Status().ailment_frames[CHARA_XIAO], expanded::kColdStorageWielderFrames);
    EXPECT_EQ(MemberPrison().stage, PrisonStage::kForming);
    EXPECT_EQ(MemberPrison().body, CHARA_XIAO);
    EXPECT_FLOAT_EQ(MemberPrison().scale, PrisonScale(CHARA_XIAO));
}

TEST(PrisonScales, BiggerKindsTakeBiggerPrisonsAndTheMembersOne) {
    for (Character member : {CHARA_TOAN, CHARA_XIAO, CHARA_GORO, CHARA_RUBY, CHARA_UNGAGA, CHARA_OSMOND}) {
        EXPECT_FLOAT_EQ(PrisonScale(member), 1.0f);
    }
    EXPECT_GT(PrisonScale(Monster::GOLEM), 1.5f);
    EXPECT_LT(PrisonScale(Monster::CAVE_BAT), 1.0f);
    EXPECT_GT(PrisonScale(Monster::DRAN), PrisonScale(Monster::GOLEM));
    for (int kind = 0; kind <= expanded::No(Monster::ATLA_GEMRON); kind++) {
        EXPECT_GT(PrisonScale(static_cast<Monster>(kind)), 0.0f) << kind;
    }
    EXPECT_FLOAT_EQ(PrisonScale(static_cast<Monster>(-1)), 1.0f);
}
