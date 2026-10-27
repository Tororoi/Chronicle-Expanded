#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dngmessageman.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/scaling.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/ruby/pocklekul.hpp"
#include "expanded/weapons/ruby/pocklekul/fights.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "hitvalue.hpp"
#include "itembombeffect.hpp"
#include "itemdata.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "sysmes.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

extern CHitValue HitValue[32];
extern s32       lockOnTargetNo;

namespace {

using expanded::kAllyMonster;
using expanded::Monster;
using expanded::No;
using expanded::test::TestFloor;
using expanded::test::Wielding;

bool r1 = false;

bool HeldR1() {
    return r1;
}

int loads = 0;
int loaded_kind = -1;
int stood_clip = expanded::kPockleIdleClip; // the clip it stands in

// A pockle's load and stand, stood in for: its kind's record, standing at position, a hurt sphere of radius 4 five up.
bool FakePockle(CMonstorUnit &unit, int kind, const float *position) {
    loads++;
    loaded_kind = kind;
    unit.model[0] = *expanded::Species(kind);
    const MONSTOR_MODEL &model = unit.model[0];
    MONSTOR             &live = unit.monster[kAllyMonster];
    live.state = 2;
    live.base_model = 0;
    live.max_hp = live.hp = model.max_hp;
    live.defense = model.defense;
    live.item_damage_rate = model.item_damage_rate;
    live.drops_items = model.drops_items;
    live.exp = model.exp;
    live.name_no = model.name_no;
    live.collision_radius = model.collision_radius;
    live.revealed = -1;
    live.knockback[2] = 1.0f;
    for (int i = 0; i < 5; i++) {
        live.attachment_weight[i] = model.attachment_weight[i];
    }
    unit.chara[kAllyMonster][0].SetPosition(position[0], position[1], position[2]);
    unit.chara[kAllyMonster][0].motion_no = stood_clip;
    unit.effect[kAllyMonster].timer[0] = 1;
    unit.effect[kAllyMonster].radius[0] = 4.0f;
    unit.effect[kAllyMonster].position[0][0] = position[0];
    unit.effect[kAllyMonster].position[0][1] = position[1] + 5.0f;
    unit.effect[kAllyMonster].position[0][2] = position[2];
    return true;
}

// Ruby (or Xiao) out at the origin facing +z with weapon_no in hand (carrying sphere when given), on a quiet floor with
// Mummies set out by Stand; the pockle's load faked.
class Pocklekul : public ::testing::Test {
protected:
    explicit Pocklekul(int chara = CHARA_RUBY, int weapon_no = ITEM_WEAPON_POCKLEKUL, int sphere = 0) : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        saved_hits_ = NowColData;
        saved_values_ = NowHitValue;
        saved_pack_ = NowShotEffect;
        saved_bombs_ = NowBombEffect;
        saved_mode_ = BtGameModeFlag;
        saved_event_ = BtEventMode;
        saved_lock_ = lockOnTargetNo;
        saved_status_ = BtActStatus;
        CharaMain.GetPosition(saved_pos_);
        CharaMain.GetRotation(saved_rot_);
        saved_motion_ = CharaMain.motion_no;
        NowColData = hits_.get();
        NowHitValue = HitValue;
        NowShotEffect = pack_.get();
        NowBombEffect = bombs_;
        for (CSHOT_EFFECT &effect : pack_->effect) {
            effect.Initialize();
        }
        for (CItemBombEffect &bomb : bombs_) {
            bomb.Initialize();
        }
        floor_.status().cur_chara = chara_;
        BtGameModeFlag = BT_GAME_MODE_PLAY;
        BtEventMode = 0;
        driveStepHold = 0;
        frameCaputer = 0;
        BtActStatus = {};
        lockOnTargetNo = -1;
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        CharaMain.motion_no = 0;
        floor_.SetUpModel(0, Monster::MUMMY);
        loads = 0;
        loaded_kind = -1;
        stood_clip = expanded::kPockleIdleClip;
        r1 = false;
        expanded::SetAllyLoader(FakePockle);
        expanded::SetPocklekulGuardButton(HeldR1);
        expanded::SeedModRandom(1);
        expanded::PocklekulReset();
    }

    void TearDown() override {
        expanded::PocklekulReset();
        expanded::SetAllyLoader(nullptr);
        expanded::SetPocklekulGuardButton(nullptr);
        NowColData = saved_hits_;
        NowHitValue = saved_values_;
        NowShotEffect = saved_pack_;
        NowBombEffect = saved_bombs_;
        BtGameModeFlag = saved_mode_;
        BtEventMode = saved_event_;
        lockOnTargetNo = saved_lock_;
        BtActStatus = saved_status_;
        driveStepHold = 0;
        frameCaputer = 0;
        CharaMain.SetPosition(saved_pos_);
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        CharaMain.motion_no = saved_motion_;
    }

    void Guard(bool on) {
        r1 = on;
        BtActStatus.guard_mode = on ? expanded::kGuardUp : 0;
    }

    void Step(int frames = 1) {
        for (int i = 0; i < frames; i++) {
            expanded::PocklekulStep(0);
        }
    }

    // The guard held its frames, then let go.
    void Summon() {
        Guard(true);
        Step(expanded::kPockleGuardFrames);
        Guard(false);
    }

    CMonstorUnit &floor() { return floor_.unit(); }

    CMonstorUnit &ally() { return *expanded::AllyStanding(); }

    MONSTOR &pockle() { return ally().monster[kAllyMonster]; }

    CCharacter &body() { return ally().chara[kAllyMonster][0]; }

    void Place(float x, float z) { body().SetPosition(x, 0.0f, z); }

    float PockleAt(int axis) {
        float at[4];
        body().GetPosition(at);
        return at[axis];
    }

    // A Mummy in play at (x, z): a hurt sphere of radius r there.
    void Stand(int monster, float x, float z, float r = 5.0f) {
        floor_.Spawn(monster, 0);
        MONSTOR &live = floor().monster[monster];
        live.state = 2;
        live.revealed = -1;
        live.hp = 150;
        live.item_damage_rate = 100;
        live.collision_radius = 4.0f;
        live.money = 30;
        live.money_chance = 50;
        floor().chara[monster][0].SetPosition(x, 0.0f, z);
        floor().effect[monster].timer[0] = 1;
        floor().effect[monster].radius[0] = r;
        floor().effect[monster].position[0][0] = x;
        floor().effect[monster].position[0][1] = 5.0f;
        floor().effect[monster].position[0][2] = z;
    }

    // Where the pockle's script finds the player.
    void Target(float *out) {
        CMonstorUnit *saved = NowMonstorUnit;
        NowMonstorUnit = &ally();
        ally().current_monster = kAllyMonster;
        expanded::ScriptPlayerPosition(out);
        NowMonstorUnit = saved;
    }

    // Where floor monster's script finds the player.
    void FloorTarget(int monster, float *out) {
        floor().current_monster = monster;
        expanded::ScriptPlayerPosition(out);
    }

    // A swing record (CCollisionData::Set as CheckDmg sets one) at (x, h, z).
    int Swing(float x, float h, float z, float radius, int damage, int owner, int target, int flags = 0) {
        float at[4] = {x, h, z, 1.0f};
        int   hit = NowColData->Set(at, damage, 2, radius, 0.0f, target, 2, flags, 0);
        NowColData->SetUserID(owner, 0);
        return hit;
    }

    // Planted hits of the summoner's, not counting skip.
    int Planted(int skip = -1) {
        int count = 0;
        for (int i = 0; i < expanded::kHitRecords; i++) {
            count += i != skip && NowColData->active[i] != 0 && NowColData->hit[i].owner == chara_ && NowColData->hit[i].target_mask == expanded::kHitsMonsters;
        }
        return count;
    }

    const COLLISION_HIT *PlantedHit() {
        for (int i = 0; i < expanded::kHitRecords; i++) {
            if (NowColData->active[i] != 0 && NowColData->hit[i].owner == chara_ && NowColData->hit[i].target_mask == expanded::kHitsMonsters) {
                return &NowColData->hit[i];
            }
        }
        return nullptr;
    }

    TestFloor                          floor_;
    Wielding                           wielding_;
    int                                chara_;
    std::unique_ptr<CCollisionData>    hits_ = std::make_unique<CCollisionData>();
    std::unique_ptr<CSHOT_EFFECT_PACK> pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
    CItemBombEffect                    bombs_[3];
    CCollisionData                    *saved_hits_ = nullptr;
    CHitValue                         *saved_values_ = nullptr;
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    CItemBombEffect                   *saved_bombs_ = nullptr;
    int                                saved_mode_ = 0;
    s32                                saved_event_ = 0;
    s32                                saved_lock_ = -1;
    BT_ACT_STATUS                      saved_status_ = {};
    float                              saved_pos_[4] = {};
    float                              saved_rot_[4] = {};
    s32                                saved_motion_ = 0;
};

class PocklekulBySuperSteve : public Pocklekul {
protected:
    PocklekulBySuperSteve() : Pocklekul(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_POCKLEKUL) {}
};

class PocklekulOtherWeapon : public Pocklekul {
protected:
    PocklekulOtherWeapon() : Pocklekul(CHARA_RUBY, ITEM_WEAPON_GOLD_RING) {}
};

// ---- The summon ----

TEST_F(Pocklekul, SummonsOnTheGuardsLastFrame) {
    Stand(0, 0.0f, 100.0f); // an enemy: it stays where it stood for its first step
    Guard(true);
    Step(expanded::kPockleGuardFrames - 1);
    EXPECT_FALSE(expanded::PockleUp());
    Step();
    ASSERT_TRUE(expanded::PockleUp());
    EXPECT_EQ(loads, 1);
    EXPECT_NE(std::find(expanded::kPockleKinds.begin(), expanded::kPockleKinds.end(), static_cast<Monster>(loaded_kind)), expanded::kPockleKinds.end());
    EXPECT_EQ(No(expanded::PockleKind()), loaded_kind);
    // kPockleSpawnAhead in front of Ruby, facing +z.
    EXPECT_NEAR(PockleAt(0), 0.0f, 1e-4f);
    EXPECT_NEAR(PockleAt(2), expanded::kPockleSpawnAhead, 1e-4f);
    // No drops but the money it steals, no ABS.
    EXPECT_EQ(pockle().drops_items, 0);
    EXPECT_EQ(pockle().exp, 0);
    EXPECT_EQ(pockle().stolen_money, 0);
}

TEST_F(Pocklekul, LettingGoStartsTheChargeOver) {
    Guard(true);
    Step(40);
    Guard(false);
    Step();
    Guard(true);
    Step(expanded::kPockleGuardFrames - 1);
    EXPECT_FALSE(expanded::PockleUp());
    Step();
    EXPECT_TRUE(expanded::PockleUp());
}

TEST_F(Pocklekul, TheGuardPoseCounts) {
    r1 = true; // R1 down, but no guard up (no lock-on)
    Step(expanded::kPockleGuardFrames + 10);
    EXPECT_FALSE(expanded::PockleUp());
}

TEST_F(Pocklekul, OneStandsAtATime) {
    Summon();
    ASSERT_TRUE(expanded::PockleUp());
    CMonstorUnit *first = &ally();
    Guard(true);
    Step(2 * expanded::kPockleGuardFrames);
    EXPECT_EQ(loads, 1);
    EXPECT_EQ(&ally(), first);
}

TEST_F(Pocklekul, WaitsForAQuietFloor) {
    Guard(true);
    driveStepHold = 1;
    Step(expanded::kPockleGuardFrames + 5);
    EXPECT_FALSE(expanded::PockleUp());
    driveStepHold = 0;
    Step();
    EXPECT_TRUE(expanded::PockleUp());
}

TEST_F(Pocklekul, EveryPockleComes) {
    std::set<int> kinds;
    for (int i = 0; i < 64; i++) {
        Summon();
        kinds.insert(loaded_kind);
        expanded::PocklekulReset();
    }
    EXPECT_EQ(kinds.size(), expanded::kPockleKinds.size());
}

TEST_F(Pocklekul, GrowsInAndFadesItsTint) {
    Guard(true);
    Step(expanded::kPockleGuardFrames);
    ASSERT_TRUE(expanded::PockleUp());
    EXPECT_FLOAT_EQ(body().scale[0], expanded::kPockleGrowth[0]);
    // Its tint: the room's light and kPockleTint, let go at once with no smoke on the floor.
    float ambient[4];
    MGGetAmbient(ambient);
    for (int i = 0; i < 3; i++) {
        EXPECT_FLOAT_EQ(pockle().palette_target[i], ambient[i] + expanded::kPockleTint[i]);
    }
    EXPECT_EQ(pockle().palette_cycles, 1);
    EXPECT_FLOAT_EQ(pockle().palette_step, -1.0f);
    Step(16);
    EXPECT_FLOAT_EQ(body().scale[1], expanded::kPockleGrowth[16]);
    Step(10);
    EXPECT_FLOAT_EQ(body().scale[2], 1.0f);
}

TEST_F(Pocklekul, ItsNoticeWaitsForTheDungeonsLine) {
    int saved_line = DngMessMan.message;
    SystemMesCount = 0;
    SystemMesNo = -1;
    DngMessMan.message = 4000; // Steve talking
    Summon();
    Step(10);
    EXPECT_NE(SystemMesNo, expanded::kPocklekulMessage);
    DngMessMan.message = -1;
    Step();
    EXPECT_EQ(SystemMesNo, expanded::kPocklekulMessage);
    EXPECT_EQ(SystemMesCount, expanded::kPockleNoticeFrames);
    DngMessMan.message = saved_line;
    SystemMesCount = 0;
}

TEST_F(Pocklekul, ItsNoticeWaitsNoLongerThanItsWait) {
    int saved_line = DngMessMan.message;
    SystemMesCount = 0;
    SystemMesNo = -1;
    DngMessMan.message = 4000;
    Summon();
    Step(expanded::kPockleNoticeWait - 1);
    EXPECT_NE(SystemMesNo, expanded::kPocklekulMessage);
    Step();
    EXPECT_EQ(SystemMesNo, expanded::kPocklekulMessage);
    DngMessMan.message = saved_line;
    SystemMesCount = 0;
}

// ---- Its end ----

TEST_F(Pocklekul, GoesWhenThePocklekulLeavesTheHand) {
    Summon();
    ASSERT_TRUE(expanded::PockleUp());
    wielding_.weapon().item_no = ITEM_WEAPON_GOLD_RING;
    Step();
    EXPECT_FALSE(expanded::PockleUp());
    EXPECT_EQ(expanded::AllyStanding(), nullptr);
}

TEST_F(Pocklekul, GoesWhenTheSummonerChanges) {
    Summon();
    floor_.status().cur_chara = CHARA_TOAN;
    Step();
    EXPECT_FALSE(expanded::PockleUp());
}

TEST_F(Pocklekul, GoesWhenLeftBehind) {
    Summon();
    Place(0.0f, expanded::kPockleLeash - 1.0f);
    Step();
    ASSERT_TRUE(expanded::PockleUp());
    Place(0.0f, expanded::kPockleLeash + 1.0f);
    Step();
    EXPECT_FALSE(expanded::PockleUp());
}

TEST_F(Pocklekul, GoesOnceItHasFallen) {
    Summon();
    pockle().hp = 0;
    Step(5);
    EXPECT_TRUE(expanded::PockleUp()); // its death plays out
    pockle().state = -1;
    Step();
    EXPECT_FALSE(expanded::PockleUp());
}

TEST_F(Pocklekul, GoesWithAnEvent) {
    Summon();
    BtEventMode = 1;
    Step();
    BtEventMode = 0;
    EXPECT_FALSE(expanded::PockleUp());
}

// ---- Its enemy ----

TEST_F(Pocklekul, GoesForTheLockOnCursorsMonster) {
    Stand(0, 0.0f, 150.0f);
    Stand(1, 60.0f, 0.0f);
    lockOnTargetNo = 0;
    Summon();
    Step();
    float target[4];
    Target(target);
    // 8 in from its sphere's edge toward the pockle: the sphere (radius 5) is smaller, so its centre.
    EXPECT_NEAR(target[0], 0.0f, 1e-3f);
    EXPECT_NEAR(target[2], 150.0f, 1e-3f);
}

TEST_F(Pocklekul, GoesForTheNearestWithoutOne) {
    Stand(0, 0.0f, 140.0f);
    Stand(1, 60.0f, 0.0f);
    Summon();
    Step();
    float target[4];
    Target(target);
    EXPECT_NEAR(target[0], 60.0f, 1e-3f);
    EXPECT_NEAR(target[2], 0.0f, 1e-3f);
}

TEST_F(Pocklekul, AimsInsideABigBody) {
    Stand(0, 0.0f, 100.0f, 20.0f);
    lockOnTargetNo = 0;
    Summon();
    Place(0.0f, 50.0f);
    Step();
    float target[4];
    Target(target);
    // kPockleAimDepth in from the edge toward the pockle: 12 out of the centre.
    EXPECT_NEAR(target[0], 0.0f, 1e-3f);
    EXPECT_NEAR(target[2], 88.0f, 1e-3f);
}

TEST_F(Pocklekul, AMonsterTooFarIsNone) {
    Stand(0, 0.0f, expanded::kPockleNearRange + 10.0f);
    Summon();
    EXPECT_EQ(ally().script_state[kAllyMonster], 1);
    ASSERT_NE(ally().interpreter[kAllyMonster].pc, nullptr);
    EXPECT_EQ(ally().interpreter[kAllyMonster].pc->op, RS_OP_WAIT);
}

// ---- No enemy ----

TEST_F(Pocklekul, WalksToTheSummonersRight) {
    Summon();
    // Its script waits, resumed on a wait.
    EXPECT_EQ(ally().script_state[kAllyMonster], 1);
    EXPECT_EQ(ally().interpreter[kAllyMonster].pc->op, RS_OP_WAIT);
    EXPECT_EQ(pockle().movement_speed, 0.0f);
    // From (0, 15) toward (-16, 0): its pace d × 0.08, a step of that.
    float distance = std::hypot(16.0f, 15.0f), pace = distance * expanded::kPockleWalkPace;
    EXPECT_NEAR(PockleAt(0), -16.0f * pace / distance, 1e-4f);
    EXPECT_NEAR(PockleAt(2), 15.0f - 15.0f * pace / distance, 1e-4f);
    EXPECT_EQ(body().motion_no, expanded::kPockleWalkClip);
    EXPECT_NEAR(body().motion_speed, pace * expanded::kPockleWalkRate, 1e-5f);
    EXPECT_NEAR(pockle().turn_target[0], -16.0f, 1e-4f);
    EXPECT_NEAR(pockle().turn_target[2], 0.0f, 1e-4f);
    EXPECT_FLOAT_EQ(pockle().turn_speed, expanded::kPockleTurnRate);
    float target[4];
    Target(target);
    EXPECT_NEAR(target[0], -16.0f, 1e-4f);
    EXPECT_NEAR(target[2], 0.0f, 1e-4f);
}

TEST_F(Pocklekul, RunsWhileTheSummonerRuns) {
    CharaMain.motion_no = expanded::kSummonerRunMotion;
    Summon();
    EXPECT_EQ(body().motion_no, expanded::kPockleRunClip);
    EXPECT_FLOAT_EQ(body().motion_speed, -1.0f);
}

TEST_F(Pocklekul, IdlesBesideTheSummoner) {
    Summon();
    Place(-15.0f, 0.0f);
    Step();
    EXPECT_EQ(body().motion_no, expanded::kPockleIdleClip);
    EXPECT_FLOAT_EQ(body().motion_speed, -1.0f);
    EXPECT_NEAR(PockleAt(0), -15.0f, 1e-4f);
    // Turned the summoner's way.
    EXPECT_NEAR(pockle().turn_target[0], -15.0f, 1e-4f);
    EXPECT_NEAR(pockle().turn_target[2], expanded::kPockleIdleLook, 1e-4f);
}

TEST_F(Pocklekul, FinishesAMoveOfItsOwnFirst) {
    stood_clip = 5;
    Summon();
    EXPECT_EQ(ally().script_state[kAllyMonster], 0); // its script still its own
    float target[4];
    Target(target);
    EXPECT_NEAR(target[2], 15.0f - expanded::kPockleSettleReach * 15.0f, 1e-3f);
    Step(expanded::kPockleSettleFrames - 2);
    EXPECT_EQ(ally().script_state[kAllyMonster], 0);
    Step();
    EXPECT_EQ(ally().script_state[kAllyMonster], 1);
    EXPECT_EQ(ally().interpreter[kAllyMonster].pc->op, RS_OP_WAIT);
}

TEST_F(Pocklekul, AnEnemyStartsItsScriptAfresh) {
    Summon();
    ASSERT_EQ(ally().script_state[kAllyMonster], 1);
    Stand(0, 0.0f, 60.0f);
    Step();
    EXPECT_EQ(ally().script_state[kAllyMonster], 0);
    EXPECT_EQ(ally().interpreter[kAllyMonster].pc, nullptr);
    EXPECT_FLOAT_EQ(body().motion_speed, -1.0f);
}

// ---- Its fights ----

TEST_F(Pocklekul, ItsSwingIsAHitOfTheSummoners) {
    Stand(0, 0.0f, 100.0f);
    Summon();
    int swing = Swing(0.0f, 5.0f, 97.0f, 3.0f, 20, 200, expanded::kHitsPlayer);
    expanded::MonsterSwingSet(ally(), kAllyMonster, swing);
    EXPECT_EQ(NowColData->active[swing], 0); // it strikes no one itself
    const COLLISION_HIT *planted = PlantedHit();
    ASSERT_NE(planted, nullptr);
    // At the Mummy's sphere, 100 from Ruby: CheckDmg halves a hit of hers there, so it is planted doubled.
    EXPECT_EQ(planted->damage, 40);
    EXPECT_FLOAT_EQ(planted->radius, expanded::pocklekul::kPockleHitRadius);
    EXPECT_FLOAT_EQ(planted->pos[2], 100.0f);
    EXPECT_TRUE(expanded::MonsterHitRefused(ally(), kAllyMonster, *planted));
    EXPECT_FALSE(expanded::MonsterHitRefused(floor(), 0, *planted));
    EXPECT_TRUE(expanded::MonsterHitRefused(floor(), 1, *planted)); // meant for the Mummy in slot 0 alone
    // One a monster every kPockleContactFrames.
    swing = Swing(0.0f, 5.0f, 97.0f, 3.0f, 20, 200, expanded::kHitsPlayer);
    expanded::MonsterSwingSet(ally(), kAllyMonster, swing);
    EXPECT_EQ(Planted(), 1);
    Step(expanded::pocklekul::kPockleContactFrames);
    swing = Swing(0.0f, 5.0f, 97.0f, 3.0f, 20, 200, expanded::kHitsPlayer);
    expanded::MonsterSwingSet(ally(), kAllyMonster, swing);
    EXPECT_EQ(Planted(), 2);
}

TEST_F(Pocklekul, ItsSwingMissesAMonsterOutOfReach) {
    Stand(0, 0.0f, 100.0f);
    Summon();
    int swing = Swing(0.0f, 5.0f, 90.0f, 3.0f, 20, 200, expanded::kHitsPlayer);
    expanded::MonsterSwingSet(ally(), kAllyMonster, swing);
    EXPECT_EQ(Planted(), 0);
}

TEST_F(Pocklekul, ItsStealingSwingTakesTheMonstersMoney) {
    Stand(0, 0.0f, 100.0f);
    Summon();
    for (int i = 0; i < 200 && pockle().stolen_money == 0; i++) {
        Step(expanded::pocklekul::kPockleContactFrames);
        int swing = Swing(0.0f, 5.0f, 97.0f, 3.0f, 20, 200, expanded::kHitsPlayer, expanded::pocklekul::kPockleStealGold);
        expanded::MonsterSwingSet(ally(), kAllyMonster, swing);
    }
    EXPECT_EQ(pockle().stolen_money, 30);
    EXPECT_EQ(floor().monster[0].money, 0);
    EXPECT_EQ(floor().monster[0].money_chance, 0);
}

TEST_F(Pocklekul, AMonstersSwingHurtsIt) {
    Stand(0, 0.0f, 40.0f);
    Summon();
    ally().current_monster = kAllyMonster;
    Swing(0.0f, 5.0f, 18.0f, 3.0f, 20, 200, expanded::kHitsPlayer);
    int life = pockle().hp;
    EXPECT_EQ(expanded::MonsterFriendlyHit(ally(), 0), MONSTER_DMG_HIT);
    EXPECT_EQ(pockle().hp, life - std::max(1, 20 - pockle().defense));
    // One an attacker every kPockleContactFrames.
    EXPECT_EQ(expanded::MonsterFriendlyHit(ally(), 0), -1);
}

TEST_F(Pocklekul, ItsGuardBlocksAMonstersSwing) {
    Stand(0, 0.0f, 40.0f);
    Summon();
    ally().current_monster = kAllyMonster;
    ally().guard[kAllyMonster].active[0] = 1;
    ally().guard[kAllyMonster].motion_start[0] = 0.0f;
    ally().guard[kAllyMonster].motion_end[0] = 100.0f;
    body().motion_type.state.time = 10.0f;
    Swing(0.0f, 5.0f, 18.0f, 3.0f, 20, 200, expanded::kHitsPlayer);
    int life = pockle().hp;
    EXPECT_EQ(expanded::MonsterFriendlyHit(ally(), 0), MONSTER_DMG_NONE);
    EXPECT_EQ(pockle().hp, life);
}

TEST_F(Pocklekul, AMonstersShotStrikesIt) {
    Stand(0, 0.0f, 40.0f);
    Summon();
    ally().current_monster = kAllyMonster;
    int shot = Swing(0.0f, 5.0f, 17.0f, 2.0f, 15, -1, expanded::kHitsPlayer);
    NowColData->hit[shot].monster_no = 0;
    int life = pockle().hp;
    EXPECT_EQ(expanded::MonsterFriendlyHit(ally(), 0), MONSTER_DMG_HIT);
    EXPECT_EQ(pockle().hp, life - std::max(1, 15 - pockle().defense));
    EXPECT_EQ(NowColData->active[shot], 0);
}

TEST_F(Pocklekul, ItsOwnShotsAreNotAMonsters) {
    Stand(0, 0.0f, 40.0f);
    Summon();
    ally().current_monster = kAllyMonster;
    int shot = Swing(0.0f, 5.0f, 17.0f, 2.0f, 15, -1, expanded::kHitsPlayer);
    NowColData->hit[shot].monster_no = expanded::kAllyShooter;
    EXPECT_EQ(expanded::MonsterFriendlyHit(ally(), 0), -1);
}

TEST_F(Pocklekul, ItsShotIsAHitOfTheSummoners) {
    Stand(0, 0.0f, 100.0f);
    Summon();
    // Its shot in flight, 3 from the Mummy's sphere: the description's flying radius reaches it.
    BT_SHOT_EFFECT dart = {
        "dart", 0, 1, {4.0f, 4.0f, 0.0f, 0.0f},
           {3.0f, 3.0f, 3.0f, 0.0f},
           180, 31, 0, 2, expanded::kHitsPlayer, {-1,   -1,   -1,   -1  },
           -1
    };
    CSHOT_EFFECT &effect = NowShotEffect->effect[0];
    effect.effect_data = &dart;
    effect.slot_count = 1;
    effect.active[0] = 1;
    effect.phase[0] = 1;
    effect.damage[0] = 31;
    effect.user_id_2[0] = expanded::kAllyShooter;
    effect.chara[0].SetPosition(0.0f, 5.0f, 92.0f);
    expanded::PocklekulShotsStep();
    const COLLISION_HIT *planted = PlantedHit();
    ASSERT_NE(planted, nullptr);
    EXPECT_EQ(planted->damage, 62); // doubled against CheckDmg's halving at 100 from Ruby
    EXPECT_EQ(effect.active[0], 0); // struck there
    effect.Initialize();
}

TEST_F(Pocklekul, AMonstersShotIsNotItsToGive) {
    Stand(0, 0.0f, 100.0f);
    Summon();
    BT_SHOT_EFFECT dart = {
        "dart", 0, 1, {4.0f, 4.0f, 0.0f, 0.0f},
           {3.0f, 3.0f, 3.0f, 0.0f},
           180, 31, 0, 2, expanded::kHitsPlayer, {-1,   -1,   -1,   -1  },
           -1
    };
    CSHOT_EFFECT &effect = NowShotEffect->effect[0];
    effect.effect_data = &dart;
    effect.slot_count = 1;
    effect.active[0] = 1;
    effect.phase[0] = 1;
    effect.user_id_2[0] = 1; // floor monster 1's
    effect.chara[0].SetPosition(0.0f, 5.0f, 92.0f);
    expanded::PocklekulShotsStep();
    EXPECT_EQ(Planted(), 0);
    EXPECT_EQ(effect.active[0], 1);
    effect.Initialize();
}

TEST_F(Pocklekul, AMonsterItHurtsGoesAfterIt) {
    Stand(0, 0.0f, 100.0f);
    Summon();
    int swing = Swing(0.0f, 5.0f, 97.0f, 3.0f, 20, 200, expanded::kHitsPlayer);
    expanded::MonsterSwingSet(ally(), kAllyMonster, swing);
    const COLLISION_HIT *planted = PlantedHit();
    ASSERT_NE(planted, nullptr);
    float target[4];
    FloorTarget(0, target);
    EXPECT_NEAR(target[2], 0.0f, 1e-4f); // not before its life drops
    expanded::MonsterDamaged(floor(), 0, *planted, 0);
    FloorTarget(0, target);
    EXPECT_NEAR(target[2], 0.0f, 1e-4f);
    expanded::MonsterDamaged(floor(), 0, *planted, 10);
    FloorTarget(0, target);
    EXPECT_NEAR(target[0], PockleAt(0), 1e-4f);
    EXPECT_NEAR(target[2], PockleAt(2), 1e-4f);
    // Back on the player as the pockle goes.
    expanded::PocklekulReset();
    FloorTarget(0, target);
    EXPECT_NEAR(target[2], 0.0f, 1e-4f);
}

TEST_F(Pocklekul, ItsBodyStopsAMonsterThatThenGoesAfterIt) {
    Stand(1, 0.0f, 25.0f);
    Summon();
    MONSTOR &mummy = floor().monster[1];
    mummy.movement[0] = 0.0f;
    mummy.movement[2] = -1.0f;
    mummy.movement_speed = 2.0f;
    expanded::MonsterMoveChecked(floor(), 1);
    EXPECT_EQ(mummy.movement_speed, 0.0f);
    float target[4];
    FloorTarget(1, target);
    EXPECT_NEAR(target[2], PockleAt(2), 1e-4f);
}

TEST_F(Pocklekul, TheFloorsFeaturesPassItBy) {
    Stand(0, 0.0f, 100.0f);
    Summon();
    expanded::ScaleMonster(floor(), 0, {.melee = 2.0f});
    CMonstorUnit *saved = NowMonstorUnit;
    NowMonstorUnit = &ally();
    EXPECT_EQ(expanded::EnemyMeleeDamage(kAllyMonster, 10), 10);
    EXPECT_FALSE(expanded::IsKind(ally(), kAllyMonster, static_cast<Monster>(loaded_kind)));
    EXPECT_FALSE(expanded::MonsterKilled(ally(), kAllyMonster));
    NowMonstorUnit = saved;
}

// ---- Who summons ----

TEST_F(PocklekulBySuperSteve, SummonsFromXiaosGuard) {
    Summon();
    EXPECT_TRUE(expanded::PockleUp());
}

TEST_F(PocklekulOtherWeapon, SummonsNothing) {
    Summon();
    EXPECT_FALSE(expanded::PockleUp());
    EXPECT_EQ(loads, 0);
}

TEST_F(Pocklekul, SummonsNothingWithWeaponAbilitiesOff) {
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Summon();
    EXPECT_FALSE(expanded::PockleUp());
}

} // namespace
