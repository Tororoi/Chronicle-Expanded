#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/weapons/kick_back.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/ungaga/babels_spear.hpp"
#include "expanded/weapons/ungaga/babels_spear/beam.hpp"
#include "expanded/weapons/ungaga/babels_spear/copy.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

namespace babel = expanded::babels_spear;
using expanded::test::StepsUnpaused;
using expanded::test::TestFloor;
using expanded::test::Wielding;

bool r1_held = false;

bool HeldR1() {
    return r1_held;
}

// The forms' characters, stood in for. The spear: a root with a weapon's grip (a half-turn about y). The statue: a root
// with Super Steve's grip (a half-turn about z), a bone 1 along its z and its fork bone 2 further, its pouch's one
// translate track resting on the catch pose.
struct SpearRig {
    CFrameVu1 frames[2];
};

struct StatueRig {
    CFrameVu1   frames[4];
    CFrameVu1  &root = frames[0], &chain = frames[1], &fork = frames[2], &pouch = frames[3];
    MOTION_INFO keys[32] = {};
    Mot_Key     track_keys[2] = {};
    Mot_List    track = {};
};

SpearRig                 spear_rig;
StatueRig                statue_rig;
std::vector<std::string> loaded; // the packs loaded, in order

bool Loaded(const std::string &path) {
    return std::count(loaded.begin(), loaded.end(), path) == 1;
}

void Frame(CFrameVu1 &frame, const char *name, CFrame *parent, float z) {
    frame.Initialize();
    std::strncpy(frame.name, name, sizeof(frame.name) - 1);
    frame.local[3][2] = z;
    if (parent != nullptr) {
        frame.SetParent(parent);
    }
}

bool FakeProp(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    loaded.emplace_back(path);
    if (std::string(path).find("c04w") == std::string::npos) {
        Frame(spear_rig.frames[0], "w10", nullptr, 0.0f);
        spear_rig.frames[0].local[0][0] = -1.0f;
        spear_rig.frames[0].local[2][2] = -1.0f;
        Frame(spear_rig.frames[1], "dcol0", &spear_rig.frames[0], 0.0f);
        chara.frame = &spear_rig.frames[0];
        return true;
    }
    StatueRig &rig = statue_rig;
    Frame(rig.root, "pati2", nullptr, 0.0f);
    rig.root.local[0][0] = -1.0f;
    rig.root.local[1][1] = -1.0f;
    Frame(rig.chain, "chn30", &rig.root, 1.0f);
    Frame(rig.fork, "eff30", &rig.chain, 2.0f);
    Frame(rig.pouch, "null24", &rig.chain, 0.0f);
    for (MOTION_INFO &key : rig.keys) {
        key = {1, 1, 1.0f, 0};
    }
    for (int i = 0; i < 2; i++) {
        rig.track_keys[i] = {};
        rig.track_keys[i].frame = i == 0 ? 0 : 254;
    }
    rig.track = {};
    rig.track.frame = 3; // the pouch
    rig.track.type = MOTION_KEY_TRANSLATION;
    rig.track.key_count = 2;
    rig.track.keys = rig.track_keys;
    chara.frame = &rig.root;
    chara.motion_type.motion_info = rig.keys;
    chara.motion_type.proc_list = &rig.track;
    return true;
}

// The beam, stood in for: one slot with its three clips.
CFrameVu1   beam_frames[2];
MOTION_INFO beam_keys[3] = {
    {10, 28, 0.5f, 0},
    {28, 48, 0.5f, 0},
    {48, 70, 0.5f, 0},
};
std::string beam_path;

bool FakeBeam(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *path) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    beam_path = path != nullptr ? path : "";
    for (CFrameVu1 &frame : beam_frames) {
        frame.Initialize();
    }
    beam_frames[1].SetParent(&beam_frames[0]);
    effect.chara[0].Initialize();
    effect.chara[0].frame = &beam_frames[0];
    effect.chara[0].motion_type.motion_info = beam_keys;
    return true;
}

// Ungaga (or Xiao) out with weapon_no (attack 120; carrying sphere when given), standing at the origin facing +z on a
// floor of no polygons, the monsters stepping; the forms' characters and the beam stood in for.
class CurseOfBabel : public ::testing::Test {
protected:
    explicit CurseOfBabel(int chara = CHARA_UNGAGA, int weapon_no = ITEM_WEAPON_BABEL_S_SPEAR, int sphere = 0)
        : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = chara_;
        wielding_.weapon().attack = 120;
        saved_event_ = BtEventMode;
        BtEventMode = 0;
        saved_map_ = NowDngMap;
        NowDngMap = nullptr;
        saved_weapon_ = NowWeapon;
        NowWeapon = &weapon_model_;
        saved_lock_flag_ = lockOnTargetFlag;
        saved_lock_no_ = lockOnTargetNo;
        lockOnTargetFlag = 0;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_data_ = NowColData;
        NowColData = data_.get();
        loaded.clear();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeProp);
        expanded::SetShotEffectLoader(FakeBeam);
        expanded::ShotEffectsClear();
        expanded::EnterShotEffect(babel::BeamEffect(), 1, expanded::kBabelBeamFile);
        expanded::SetCurseOfBabelGuardButton(HeldR1);
        expanded::ConfuseFloorReset(false);
        expanded::CurseOfBabelReset();
        r1_held = false;
    }

    void TearDown() override {
        expanded::CurseOfBabelReset();
        expanded::ConfuseFloorReset(false);
        expanded::SetCurseOfBabelGuardButton(nullptr);
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        NowColData = saved_data_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        lockOnTargetFlag = saved_lock_flag_;
        lockOnTargetNo = saved_lock_no_;
        NowWeapon = saved_weapon_;
        NowDngMap = saved_map_;
        BtEventMode = saved_event_;
        BtActStatus = {};
        r1_held = false;
    }

    void Guard(bool on) {
        r1_held = on;
        BtActStatus.guard_mode = on ? 5 : 0;
    }

    // The dungeon's step: the confusion counted, then the curse.
    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::ConfuseTick();
            expanded::CurseOfBabelStep();
        }
    }

    // Held long enough to raise one, then let go.
    void Raise() {
        Guard(true);
        Steps(expanded::kBabelGuardFrames + 1);
        Guard(false);
    }

    void RaiseTo(int age) {
        Raise();
        Steps(age);
    }

    void LockOn(int monster) {
        lockOnTargetFlag = 1;
        lockOnTargetNo = monster;
    }

    // monster standing at (x, 0, z), in play, with a body sphere of radius 3 at height.
    void Stand(int monster, float x, float z, float height = 5.0f) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = -1;
        live.hp = 1000;
        live.max_hp = 1000;
        live.collision_radius = 4.0f;
        CCharacter &body = floor_.unit().chara[monster][0];
        body.pos[0] = x;
        body.pos[2] = z;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.position[0][0] = x;
        effect.position[0][1] = height;
        effect.position[0][2] = z;
        effect.position[0][3] = 1.0f;
        effect.radius[0] = 3.0f;
        effect.timer[0] = 1;
    }

    void Move(int monster, float x, float z) {
        floor_.unit().chara[monster][0].pos[0] = x;
        floor_.unit().chara[monster][0].pos[2] = z;
        floor_.unit().effect[monster].position[0][0] = x;
        floor_.unit().effect[monster].position[0][2] = z;
    }

    bool Confused(int monster) { return expanded::MonsterConfused(floor_.unit(), monster); }

    // The planted hits, in order.
    std::vector<COLLISION_HIT *> Planted() {
        std::vector<COLLISION_HIT *> hits;
        for (int i = 0; i < 96; i++) {
            if (NowColData->active[i] != 0) {
                hits.push_back(&NowColData->hit[i]);
            }
        }
        return hits;
    }

    void ClearHits() {
        for (int &active : NowColData->active) {
            active = 0;
        }
    }

    void RootAt(float *at) { babel::CopyCharacter()->GetPosition(at); }

    float Yaw() {
        float rotation[4];
        babel::CopyCharacter()->GetRotation(rotation);
        return rotation[1];
    }

    CSHOT_EFFECT &Beam() { return *expanded::ShotEffectAt(expanded::ShotEffectIndex(babel::BeamEffect())); }

    TestFloor                       floor_;
    StepsUnpaused                   stepping_;
    Wielding                        wielding_;
    int                             chara_;
    CCharacter                      weapon_model_;
    std::unique_ptr<CCollisionData> data_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_data_ = nullptr;
    CDungeonMap                    *saved_map_ = nullptr;
    CCharacter                     *saved_weapon_ = nullptr;
    s32                             saved_lock_flag_ = 0;
    s32                             saved_lock_no_ = 0;
    float                           saved_pos_[3] = {};
    float                           saved_rot_[4] = {};
    s32                             saved_event_ = 0;
};

class CurseOfBabelBySuperSteve : public CurseOfBabel {
protected:
    CurseOfBabelBySuperSteve() : CurseOfBabel(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BABEL_S_SPEAR) {}
};

class CurseOfBabelWithTheCactus : public CurseOfBabel {
protected:
    CurseOfBabelWithTheCactus() : CurseOfBabel(CHARA_UNGAGA, ITEM_WEAPON_CACTUS) {}
};

class CurseOfBabelByToan : public CurseOfBabel {
protected:
    CurseOfBabelByToan() : CurseOfBabel(CHARA_TOAN, ITEM_WEAPON_BABEL_S_SPEAR) {}
};

constexpr float kSpearTop = 10.0f * 5.0f;    // what stands out of the floor once risen
constexpr float kSpearRoot = -16.0f * 5.0f;  // its root under the floor, by its tip's reach
constexpr float kStatueTop = 3.94f * 4.0f;   // the statue's
constexpr float kStatueRoot = -2.49f * 4.0f; // and its root's

} // namespace

TEST_F(CurseOfBabel, HoldingTheGuardFiveSecondsRaisesTheSpearUnderTheLockedMonster) {
    Stand(0, 20.0f, 40.0f);
    Stand(1, 0.0f, 10.0f); // nearer, not locked on to
    LockOn(0);
    Guard(true);
    Steps(expanded::kBabelGuardFrames / 2 + 1);
    EXPECT_NEAR(expanded::CurseOfBabelWeaponTint(), 0.5f, 1e-5f); // the weapon greys as it charges
    Steps(expanded::kBabelGuardFrames / 2 - 1);
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    Steps(1);
    ASSERT_TRUE(expanded::CurseOfBabelUp());
    EXPECT_EQ(expanded::CurseOfBabelTarget(), 0);
    EXPECT_TRUE(Loaded("commenu/weapon/c10w10.chr"));
    float spot[3];
    expanded::CurseOfBabelSpot(spot);
    EXPECT_FLOAT_EQ(spot[0], 20.0f);
    EXPECT_FLOAT_EQ(spot[2], 40.0f);
    // Point-up: the grip turned a quarter about x, so the blade's axis (the model's +z) stands straight up.
    const CFrame *root = babel::CopyCharacter()->frame;
    EXPECT_FLOAT_EQ(root->local[2][1], 1.0f);
    EXPECT_FLOAT_EQ(root->local[1][2], 1.0f);
    // Grey, solid, its tip buried.
    EXPECT_FLOAT_EQ(babel::CopyCharacter()->ambient_offset[0], expanded::kBabelTint);
    EXPECT_FLOAT_EQ(babel::CopyCharacter()->ambient_offset[3], 128.0f);
    float at[4];
    RootAt(at);
    EXPECT_NEAR(at[1], kSpearRoot - expanded::kBabelBuried, 1e-4f);
    EXPECT_FLOAT_EQ(expanded::CurseOfBabelWeaponTint(), 1.0f);
    // The beam marks the spot from its rise.
    EXPECT_TRUE(babel::BeamPlaying());
    EXPECT_EQ(babel::BeamKey(), babel::kBeamRise);
    EXPECT_EQ(beam_path, expanded::kBabelBeamFile);
    // Held on, no other rises.
    Steps(400);
    EXPECT_EQ(expanded::CurseOfBabelAge(), 400);
}

TEST_F(CurseOfBabel, LeavingTheGuardPoseStartsTheChargeAgain) {
    Stand(0, 0.0f, 40.0f);
    Guard(true);
    Steps(200);
    BtActStatus.guard_mode = 0; // a hit knocks him out of the pose, R1 still down
    Steps(1);
    EXPECT_FLOAT_EQ(expanded::CurseOfBabelWeaponTint(), 0.0f);
    BtActStatus.guard_mode = 5;
    Steps(expanded::kBabelGuardFrames);
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    Steps(1);
    EXPECT_TRUE(expanded::CurseOfBabelUp());
}

TEST_F(CurseOfBabel, UnlockedItRisesUnderTheNearestMonsterWithinReachElseAhead) {
    Stand(0, 0.0f, 70.0f);  // beyond reach
    Stand(1, 30.0f, 40.0f); // 50 away
    Stand(2, 0.0f, 55.0f);  // 55 away
    Raise();
    EXPECT_EQ(expanded::CurseOfBabelTarget(), 1);
    expanded::CurseOfBabelReset();
    floor_.unit().monster[1].state = -1;
    floor_.unit().monster[2].state = -1;
    Raise();
    ASSERT_TRUE(expanded::CurseOfBabelUp());
    EXPECT_EQ(expanded::CurseOfBabelTarget(), -1);
    float spot[3];
    expanded::CurseOfBabelSpot(spot);
    EXPECT_NEAR(spot[0], 0.0f, 1e-4f);
    EXPECT_NEAR(spot[2], expanded::kBabelAhead, 1e-4f);
}

TEST_F(CurseOfBabel, ItRisesFollowingItsMonsterThenStandsSolidAndTurns) {
    Stand(0, 0.0f, 40.0f, 60.0f); // its body over the risen tip: never struck
    LockOn(0);
    Raise();
    // The ease: up 2D/T the first frame, slowing by 1/T of that a frame.
    float d = kSpearTop + expanded::kBabelBuried, v0 = 2.0f * d / expanded::kBabelEmergeFrames;
    Steps(1);
    float at[4];
    RootAt(at);
    EXPECT_NEAR(at[1], kSpearRoot - expanded::kBabelBuried + v0, 1e-4f);
    // Following the monster across the floor while it rises.
    Move(0, 10.0f, 45.0f);
    Steps(1);
    RootAt(at);
    EXPECT_FLOAT_EQ(at[0], 10.0f);
    EXPECT_FLOAT_EQ(at[2], 45.0f);
    EXPECT_FALSE(expanded::SolidColumnArmed());
    Steps(expanded::kBabelEmergeFrames - 3);
    EXPECT_FALSE(expanded::CurseOfBabelRisen());
    Steps(1); // risen: its exact height, solid
    EXPECT_TRUE(expanded::CurseOfBabelRisen());
    RootAt(at);
    EXPECT_NEAR(at[1], kSpearRoot + kSpearTop, 1e-4f);
    EXPECT_TRUE(expanded::SolidColumnArmed());
    float yaw = Yaw();
    // No longer following; turning 4 degrees a frame.
    Move(0, 30.0f, 45.0f);
    Steps(1);
    RootAt(at);
    EXPECT_FLOAT_EQ(at[0], 10.0f);
    EXPECT_NEAR(Yaw() - yaw, expanded::kBabelSpin, 1e-5f);
}

TEST_F(CurseOfBabel, TheTipStrikesItsMonsterAsItRisesThrowingItAwayFromTheWielder) {
    Stand(0, 0.0f, 40.0f); // its body's underside 2 over the floor
    LockOn(0);
    Raise();
    int frames = 0;
    while (Planted().empty() && frames < expanded::kBabelEmergeFrames) {
        Steps(1);
        frames++;
    }
    // Up 3.6, then 3.48: the tip passes 2 over the floor on its second climb.
    EXPECT_EQ(frames, 2);
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    COLLISION_HIT &hit = *hits[0];
    EXPECT_EQ(hit.damage, 120);
    EXPECT_EQ(hit.owner, CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(hit.radius, expanded::kBabelStrikeRadius);
    EXPECT_FLOAT_EQ(hit.pos[2], 40.0f);
    EXPECT_EQ(hit.knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit.knockback_speed, expanded::kHeavyKickBack.speed);
    EXPECT_FLOAT_EQ(hit.knockback_decay, expanded::kHeavyKickBack.decay);
    EXPECT_FLOAT_EQ(hit.knockback_origin[2], 0.0f); // from the wielder
    EXPECT_FALSE(expanded::CurseOfBabelCatch(hit));
    EXPECT_TRUE(expanded::HitWearsWeapon(hit));
    // Once.
    ClearHits();
    Steps(expanded::kBabelEmergeFrames - 3);
    EXPECT_TRUE(Planted().empty());
}

TEST_F(CurseOfBabel, AMonsterThatLeftTheSpearIsNotStruck) {
    Stand(0, 0.0f, 40.0f);
    LockOn(0);
    Raise();
    Steps(1);
    floor_.unit().monster[0].hp = 0; // gone before the tip reached it
    Steps(expanded::kBabelEmergeFrames);
    EXPECT_TRUE(Planted().empty());
}

TEST_F(CurseOfBabel, ItsSpikesCatchTheMonstersAtItsColumnEveryQuarterTurn) {
    Stand(0, 0.0f, 40.0f); // its target, struck as it rises
    LockOn(0);
    Stand(1, 0.0f, 49.0f);  // its body 6 from the axis: within the column's 8 and 2
    Stand(2, 0.0f, 54.0f);  // 11: out of reach
    Stand(3, 60.0f, 40.0f); // far
    RaiseTo(expanded::kBabelEmergeFrames - 1);
    ClearHits(); // the strike's
    Steps(1);    // risen: the first catch
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 2u);
    EXPECT_FLOAT_EQ(hits[0]->pos[2], 40.0f);
    COLLISION_HIT &hit = *hits[1];
    EXPECT_EQ(hit.damage, 20); // a sixth of 120
    EXPECT_FLOAT_EQ(hit.pos[2], 49.0f);
    EXPECT_FLOAT_EQ(hit.radius, expanded::kBabelStrikeRadius);
    EXPECT_FLOAT_EQ(hit.knockback_speed, expanded::kBabelSpikeKick.speed);
    EXPECT_FLOAT_EQ(hit.knockback_origin[2], 40.0f); // from the spear
    EXPECT_TRUE(expanded::CurseOfBabelCatch(hit));
    EXPECT_FALSE(expanded::HitWearsWeapon(hit));
    // Every 60 degrees of its turn.
    ClearHits();
    Steps(expanded::kBabelSpear.catch_frames - 1);
    EXPECT_TRUE(Planted().empty());
    Steps(1);
    EXPECT_EQ(Planted().size(), 2u);
}

TEST_F(CurseOfBabel, ItsColumnStopsMonstersThePlayerAndShots) {
    Stand(0, 0.0f, 40.0f, 60.0f);
    LockOn(0);
    RaiseTo(expanded::kBabelEmergeFrames);
    ASSERT_TRUE(expanded::SolidColumnArmed());
    Stand(1, 0.0f, 52.0f);
    MONSTOR &live = floor_.unit().monster[1];
    live.movement[0] = 0.0f, live.movement[2] = -1.0f, live.movement_speed = 1.5f;
    expanded::MonsterMoveChecked(floor_.unit(), 1);
    EXPECT_NEAR(live.movement[2], 0.0f, 1e-5f); // slides round it
    float at[3] = {0.0f, 0.0f, 27.0f}, step[3] = {0.0f, 0.0f, 1.0f};
    expanded::PlayerMoveChecked(at, step);
    EXPECT_NEAR(step[2], 0.0f, 1e-5f); // nothing into it
}

TEST_F(CurseOfBabel, EveryMonsterOnTheFloorButTheBossesIsConfusedUntilTheSpearHasGone) {
    Stand(0, 0.0f, 40.0f, 60.0f);
    Stand(1, 200.0f, 300.0f); // far across the floor
    Stand(2, -50.0f, 0.0f);
    floor_.unit().monster[2].kind = MONSTER_KIND_NO_LOCK_ON; // a boss
    Stand(3, 100.0f, 0.0f);
    floor_.unit().monster[3].revealed = 0; // shut in its chest
    LockOn(0);
    Raise();
    EXPECT_TRUE(Confused(0));
    EXPECT_TRUE(Confused(1));
    EXPECT_FALSE(Confused(2));
    EXPECT_FALSE(Confused(3));
    // One that comes out while it stands is confused too, until the same end.
    Steps(500);
    floor_.unit().monster[3].revealed = -1;
    Steps(1);
    EXPECT_TRUE(Confused(3));
    // The confused take on its tint, the weapon its grey.
    float ambient[4] = {};
    expanded::CurseOfBabelLight(floor_.unit().chara[1][0], ambient);
    EXPECT_FLOAT_EQ(ambient[0], expanded::kBabelConfusedTint[0]);
    EXPECT_FLOAT_EQ(ambient[2], expanded::kBabelConfusedTint[2]);
    float boss[4] = {};
    expanded::CurseOfBabelLight(floor_.unit().chara[2][0], boss);
    EXPECT_FLOAT_EQ(boss[2], 0.0f);
    float weapon[4] = {};
    expanded::CurseOfBabelLight(*NowWeapon, weapon);
    EXPECT_FLOAT_EQ(weapon[1], expanded::kBabelTint);
    Steps(expanded::kBabelLifeFrames - 501 - 1);
    EXPECT_TRUE(expanded::CurseOfBabelUp());
    EXPECT_TRUE(Confused(1));
    EXPECT_TRUE(Confused(3));
    Steps(1);
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    EXPECT_FALSE(Confused(0));
    EXPECT_FALSE(Confused(1));
    EXPECT_FALSE(Confused(3));
}

TEST_F(CurseOfBabel, TheBeamLoopsThenVanishesAndTheSpearFadesWithIt) {
    Stand(0, 0.0f, 40.0f, 60.0f);
    LockOn(0);
    Raise();
    CCharacter &beam = Beam().chara[0];
    EXPECT_FLOAT_EQ(beam.motion_speed, babel::kBeamRate);
    EXPECT_FLOAT_EQ(beam.motion_type.state.time, babel::kBeamRiseStart);
    // Its rise played out, its loop.
    beam.motion_type.state.time = babel::kBeamRiseEnd - babel::kBeamClipLead;
    Steps(1);
    EXPECT_EQ(babel::BeamKey(), babel::kBeamLoop);
    EXPECT_FLOAT_EQ(beam.motion_type.state.time, babel::kBeamLoopStart);
    // The loop from its start again.
    beam.motion_type.state.time = babel::kBeamLoopEnd - babel::kBeamClipLead;
    Steps(1);
    EXPECT_EQ(babel::BeamKey(), babel::kBeamLoop);
    EXPECT_FLOAT_EQ(beam.motion_type.state.time, babel::kBeamLoopStart);
    // Its time up, the vanish; the spear solid until the vanish's frame 60.
    Steps(expanded::kBabelStandFrames - 3);
    EXPECT_EQ(babel::BeamKey(), babel::kBeamLoop);
    Steps(1);
    EXPECT_EQ(babel::BeamKey(), babel::kBeamVanish);
    EXPECT_FLOAT_EQ(beam.motion_type.state.time, babel::kBeamVanishStart);
    Steps(expanded::kBabelVanishFrames - expanded::kBabelFadeFrames - 1);
    EXPECT_FLOAT_EQ(expanded::CurseOfBabelAlpha(), 1.0f);
    Steps(expanded::kBabelFadeFrames / 2 + 1);
    EXPECT_NEAR(expanded::CurseOfBabelAlpha(), 0.5f, 1e-5f);
    float weapon[4] = {};
    expanded::CurseOfBabelLight(*NowWeapon, weapon);
    EXPECT_NEAR(weapon[0], expanded::kBabelTint / 2.0f, 1e-3f);
    EXPECT_TRUE(expanded::SolidColumnArmed());
    Steps(expanded::kBabelFadeFrames / 2 - 1);
    EXPECT_TRUE(expanded::CurseOfBabelUp());
    Steps(1);
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    EXPECT_FALSE(expanded::SolidColumnArmed());
    EXPECT_FALSE(babel::BeamPlaying());
    EXPECT_FLOAT_EQ(expanded::CurseOfBabelWeaponTint(), 0.0f);
}

TEST_F(CurseOfBabel, ABeamTheGameEndsBeforeItsVanishIsTakenUpAgain) {
    Stand(0, 0.0f, 40.0f, 60.0f);
    LockOn(0);
    RaiseTo(100);
    Beam().active[0] = 0;
    Steps(1);
    EXPECT_TRUE(babel::BeamPlaying());
}

TEST_F(CurseOfBabel, NoOtherRisesUntilItHasGone) {
    Stand(0, 0.0f, 40.0f, 60.0f);
    LockOn(0);
    RaiseTo(100);
    Raise();
    EXPECT_EQ(expanded::CurseOfBabelAge(), 100 + expanded::kBabelGuardFrames + 1);
}

TEST_F(CurseOfBabel, ItGoesWithTheWielderAndEveryConfusionWithIt) {
    Stand(0, 0.0f, 40.0f, 60.0f);
    Stand(1, 50.0f, 40.0f);
    LockOn(0);
    RaiseTo(100);
    ASSERT_TRUE(Confused(1));
    floor_.status().cur_chara = CHARA_XIAO;
    Steps(1);
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    EXPECT_FALSE(expanded::SolidColumnArmed());
    EXPECT_FALSE(Confused(1));
    floor_.status().cur_chara = CHARA_UNGAGA;
    RaiseTo(100);
    ASSERT_TRUE(expanded::CurseOfBabelUp());
    Wielding halberd(ITEM_WEAPON_HALBERT);
    Steps(1);
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    EXPECT_FALSE(Confused(1));
}

TEST_F(CurseOfBabelWithTheCactus, NothingRises) {
    Stand(0, 0.0f, 40.0f);
    LockOn(0);
    Raise();
    Steps(1);
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    EXPECT_FALSE(Confused(0));
}

TEST_F(CurseOfBabelByToan, NothingRises) {
    Stand(0, 0.0f, 40.0f);
    LockOn(0);
    Raise();
    EXPECT_FALSE(expanded::CurseOfBabelUp());
    EXPECT_TRUE(loaded.empty());
}

TEST_F(CurseOfBabelBySuperSteve, AStatueOfSuperSteveRisesItsHandsCatchingEveryHalfTurn) {
    Stand(0, 0.0f, 40.0f, 60.0f); // its target, over the statue
    LockOn(0);
    Raise();
    ASSERT_TRUE(expanded::CurseOfBabelUp());
    EXPECT_TRUE(Loaded("commenu/weapon/c04w13.chr"));
    // Fork up: the root keeps its scale alone, the grip's rotation turned into its children.
    const CFrame *root = babel::CopyCharacter()->frame;
    EXPECT_FLOAT_EQ(root->local[0][0], 1.0f);
    EXPECT_FLOAT_EQ(root->local[1][1], 1.0f);
    EXPECT_EQ(babel::CopyCharacter()->motion_no, babel::kStatueKey);
    EXPECT_EQ(statue_rig.keys[babel::kStatueKey].start, static_cast<s32>(babel::kStatuePose));
    EXPECT_FLOAT_EQ(Beam().chara[0].scale[0], expanded::kBabelStatue.beam_scale);
    Steps(expanded::kBabelEmergeFrames);
    float at[4];
    RootAt(at);
    EXPECT_NEAR(at[1], kStatueRoot + kStatueTop, 1e-4f);
    // Its fork above its root.
    float fork[4];
    float centre[3] = {0.0f, 0.0f, 0.0f};
    ASSERT_TRUE(babel::CopyForkPoint(centre, fork));
    EXPECT_GT(fork[1], at[1]);
    // A monster at a hand is caught for half the attack; the next catch is half a turn on.
    float hand[4];
    ASSERT_TRUE(babel::CopyForkPoint(expanded::kBabelHands[1], hand));
    ClearHits();
    Stand(1, hand[0], hand[2] + 8.0f, hand[1]);  // its body 5 from the hand
    Stand(2, hand[0], hand[2] + 10.0f, hand[1]); // 7: out of reach
    Steps(expanded::kBabelStatue.catch_frames);
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0]->damage, 60);
    EXPECT_TRUE(expanded::CurseOfBabelCatch(*hits[0]));
    ClearHits();
    Steps(expanded::kBabelStatue.catch_frames - 1);
    EXPECT_TRUE(Planted().empty());
    // Solid at its feet.
    Stand(3, 0.0f, 40.0f + expanded::kBabelStatue.block_radius + 4.0f);
    MONSTOR &live = floor_.unit().monster[3];
    live.movement[0] = 0.0f, live.movement[2] = -1.0f, live.movement_speed = 1.5f;
    expanded::MonsterMoveChecked(floor_.unit(), 3);
    EXPECT_NEAR(live.movement[2], 0.0f, 1e-5f);
}

TEST_F(CurseOfBabelBySuperSteve, EveryMonsterIsConfusedByTheStatueToo) {
    Stand(0, 0.0f, 40.0f, 60.0f);
    Stand(1, 200.0f, 300.0f);
    LockOn(0);
    Raise();
    EXPECT_TRUE(Confused(0));
    EXPECT_TRUE(Confused(1));
}
