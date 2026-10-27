#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "battlemenu.hpp"
#include "bt_shot_effect.hpp"
#include "clsmes.hpp"
#include "collisiondata.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "dungeoneventdata.hpp"
#include "dungeoneventman.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/ruby/crystal_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "frame.hpp"
#include "gametext.hpp"
#include "itemdata.hpp"
#include "main.hpp"
#include "menu_dungeon_port.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

extern s32          mode;
extern s32          lockOnTargetNo;
extern CSHOT_EFFECT CharaMainEffect;

namespace {

using expanded::Monster;

constexpr int kRing = ITEM_WEAPON_CRYSTAL_RING;

// The arrows' KEYs and a frame for each of their shots.
std::vector<MOTION_INFO> g_motions(4, MOTION_INFO{.start = 10, .end = 30, .speed = 0.2f});
std::deque<CFrame>       g_frames;

bool FakeLoad(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    for (int slot = 0; slot < shots; slot++) {
        effect.chara[slot].motion_type.motion_info = g_motions.data();
        effect.chara[slot].frame = &g_frames.emplace_back();
    }
    return true;
}

// The angle in degrees between a flight and the way from a point to aim.
float Off(const float *flight, const float *from, const float *aim) {
    float to[3] = {aim[0] - from[0], aim[1] - from[1], aim[2] - from[2]};
    float dot = flight[0] * to[0] + flight[1] * to[1] + flight[2] * to[2];
    float lengths = std::sqrt(flight[0] * flight[0] + flight[1] * flight[1] + flight[2] * flight[2]) *
                    std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    return std::acos(std::clamp(dot / lengths, -1.0f, 1.0f)) * 180.0f / PI;
}

float Speed(const float *flight) {
    return std::sqrt(flight[0] * flight[0] + flight[1] * flight[1] + flight[2] * flight[2]);
}

// Ruby out on a dungeon floor with weapon in hand (in the bag's first slot, and its copy in hand), the ring's arrows
// loaded, her orbs her own, no monster set out and no crystal on the floor. Everything put back after.
class CrystalRing : public ::testing::Test {
protected:
    explicit CrystalRing(int weapon_no = kRing, int chara = CHARA_RUBY) : weapon_no_(weapon_no), chara_(chara) {}

    void SetUp() override {
        mode = GAME_MODE_DUNGEON;
        NowColData = hits_.get();
        NowEventMan = events_.get();
        CharaMainEffect.Initialize();
        CharaMainEffect.effect_data = &orb_;
        CharaMainEffect.slot_count = 6;
        NowMainEffect = &CharaMainEffect;
        BtRubyDoorKey = WEAPON_ELEMENT_WIND;
        lockOnTargetFlag = 0;
        status().cur_chara = static_cast<s8>(chara_);
        for (int chara = 0; chara < 6; chara++) {
            status().hp[chara] = status().max_hp[chara] = 60;
        }
        Equip(CHARA_RUBY, weapon_no_);
        if (chara_ == CHARA_RUBY) {
            wielding_.weapon() = status().chara_weapons[CHARA_RUBY][0];
        }
        expanded::CursedPrismFloorEnds();
        expanded::SetShotEffectLoader(FakeLoad);
        expanded::ShotEffectsClear();
        expanded::ShotEffectsFloorLoaded();
        expanded::CurseStep();
        arrows_ = expanded::PrismArrows();
    }

    void TearDown() override {
        for (int chara = 0; chara < 6; chara++) {
            status().equipped_weapon_slot[chara] = -1;
        }
        expanded::CurseStep();
        expanded::CursedPrismFloorEnds();
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        CharaMainEffect.Initialize();
        NowMainEffect = saved_live_;
        NowEventMan = saved_events_;
        NowColData = saved_hits_;
        lockOnTargetFlag = saved_lock_flag_;
        lockOnTargetNo = saved_lock_no_;
        BtRubyDoorKey = saved_key_;
        mode = saved_mode_;
    }

    CUserStatus &status() { return floor_.status(); }

    void Equip(int chara, int weapon_no, int sphere = 0) {
        status().equipped_weapon_slot[chara] = 0;
        WEAPON_HAVE &record = status().chara_weapons[chara][0];
        record = {};
        record.item_no = static_cast<s16>(weapon_no);
        record.durability = 100;
        record.durability_f = 100.0f;
        record.best_elem = WEAPON_ELEMENT_HOLY;
        if (sphere != 0) {
            record.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
            record.attach[0].sphere_weapon_no = static_cast<s16>(sphere);
        }
    }

    // An item lifting her curse, as the game's steps then see it.
    void Lift() {
        status().ailments[CHARA_RUBY] &= ~AILMENT_CURSE;
        expanded::CurseStep();
    }

    // An orb of hers flying from (0, 8, 0) along flight for damage, let go into slot 0 as BattleActionShotRuby does: scale
    // the throw's charge, repeat a full charge's second orb.
    void Throw(float x, float y, float z, int damage, float scale = 1.0f, bool repeat = false) {
        CharaMainEffect.chara[0].SetPosition(0.0f, 8.0f, 0.0f);
        CharaMainEffect.velocity[0][0] = x;
        CharaMainEffect.velocity[0][1] = y;
        CharaMainEffect.velocity[0][2] = z;
        CharaMainEffect.phase[0] = 1;
        CharaMainEffect.active[0] = 1;
        CharaMainEffect.damage[0] = damage;
        CharaMainEffect.weapon_status[0] = 0x40;
        CharaMainEffect.vs_monster[0] = vs_monster_;
        CharaMainEffect.current_slot = 0;
        expanded::RubyOrbThrown(CharaMainEffect, 0, scale, repeat);
    }

    // A monster of kind set out at (x, 0, z) in monster slot.
    void SetOut(int monster, float x, float z) {
        floor_.SetUpModel(monster, Monster::MASTER_JACKET);
        floor_.Spawn(monster, monster);
        floor_.unit().monster[monster].revealed = -1;
        floor_.unit().chara[monster][0].SetPosition(x, 0.0f, z);
    }

    // A door crystal answering to chara at (x, 0, z), switched on or off, in event slot.
    void Crystal(int slot, float x, float z, int chara = CHARA_RUBY, bool on = true) {
        definitions_[slot] = {};
        definitions_[slot].chara_no = chara;
        definitions_[slot].key_id = BtRubyDoorKey;
        definitions_[slot].radius = 3.0f;
        CDungeonEventData &event = events_->event[slot];
        event.event = &definitions_[slot];
        event.enabled = 1;
        event.switch_on = on ? 1 : 0;
        event.pos[0] = x;
        event.pos[1] = 0.0f;
        event.pos[2] = z;
        event.pos[3] = 1.0f;
    }

    // An arrow in slot flying from (0, 8, 0) along +z.
    void Fly(int slot) {
        arrows_->active[slot] = 1;
        arrows_->phase[slot] = 1;
        arrows_->chara[slot].SetPosition(0.0f, 8.0f, 0.0f);
        arrows_->velocity[slot][0] = 0.0f;
        arrows_->velocity[slot][1] = 0.0f;
        arrows_->velocity[slot][2] = 1.2f;
    }

    void Steps(int frames) {
        for (int i = 0; i < frames; i++) {
            expanded::CursedPrismStep(floor_.unit());
        }
    }

    int                               weapon_no_;
    int                               chara_;
    expanded::test::TestFloor         floor_;
    expanded::test::Wielding          wielding_{weapon_no_};
    BT_SHOT_EFFECT                    orb_ = {};
    char                              vs_monster_[10] = {};
    std::unique_ptr<CCollisionData>   hits_ = std::make_unique<CCollisionData>();
    std::unique_ptr<CDungeonEventMan> events_ = std::make_unique<CDungeonEventMan>();
    CDungeonEvent                     definitions_[4] = {};
    CSHOT_EFFECT                     *arrows_ = nullptr;
    CSHOT_EFFECT                     *saved_live_ = NowMainEffect;
    CDungeonEventMan                 *saved_events_ = NowEventMan;
    CCollisionData                   *saved_hits_ = NowColData;
    s32                               saved_lock_flag_ = lockOnTargetFlag;
    s32                               saved_lock_no_ = lockOnTargetNo;
    int                               saved_key_ = BtRubyDoorKey;
    s32                               saved_mode_ = mode;
};

class CrystalRingWithAnotherWeapon : public CrystalRing {
protected:
    CrystalRingWithAnotherWeapon() : CrystalRing(ITEM_WEAPON_GOLD_RING) {}
};

class CrystalRingOnAnotherCharacter : public CrystalRing {
protected:
    CrystalRingOnAnotherCharacter() : CrystalRing(kRing, CHARA_XIAO) {}
};

} // namespace

TEST_F(CrystalRing, CursesRubyAndLiftingItCostsNothing) {
    EXPECT_TRUE(status().ailments[CHARA_RUBY] & AILMENT_CURSE);
    EXPECT_TRUE(expanded::CursedPrismActive());
    // Not immune to the other ailments, as the Evilcise's curse is.
    EXPECT_TRUE(expanded::CurseAllowsAilment(AILMENT_FREEZE));
    status().hp[CHARA_RUBY] = 55;
    Lift();
    expanded::CurseStep();
    EXPECT_EQ(status().ailments[CHARA_RUBY], 0);
    EXPECT_EQ(status().hp[CHARA_RUBY], 55);
    EXPECT_FALSE(expanded::CursedPrismActive());
}

TEST_F(CrystalRing, HoldsTheRingToIce) {
    Equip(CHARA_RUBY, kRing);
    status().chara_weapons[CHARA_RUBY][3] = status().chara_weapons[CHARA_RUBY][0];
    status().chara_weapons[CHARA_RUBY][3].best_elem = WEAPON_ELEMENT_FIRE;
    status().chara_weapons[CHARA_RUBY][1].item_no = ITEM_WEAPON_GOLD_RING;
    status().chara_weapons[CHARA_RUBY][1].best_elem = WEAPON_ELEMENT_FIRE;
    wielding_.weapon().best_elem = WEAPON_ELEMENT_HOLY;
    Steps(1);
    EXPECT_EQ(status().chara_weapons[CHARA_RUBY][0].best_elem, WEAPON_ELEMENT_COLD);
    EXPECT_EQ(status().chara_weapons[CHARA_RUBY][3].best_elem, WEAPON_ELEMENT_COLD);
    EXPECT_EQ(wielding_.weapon().best_elem, WEAPON_ELEMENT_COLD);
    EXPECT_EQ(status().chara_weapons[CHARA_RUBY][1].best_elem, WEAPON_ELEMENT_FIRE);

    // The weapon menu refuses any other element, the picker offers Ice alone, and the 0-point warning is the curse's.
    const WEAPON_HAVE &ring = status().chara_weapons[CHARA_RUBY][0];
    EXPECT_EQ(expanded::HeldElement(ring), WEAPON_ELEMENT_COLD);
    EXPECT_FALSE(expanded::ElementRefused(ring, WEAPON_ELEMENT_COLD));
    for (int element : {WEAPON_ELEMENT_FIRE, WEAPON_ELEMENT_THUNDER, WEAPON_ELEMENT_WIND, WEAPON_ELEMENT_HOLY}) {
        EXPECT_TRUE(expanded::ElementRefused(ring, element)) << element;
    }
    EXPECT_FALSE(expanded::ElementRefused(status().chara_weapons[CHARA_RUBY][1], WEAPON_ELEMENT_FIRE));
}

TEST_F(CrystalRing, TheWeaponMenusWarningIsTheCursesOnTheRing) {
    GameTextFile file;
    ASSERT_EQ(file.Set(0xA1, "\nAttribute with 0 pt\ncannot be activated."), 0);
    const s16       *data = file.Data();
    std::vector<s16> bank(data, data + 64);
    expanded::MenuTextLoaded("allmenu.mes", bank.data());
    CDngStatusData *saved = BtlMenuStatusPt;
    BtlMenuStatusPt = static_cast<CDngStatusData *>(&status());
    WepMenu.chara = CHARA_RUBY;
    WepMenu.weapon_slot = 0;
    ClsMes mes;
    mes.SetBuff(bank.data());
    EXPECT_EQ(GameTextDecode(mes.GetTextLineDataTop(0xA1)), "\nElement can't be\nselected due to\ncurse.");
    status().chara_weapons[CHARA_RUBY][0].item_no = ITEM_WEAPON_GOLD_RING;
    EXPECT_EQ(GameTextDecode(mes.GetTextLineDataTop(0xA1)), "\nAttribute with 0 pt\ncannot be activated.");
    BtlMenuStatusPt = saved;
}

TEST_F(CrystalRing, ThePickerOpensOnIceAlone) {
    expanded::test::TestSave save;
    EXPECT_TRUE(ElementPickerHasChoice());
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(expanded::HeldElement(wielding_.weapon()), -1);
    EXPECT_FALSE(ElementPickerHasChoice());
}

TEST_F(CrystalRing, ALiftedCurseFreesTheElementUntilTheFloorEnds) {
    Lift();
    WEAPON_HAVE &ring = status().chara_weapons[CHARA_RUBY][0];
    EXPECT_EQ(expanded::HeldElement(ring), -1);
    ring.best_elem = WEAPON_ELEMENT_FIRE;
    Steps(1);
    EXPECT_EQ(ring.best_elem, WEAPON_ELEMENT_FIRE);
    // Out of the dungeon it is held again.
    mode = GAME_MODE_EDIT;
    EXPECT_EQ(expanded::HeldElement(ring), WEAPON_ELEMENT_COLD);
    mode = GAME_MODE_DUNGEON;

    // The floor ends: held, and back to Ice before the next floor loads her shot; the next floor curses her again.
    expanded::CursedPrismFloorEnds();
    EXPECT_EQ(ring.best_elem, WEAPON_ELEMENT_COLD);
    EXPECT_EQ(expanded::HeldElement(ring), WEAPON_ELEMENT_COLD);
    floor_.SetUpModel(0, Monster::MASTER_JACKET);
    expanded::CurseStep();
    EXPECT_TRUE(status().ailments[CHARA_RUBY] & AILMENT_CURSE);
    EXPECT_TRUE(expanded::CursedPrismActive());
}

TEST_F(CrystalRing, ALiftedCurseEndsWhenTheRingLeavesHerHand) {
    Lift();
    Equip(CHARA_RUBY, ITEM_WEAPON_GOLD_RING);
    status().chara_weapons[CHARA_RUBY][2].item_no = kRing;
    status().chara_weapons[CHARA_RUBY][2].best_elem = WEAPON_ELEMENT_FIRE;
    expanded::CurseStep();
    Steps(1);
    EXPECT_EQ(status().chara_weapons[CHARA_RUBY][2].best_elem, WEAPON_ELEMENT_COLD);
    EXPECT_EQ(expanded::HeldElement(status().chara_weapons[CHARA_RUBY][2]), WEAPON_ELEMENT_COLD);
}

TEST_F(CrystalRing, ABuildUpIntoTheRingTakesIce) {
    s32 saved = BtlMenuMode;
    BtlMenuMode = BATTLE_MENU_MODE_TOWN;
    WEAPON_HAVE built = {};
    built.item_no = kRing;
    built.best_elem = WEAPON_ELEMENT_FIRE;
    expanded::CrystalRingBuiltUp(built);
    EXPECT_EQ(built.best_elem, WEAPON_ELEMENT_COLD);
    built.item_no = ITEM_WEAPON_GOLD_RING;
    built.best_elem = WEAPON_ELEMENT_FIRE;
    expanded::CrystalRingBuiltUp(built);
    EXPECT_EQ(built.best_elem, WEAPON_ELEMENT_FIRE);
    BtlMenuMode = saved;
}

TEST_F(CrystalRing, EveryOrbFliesOnAsTheIceArrow) {
    ASSERT_NE(arrows_, nullptr);
    Throw(0.0f, 0.0f, 0.9f, 77);
    EXPECT_EQ(CharaMainEffect.active[0], 0);
    ASSERT_NE(arrows_->active[0], 0);
    EXPECT_EQ(arrows_->phase[0], 1);
    EXPECT_EQ(arrows_->damage[0], 77);
    EXPECT_EQ(arrows_->weapon_status[0], 0x40);
    EXPECT_EQ(arrows_->vs_monster[0], vs_monster_);
    EXPECT_EQ(arrows_->user_id[0], CHARA_RUBY);
    EXPECT_EQ(arrows_->user_sub_id[0], WEAPON_ELEMENT_WIND);
    EXPECT_EQ(arrows_->wait[0], expanded::kPrismStrikeWait);
    EXPECT_EQ(arrows_->phase_delay[0], 360);
    float at[4];
    arrows_->chara[0].GetPosition(at);
    EXPECT_FLOAT_EQ(at[1], 8.0f);
    EXPECT_FLOAT_EQ(arrows_->velocity[0][2], 1.2f);
    EXPECT_FLOAT_EQ(arrows_->velocity[0][0], 0.0f);
    // Turned on the monsters, with only its Ice element and nothing planted in flight.
    const BT_SHOT_EFFECT *arrow = arrows_->effect_data;
    EXPECT_EQ(arrow->target, expanded::kHitsMonsters);
    EXPECT_EQ(arrow->hit_flags, HIT_ELEMENT_COLD);
    EXPECT_FLOAT_EQ(arrow->radius[1], 0.0f);
    EXPECT_FLOAT_EQ(arrow->radius[2], 8.0f);

    // A full charge: the first orb's arrow, and the second orb, shimmering over it, gone with no arrow of its own.
    Throw(0.0f, 0.0f, 0.9f, 140, 2.2f);
    Throw(0.0f, 0.0f, 0.9f, 140, 2.2f, true);
    EXPECT_NE(arrows_->active[1], 0);
    EXPECT_EQ(arrows_->damage[1], 140);
    EXPECT_EQ(arrows_->active[2], 0);
    EXPECT_EQ(CharaMainEffect.active[0], 0);
}

TEST_F(CrystalRing, AChargedThrowsArrowFliesALittleFaster) {
    SetOut(0, 100.0f, 0.0f);
    Throw(0.0f, 0.0f, 0.9f, 60);        // a quick shot
    Throw(0.0f, 0.0f, 0.9f, 90, 1.5f);  // a part charge
    Throw(0.0f, 0.0f, 0.9f, 132, 2.2f); // a full charge
    EXPECT_NEAR(Speed(arrows_->velocity[0]), 1.2f, 1e-5f);
    EXPECT_NEAR(Speed(arrows_->velocity[1]), expanded::kPrismChargedSpeed, 1e-5f);
    EXPECT_NEAR(Speed(arrows_->velocity[2]), expanded::kPrismChargedSpeed, 1e-5f);
    // and keeps its speed as it turns.
    Steps(30);
    EXPECT_NEAR(Speed(arrows_->velocity[0]), 1.2f, 1e-5f);
    EXPECT_NEAR(Speed(arrows_->velocity[2]), expanded::kPrismChargedSpeed, 1e-5f);
}

TEST_F(CrystalRing, WithNoRoomLeftTheOrbFlies) {
    for (int slot = 0; slot < expanded::kPrismArrows; slot++) {
        Throw(0.0f, 0.0f, 0.9f, 10);
    }
    Throw(0.0f, 0.0f, 0.9f, 10);
    EXPECT_NE(CharaMainEffect.active[0], 0);
    // and so does a full charge's second orb with it.
    Throw(0.0f, 0.0f, 0.9f, 10, 2.2f, true);
    EXPECT_NE(CharaMainEffect.active[0], 0);
}

TEST_F(CrystalRing, LiftedOrBrokenTheOrbsAreHerOwn) {
    Lift();
    Throw(0.0f, 0.0f, 0.9f, 77);
    EXPECT_NE(CharaMainEffect.active[0], 0);
    EXPECT_EQ(arrows_->active[0], 0);

    floor_.SetUpModel(0, Monster::MASTER_JACKET);
    expanded::CurseStep();
    NowMainEffect = &CharaMainEffectCrash;
    Throw(0.0f, 0.0f, 0.9f, 77);
    EXPECT_NE(CharaMainEffect.active[0], 0);
    EXPECT_EQ(arrows_->active[0], 0);
}

TEST_F(CrystalRing, AnArrowTurnsNinetyDegreesASecondTowardTheNearestMonster) {
    SetOut(0, 100.0f, 0.0f);
    SetOut(1, -150.0f, 0.0f);
    Fly(0);
    const float aim[3] = {100.0f, expanded::kPrismAimLift, 0.0f};
    float       at[4];
    arrows_->chara[0].GetPosition(at);
    Steps(30);
    EXPECT_NEAR(Off(arrows_->velocity[0], at, aim), 45.0f, 0.05f);
    EXPECT_NEAR(Speed(arrows_->velocity[0]), 1.2f, 1e-5f);
    Steps(60);
    EXPECT_LE(Off(arrows_->velocity[0], at, aim), expanded::kPrismTurnDegrees + 0.01f);
}

TEST_F(CrystalRing, AMissComesRound) {
    SetOut(0, -2.0f, -100.0f);
    Fly(0);
    const float aim[3] = {-2.0f, expanded::kPrismAimLift, -100.0f};
    float       at[4];
    arrows_->chara[0].GetPosition(at);
    Steps(60);
    EXPECT_NEAR(Off(arrows_->velocity[0], at, aim), 180.0f - 90.0f - 1.15f, 0.2f);
    Steps(60);
    EXPECT_LE(Off(arrows_->velocity[0], at, aim), expanded::kPrismTurnDegrees + 0.01f);
}

TEST_F(CrystalRing, TheLockedOnMonsterIsEveryArrowsTarget) {
    SetOut(0, 100.0f, 0.0f);
    SetOut(1, -300.0f, 0.0f);
    lockOnTargetFlag = 1;
    lockOnTargetNo = 1;
    Fly(0);
    Steps(30);
    EXPECT_LT(arrows_->velocity[0][0], 0.0f);
}

TEST_F(CrystalRing, AMonsterOutOfReachIsNoTarget) {
    SetOut(0, expanded::kPrismMonsterReach + 1.0f, 0.0f);
    Fly(0);
    Steps(10);
    EXPECT_FLOAT_EQ(arrows_->velocity[0][0], 0.0f);
    EXPECT_FLOAT_EQ(arrows_->velocity[0][2], 1.2f);
}

TEST_F(CrystalRing, WithNoMonsterNearArrowsHomeOnHerDoorCrystalPlantingAllTheWay) {
    Crystal(0, 0.0f, 0.0f, -1);
    Crystal(1, -50.0f, 0.0f, CHARA_RUBY, false);
    Crystal(2, 50.0f, 0.0f);
    Fly(0);
    Steps(1);
    EXPECT_GT(arrows_->velocity[0][0], 0.0f);
    EXPECT_FLOAT_EQ(arrows_->effect_data->radius[1], expanded::kPrismCrystalRadius);
    EXPECT_EQ(arrows_->wait[0], 0);
    // An arrow fired now plants all the way too.
    Throw(0.0f, 0.0f, 0.9f, 10);
    EXPECT_EQ(arrows_->wait[1], 0);

    // A monster near an arrow: the strike is its one hit again.
    SetOut(0, 0.0f, 60.0f);
    Steps(1);
    EXPECT_FLOAT_EQ(arrows_->effect_data->radius[1], 0.0f);
    EXPECT_EQ(arrows_->wait[0], expanded::kPrismStrikeWait);
}

TEST_F(CrystalRing, ItsHitOpensADoorCrystalOfAnyColour) {
    Crystal(0, 0.0f, 10.0f);
    Throw(0.0f, 0.0f, 0.9f, 77);
    ASSERT_NE(arrows_->active[0], 0);
    // Her own orb carries her element (Ice); the crystals take the floor's key (Wind).
    float at[4] = {0.0f, 0.0f, 9.0f, 1.0f};
    ASSERT_NE(NowColData->Set(at, 77, 2, 1.0f, 1.0f, expanded::kHitsMonsters, 2, HIT_ELEMENT_COLD, 0), -1);
    NowColData->SetUserID(CHARA_RUBY, WEAPON_ELEMENT_COLD);
    EXPECT_EQ(NowEventMan->CheckCollisionDataHit(0), nullptr);
    // The arrow's, planted as its step plants it, opens it.
    ASSERT_NE(NowColData->Set(at, 77, 2, 1.0f, 1.0f, expanded::kHitsMonsters, 2, HIT_ELEMENT_COLD, 0), -1);
    NowColData->SetUserID(arrows_->user_id[0], arrows_->user_sub_id[0]);
    EXPECT_EQ(NowEventMan->CheckCollisionDataHit(0), &NowEventMan->event[0]);
}

TEST_F(CrystalRingWithAnotherWeapon, NoCurseNoArrowsNoLock) {
    EXPECT_EQ(status().ailments[CHARA_RUBY] & AILMENT_CURSE, 0);
    EXPECT_FALSE(expanded::CursedPrismActive());
    Throw(0.0f, 0.0f, 0.9f, 77);
    EXPECT_NE(CharaMainEffect.active[0], 0);
    EXPECT_EQ(expanded::HeldElement(status().chara_weapons[CHARA_RUBY][0]), -1);
}

TEST_F(CrystalRingOnAnotherCharacter, CursedButNotOutHerOrbsAreNotThrown) {
    // The curse stays on her while another member is out; the arrows are hers alone.
    EXPECT_TRUE(status().ailments[CHARA_RUBY] & AILMENT_CURSE);
    EXPECT_FALSE(expanded::CursedPrismActive());
}

TEST_F(CrystalRing, SuperStevesSphereHasNoForm) {
    Equip(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, kRing);
    expanded::CurseStep();
    EXPECT_EQ(status().ailments[CHARA_XIAO] & AILMENT_CURSE, 0);
}

TEST_F(CrystalRing, WeaponAbilitiesOffItIsTheGamesRing) {
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    expanded::CurseStep();
    expanded::CurseStep();
    EXPECT_EQ(status().ailments[CHARA_RUBY] & AILMENT_CURSE, 0);
    EXPECT_FALSE(expanded::CursedPrismActive());
    EXPECT_EQ(expanded::HeldElement(status().chara_weapons[CHARA_RUBY][0]), -1);
    status().chara_weapons[CHARA_RUBY][0].best_elem = WEAPON_ELEMENT_FIRE;
    Steps(1);
    EXPECT_EQ(status().chara_weapons[CHARA_RUBY][0].best_elem, WEAPON_ELEMENT_FIRE);
}
