#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <numbers>
#include <string>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/weapons/abilities.hpp"
#include "expanded/weapons/xiao/angel_gear.hpp"
#include "expanded/weapons/xiao/angel_gear/shield_prop.hpp"
#include "expanded/weapons/xiao/angel_gear/shield_ring.hpp"
#include "expanded/weapons/xiao/angel_gear/shot_reflect.hpp"
#include "framevu1.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

namespace gear = expanded::angel_gear;
using expanded::test::TestFloor;
using expanded::test::Wielding;

constexpr int kGoo = 0x800;

// The weapon's copy, stood in for: its grip, a bone 2 up, and under that the pouch and the muzzle (3 up), one block of frames
// as a model's are (a motion names its frames by their place in it). The pouch's one translate track rests at 0, draws back
// 4 along z to full draw at 250, snaps forward past its rest to -1 by 254 and settles.
struct Rig {
    CFrameVu1   frames[4];
    CFrameVu1  &root = frames[0], &chain = frames[1], &pouch = frames[2], &muzzle = frames[3];
    MOTION_INFO keys[32] = {};
    Mot_Key     track_keys[9] = {};
    Mot_List    track = {};
};

Rig         rig;
std::string loaded_path;

constexpr u32   kTrackFrames[9] = {0, 240, 245, 250, 251, 253, 254, 255, 261};
constexpr float kTrackZ[9] = {0.0f, 0.0f, 2.0f, 4.0f, 4.0f, -0.5f, -1.0f, -0.8f, 0.0f};

void Frame(CFrameVu1 &frame, const char *name, CFrame *parent, float y) {
    frame.Initialize();
    std::strncpy(frame.name, name, sizeof(frame.name) - 1);
    frame.local[3][1] = y;
    if (parent != nullptr) {
        frame.SetParent(parent);
    }
}

bool FakeWeapon(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    loaded_path = path;
    Frame(rig.root, "pati2", nullptr, 0.0f);
    Frame(rig.chain, "chn30", &rig.root, 2.0f);
    Frame(rig.pouch, "null24", &rig.chain, 0.0f);
    Frame(rig.muzzle, "eff30", &rig.chain, 3.0f);
    for (MOTION_INFO &key : rig.keys) {
        key = {1, 1, 1.0f, 0};
    }
    rig.keys[gear::kPropKeyDraw] = {240, 251, 0.7f, 0};
    rig.keys[gear::kPropKeyHold] = {250, 250, 0.0f, 0};
    rig.keys[gear::kPropKeyShoot] = {251, 255, 0.7f, 0};
    for (int i = 0; i < 9; i++) {
        rig.track_keys[i] = {};
        rig.track_keys[i].frame = kTrackFrames[i];
        rig.track_keys[i].value[2] = kTrackZ[i];
    }
    rig.track = {};
    rig.track.frame = 2; // the pouch
    rig.track.type = MOTION_KEY_TRANSLATION;
    rig.track.key_count = 9;
    rig.track.keys = rig.track_keys;
    chara.frame = &rig.root;
    chara.motion_type.motion_info = rig.keys;
    chara.motion_type.proc_list = &rig.track;
    return true;
}

// A monster's fireball: flies at 1.5 a frame for 119 frames, hits the player in a 4-unit sphere, fire.
BT_SHOT_EFFECT fireball = {
    "f_boll_3", 0, 0, {0.0f, 1.5f, 0.0f, 0.0f},
       {0.0f, 4.0f, 6.0f, 0.0f},
       119, 17, 0x1, 3, 1, {-1, 0, 1, 2},
       0, {},
       0.0f, 0, {-1, -1, -1, -1}
};

CFrameVu1   shot_frames[8];
MOTION_INFO shot_keys[4] = {
    {0,  10, 1.0f, 0},
    {10, 20, 1.0f, 0},
    {20, 30, 1.0f, 0},
    {30, 40, 1.0f, 0},
};

// Xiao with weapon_no (carrying sphere when given), attack 140, guarding on a full gauge at the origin facing +z; a
// fireball pack in the shot pack's first slot; the weapon's copy faked.
class GuardianReflector : public ::testing::Test {
protected:
    explicit GuardianReflector(int weapon_no = ITEM_WEAPON_ANGE_GEAR, int sphere = 0) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        BtActStatus = {};
        BtActStatus.action_gauge = 100.0f;
        floor_.status().cur_chara = CHARA_XIAO;
        floor_.status().hp[CHARA_XIAO] = 50;
        floor_.status().max_hp[CHARA_XIAO] = 100;
        wielding_.weapon().attack = 140;
        saved_event_ = BtEventMode;
        BtEventMode = 0;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_data_ = NowColData;
        NowColData = data_.get();
        saved_pack_ = NowShotEffect;
        NowShotEffect = pack_.get();
        for (CSHOT_EFFECT &effect : pack_->effect) {
            effect.Initialize();
        }
        CSHOT_EFFECT &effect = Fireballs();
        effect.effect_data = &fireball;
        effect.slot_count = 4;
        for (int i = 0; i < 8; i++) {
            effect.chara[i].Initialize();
            shot_frames[i].Initialize();
            effect.chara[i].frame = &shot_frames[i];
            effect.chara[i].motion_type.motion_info = shot_keys;
        }
        loaded_path.clear();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeWeapon);
        expanded::GuardianReflectorReset();
        gear::PropReset();
        gear::RingReset();
        Guard(true);
    }

    void TearDown() override {
        expanded::GuardianReflectorReset();
        gear::PropReset();
        gear::RingReset();
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        NowShotEffect = saved_pack_;
        NowColData = saved_data_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        BtEventMode = saved_event_;
        BtActStatus = {};
    }

    CSHOT_EFFECT &Fireballs() { return pack_->effect[0]; }

    void Guard(bool on) { BtActStatus.guard_mode = on ? 5 : 0; }

    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::GuardianReflectorStep();
        }
    }

    // Up and solid.
    void Raise() { Steps(expanded::kReflectorFadeIn); }

    // monster standing at (x, 0, z), in play, with a body sphere of radius 5 at height 6.
    void Stand(int monster, float x, float z) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = -1;
        live.hp = 100;
        live.status_chance = 100;
        CCharacter &body = floor_.unit().chara[monster][0];
        body.pos[0] = x;
        body.pos[2] = z;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.position[0][0] = x;
        effect.position[0][1] = 6.0f;
        effect.position[0][2] = z;
        effect.position[0][3] = 1.0f;
        effect.radius[0] = 5.0f;
        effect.timer[0] = 1;
    }

    // A fireball in slot from at, flying along velocity.
    void Fire(int slot, float x, float y, float z, float vx, float vy, float vz) {
        CSHOT_EFFECT &effect = Fireballs();
        effect.active[slot] = 1;
        effect.phase[slot] = 1;
        effect.chara[slot].SetPosition(x, y, z);
        effect.velocity[slot][0] = vx;
        effect.velocity[slot][1] = vy;
        effect.velocity[slot][2] = vz;
        effect.phase_delay[slot] = fireball.life_time;
        effect.damage[slot] = fireball.damage;
        effect.wait_state[slot] = 0;
        effect.user_id[slot] = 200;
        effect.source_id[slot] = 3;
    }

    // A monster's swing (a shot's blast with scale_rate 1) at point, reaching radius.
    int Swing(const float *point, float radius, float scale_rate = 0.0f) {
        float at[4] = {point[0], point[1], point[2], 1.0f};
        return NowColData->Set(at, 20, 2, radius, scale_rate, 1, 2, 0, 0);
    }

    // A swing at the slingshot's grip.
    int SwingAtShield() {
        float root[3];
        gear::PropRoot(root);
        return Swing(root, 3.0f);
    }

    TestFloor                          floor_;
    Wielding                           wielding_;
    std::unique_ptr<CCollisionData>    data_ = std::make_unique<CCollisionData>();
    std::unique_ptr<CSHOT_EFFECT_PACK> pack_ = std::make_unique<CSHOT_EFFECT_PACK>();
    CCollisionData                    *saved_data_ = nullptr;
    CSHOT_EFFECT_PACK                 *saved_pack_ = nullptr;
    float                              saved_pos_[3] = {};
    float                              saved_rot_[4] = {};
    s32                                saved_event_ = 0;
};

class GuardianReflectorWithTheShooter : public GuardianReflector {
protected:
    GuardianReflectorWithTheShooter() : GuardianReflector(ITEM_WEAPON_ANGEL_SHOOTER) {}
};

class GuardianReflectorBySuperSteve : public GuardianReflector {
protected:
    GuardianReflectorBySuperSteve() : GuardianReflector(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_ANGE_GEAR) {}
};

} // namespace

TEST_F(GuardianReflector, GuardingOnAFullGaugeTheSlingshotStandsAndFadesIn) {
    Steps(1);
    ASSERT_TRUE(expanded::GuardianReflectorUp());
    EXPECT_NEAR(expanded::GuardianReflectorAlpha(), 1.0f / expanded::kReflectorFadeIn, 1e-5f);
    EXPECT_EQ(expanded::GuardianReflectorHits(), expanded::kReflectorHits);
    Steps(expanded::kReflectorFadeIn - 1);
    EXPECT_FLOAT_EQ(expanded::GuardianReflectorAlpha(), 1.0f);
    EXPECT_EQ(loaded_path, "commenu/weapon/c04w14.chr"); // the Angel Gear's own pack
}

TEST_F(GuardianReflector, LetGoItClearsAway) {
    Raise();
    Guard(false);
    Steps(expanded::kReflectorFadeOut - 1);
    EXPECT_TRUE(expanded::GuardianReflectorUp());
    Steps(1);
    EXPECT_FALSE(expanded::GuardianReflectorUp());
}

TEST_F(GuardianReflector, ItStandsOnlyOnAFullGauge) {
    BtActStatus.action_gauge = 50.0f;
    Steps(10);
    EXPECT_FALSE(expanded::GuardianReflectorUp());
    EXPECT_FLOAT_EQ(expanded::GuardianReflectorGaugeGain(1.5f), 1.5f); // the gauge its own
}

TEST_F(GuardianReflectorWithTheShooter, AnotherWeaponNeverRaisesIt) {
    Steps(30);
    EXPECT_FALSE(expanded::GuardianReflectorUp());
}

TEST_F(GuardianReflectorBySuperSteve, SuperSteveWithTheSphereRaisesItsOwnModel) {
    Steps(1);
    EXPECT_TRUE(expanded::GuardianReflectorUp());
    EXPECT_EQ(loaded_path, "commenu/weapon/c04w13.chr");
}

TEST_F(GuardianReflector, ItStandsInFrontOfHerAndTurnsToTheNearestMonster) {
    Steps(1);
    float root[3];
    gear::PropRoot(root);
    EXPECT_NEAR(root[0], 0.0f, 1e-4f);
    EXPECT_NEAR(root[1], gear::kPropUp, 1e-4f);
    EXPECT_NEAR(root[2], gear::kPropAhead, 1e-4f);
    Stand(0, 100.0f, 0.0f); // off to her +x
    Steps(60);
    EXPECT_NEAR(gear::PropOrbit(), std::numbers::pi_v<float> / 2.0f, 0.01f);
    gear::PropRoot(root);
    EXPECT_NEAR(root[0], gear::kPropAhead, 0.1f);
    EXPECT_NEAR(root[2], 0.0f, 0.1f);
}

TEST_F(GuardianReflector, ItsTurnEasesAndIsCapped) {
    Steps(1);
    Stand(0, -100.0f, -1.0f); // nearly straight behind her
    Steps(1);
    EXPECT_NEAR(std::fabs(gear::PropOrbit()), gear::kPropOrbitRate, 1e-4f);
}

TEST_F(GuardianReflector, TheCopysPullDrawsBackAsAnImpactAndRestsTaut) {
    Steps(1);
    // Scaled about the rest to kPropPull of travel (authored 4: 0.625 of it), the catch pose -0.625.
    float k = gear::kPropPull / 4.0f, idle = -1.0f * k;
    EXPECT_FLOAT_EQ(rig.track_keys[3].value[2], 4.0f * k);  // 250, full draw
    EXPECT_FLOAT_EQ(rig.track_keys[6].value[2], idle);      // 254, the catch pose
    EXPECT_FLOAT_EQ(rig.track_keys[5].value[2], -0.5f * k); // 253, the snap past it
    // The draw runs from the catch pose to full draw, a cubic ease-out; past the catch it holds there.
    EXPECT_FLOAT_EQ(rig.track_keys[1].value[2], idle);
    float t = 0.5f, e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    EXPECT_NEAR(rig.track_keys[2].value[2], idle + (4.0f * k - idle) * e, 1e-5f);
    EXPECT_FLOAT_EQ(rig.track_keys[7].value[2], idle);
    EXPECT_FLOAT_EQ(rig.track_keys[8].value[2], idle);
    EXPECT_FLOAT_EQ(rig.track_keys[0].value[2], 0.0f); // outside the draw: untouched
    // It idles on that pose.
    EXPECT_EQ(rig.keys[gear::kPropKeyIdle].start, 254);
    EXPECT_EQ(rig.keys[gear::kPropKeyIdle].end, 254);
    EXPECT_FLOAT_EQ(rig.keys[gear::kPropKeyIdle].speed, 0.0f);
    EXPECT_EQ(gear::PropCharacter()->motion_no, gear::kPropKeyIdle);
}

TEST_F(GuardianReflector, TheCopyStandsUprightAsTheModPosedIt) {
    Steps(1);
    // The grip's (here plain) rotation turned three quarter-turns in pitch and one in yaw, set into the root's child.
    const float want[4][3] = {
        {0.0f, 0.0f,  -1.0f},
        {1.0f, 0.0f,  0.0f },
        {0.0f, -1.0f, 0.0f },
        {2.0f, 0.0f,  0.0f }, // its 2 up, turned with it
    };
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            EXPECT_NEAR(rig.chain.local[r][c], want[r][c], 1e-6f) << r << "," << c;
        }
    }
    // So the pouch (on its catch pose, -0.625 along the chain's z) stands at (2, 0.625, 0) from the grip and the muzzle (3 up
    // the chain) at (5, 0, 0), twice that in the world.
    float pouch[3], muzzle[3];
    gear::PropPouch(pouch);
    gear::PropMuzzle(muzzle);
    EXPECT_NEAR(pouch[0], 2.0f * gear::kPropScale, 1e-4f);
    EXPECT_NEAR(pouch[1], gear::kPropUp + 0.625f * gear::kPropScale, 1e-4f);
    EXPECT_NEAR(pouch[2], gear::kPropAhead, 1e-4f);
    EXPECT_NEAR(muzzle[0], 5.0f * gear::kPropScale, 1e-4f);
    EXPECT_NEAR(muzzle[1], gear::kPropUp, 1e-4f);
}

TEST_F(GuardianReflector, ASwingStrikesItAndTheGaugeIsItsHealth) {
    Raise();
    int swing = SwingAtShield();
    expanded::PlayerHitsChecking();
    EXPECT_EQ(NowColData->active[swing], 0); // spent on it
    EXPECT_EQ(expanded::GuardianReflectorHits(), expanded::kReflectorHits - 1);
    EXPECT_FLOAT_EQ(BtActStatus.action_gauge, 80.0f);
    EXPECT_FLOAT_EQ(expanded::GuardianReflectorGaugeGain(1.5f), 0.0f);
    // For a while after, no swing is looked at.
    swing = SwingAtShield();
    expanded::PlayerHitsChecking();
    EXPECT_NE(NowColData->active[swing], 0);
    EXPECT_EQ(expanded::GuardianReflectorHits(), expanded::kReflectorHits - 1);
    Steps(gear::kStrikeFrames);
    SwingAtShield();
    expanded::PlayerHitsChecking();
    EXPECT_EQ(expanded::GuardianReflectorHits(), expanded::kReflectorHits - 2);
    // A hit on her refills the gauge: it is put back.
    BtActStatus.action_gauge = 100.0f;
    Steps(1);
    EXPECT_FLOAT_EQ(BtActStatus.action_gauge, 60.0f);
}

TEST_F(GuardianReflector, AShotsBlastOnItIsSpentSilentlyAndASwingOutOfReachLeftForHer) {
    Raise();
    float root[3];
    gear::PropRoot(root);
    int   blast = Swing(root, 3.0f, 1.0f);
    float far[3] = {root[0] + 30.0f, root[1], root[2]};
    int   missed = Swing(far, 3.0f);
    expanded::PlayerHitsChecking();
    EXPECT_EQ(NowColData->active[blast], 0);
    EXPECT_NE(NowColData->active[missed], 0);
    EXPECT_EQ(expanded::GuardianReflectorHits(), expanded::kReflectorHits);
}

TEST_F(GuardianReflector, TheLastStrikeBreaksItAndTheGaugeRefillsBeforeItStandsAgain) {
    Raise();
    for (int strike = 0; strike < expanded::kReflectorHits; strike++) {
        SwingAtShield();
        expanded::PlayerHitsChecking();
        Steps(gear::kStrikeFrames);
    }
    EXPECT_FALSE(expanded::GuardianReflectorUp()); // cleared over kReflectorDispel, within the strike's wait
    EXPECT_TRUE(expanded::GuardianReflectorCooling());
    EXPECT_FLOAT_EQ(BtActStatus.action_gauge, 0.0f);
    EXPECT_FLOAT_EQ(expanded::GuardianReflectorGaugeGain(1.5f), 100.0f / expanded::kReflectorCooldown);
    Steps(10);
    EXPECT_FALSE(expanded::GuardianReflectorUp()); // still guarding, the gauge refilling
    BtActStatus.action_gauge = 100.0f;
    Steps(1);
    EXPECT_FALSE(expanded::GuardianReflectorCooling());
    EXPECT_TRUE(expanded::GuardianReflectorUp());
}

TEST_F(GuardianReflector, MonstersFindHerOnTheRingAndStopThere) {
    Stand(0, 0.0f, 100.0f);
    Stand(1, 0.0f, 10.0f); // inside it
    Stand(2, 100.0f, 0.0f);
    floor_.unit().monster[2].state = 1; // spawned, not yet in play
    float found[4];
    NowMonstorUnit->current_monster = 0;
    expanded::ScriptPlayerPosition(found);
    EXPECT_FLOAT_EQ(found[2], 0.0f); // the ring down: her
    EXPECT_FLOAT_EQ(expanded::MonsterBlockRadius(6.0f), 6.0f);
    Steps(1);
    expanded::ScriptPlayerPosition(found);
    EXPECT_NEAR(found[0], 0.0f, 1e-4f);
    EXPECT_NEAR(found[2], gear::kRingRadius, 1e-4f);
    NowMonstorUnit->current_monster = 1;
    expanded::ScriptPlayerPosition(found);
    EXPECT_FLOAT_EQ(found[2], 0.0f);
    NowMonstorUnit->current_monster = 2;
    expanded::ScriptPlayerPosition(found);
    EXPECT_NEAR(found[0], gear::kRingRadius, 1e-4f);
    EXPECT_FLOAT_EQ(expanded::MonsterBlockRadius(6.0f), gear::kRingRadius);
}

TEST_F(GuardianReflector, AShotComingAtHerIsClaimedAndCaughtInThePouch) {
    Raise();
    Fire(0, 0.0f, 10.0f, 60.0f, 0.0f, 0.0f, -1.5f);
    Steps(1);
    EXPECT_EQ(gear::ClaimCount(), 1);
    EXPECT_EQ(Fireballs().wait_state[0], gear::kHeld); // it hurts no one
    float pouch[3];
    gear::PropPouch(pouch);
    Fireballs().chara[0].SetPosition(pouch[0], pouch[1], pouch[2] + 5.0f);
    Steps(1);
    EXPECT_EQ(Fireballs().active[0], 0);
    EXPECT_EQ(gear::ClaimCount(), 0);
    EXPECT_EQ(gear::PendingCount(), 1);
    // While it stands solid, a shot meeting "the player" meets the pouch.
    float point[4] = {0.0f, 14.0f, 0.0f, 1.0f};
    expanded::ShotPlayerPoint(point);
    gear::PropPouch(pouch);
    for (int c = 0; c < 3; c++) {
        EXPECT_FLOAT_EQ(point[c], pouch[c]);
    }
}

TEST_F(GuardianReflector, AShotTheGameStoppedAtThePouchIsCaught) {
    Raise();
    Fire(0, 0.0f, 10.0f, 60.0f, 0.0f, 0.0f, -1.5f);
    Steps(1);
    float pouch[3];
    gear::PropPouch(pouch);
    Fireballs().chara[0].SetPosition(pouch[0], pouch[1], pouch[2] + 10.5f); // met the pouch as "the player"
    Fireballs().phase[0] = 2;
    Steps(1);
    EXPECT_EQ(gear::PendingCount(), 1);
}

TEST_F(GuardianReflector, AShotThatWouldEndShortOrIsGoingAwayIsLeftAlone) {
    Raise();
    Fire(0, 0.0f, 10.0f, 60.0f, 0.0f, 0.0f, -1.5f);
    Fireballs().phase_delay[0] = 10;
    Fire(1, 0.0f, 10.0f, 60.0f, 0.0f, 0.0f, 1.5f);
    Fire(2, 0.0f, 10.0f, 60.0f, 0.0f, 0.0f, -1.5f);
    Fireballs().phase_delay[2] = -1; // flies for good: never claimed, as the mod's flight test has it
    Steps(1);
    EXPECT_EQ(gear::ClaimCount(), 0);
    EXPECT_EQ(Fireballs().wait_state[0], 0);
}

TEST_F(GuardianReflector, LettingGoLetsTheClaimsLand) {
    Raise();
    Fire(0, 0.0f, 10.0f, 60.0f, 0.0f, 0.0f, -1.5f);
    Steps(1);
    Guard(false);
    Steps(1);
    EXPECT_EQ(gear::ClaimCount(), 0);
    EXPECT_EQ(Fireballs().wait_state[0], 0);
}

TEST_F(GuardianReflector, ACaughtShotIsFiredBackAtTheNearestMonsterFromTheMuzzle) {
    Raise();
    Stand(0, 0.0f, 80.0f);
    Fire(0, 0.0f, 10.0f, 30.0f, 0.0f, 0.0f, -1.5f);
    Steps(1);
    float pouch[3];
    gear::PropPouch(pouch);
    Fireballs().chara[0].SetPosition(pouch[0], pouch[1], pouch[2]);
    Steps(1); // caught, and nothing more coming: the draw starts
    EXPECT_EQ(gear::PropKeyNow(), gear::kPropKeyDraw);
    Steps(expanded::kReflectorDraw - 1);
    EXPECT_EQ(gear::PropKeyNow(), gear::kPropKeyHold);
    Steps(expanded::kReflectorHold);
    EXPECT_EQ(gear::PropKeyNow(), gear::kPropKeyShoot);
    EXPECT_EQ(gear::PendingCount(), 0);
    CSHOT_EFFECT &effect = Fireballs();
    int           slot = effect.current_slot;
    ASSERT_GE(slot, 0);
    EXPECT_NE(effect.active[slot], 0);
    EXPECT_EQ(effect.phase[slot], 1);
    EXPECT_EQ(effect.damage[slot], fireball.damage);
    EXPECT_EQ(effect.wait_state[slot], gear::kHeld);
    EXPECT_EQ(effect.phase_delay[slot], gear::kReturnFrames);
    const float *v = effect.velocity[slot];
    EXPECT_NEAR(std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]), 1.5f * gear::kReturnBoost, 1e-4f);
    EXPECT_GT(v[2], 0.0f); // toward it
    Steps(expanded::kReflectorShoot);
    EXPECT_EQ(gear::PropKeyNow(), gear::kPropKeyIdle);
}

// The shot caught, then fired back at monster 0, standing 80 ahead: its slot.
int ReturnAtMonster(std::function<void(int)> steps, CSHOT_EFFECT &effect) {
    steps(1);
    float pouch[3];
    gear::PropPouch(pouch);
    effect.chara[0].SetPosition(pouch[0], pouch[1], pouch[2]);
    steps(1 + expanded::kReflectorDraw + expanded::kReflectorHold);
    return effect.current_slot;
}

// The hit a returned shot planted, or -1.
int Planted() {
    int planted = -1;
    for (int i = 0; i < 96; i++) {
        const COLLISION_HIT &hit = NowColData->hit[i];
        planted = NowColData->active[i] != 0 && hit.owner == CHARA_XIAO && hit.target_mask == 2 ? i : planted;
    }
    return planted;
}

TEST_F(GuardianReflector, AReturnedShotMeetsAMonstersHurtSphereAndLandsTheWeaponsHit) {
    Raise();
    Stand(0, 0.0f, 80.0f); // its hurt sphere: radius 5, 6 up
    Fire(0, 0.0f, 10.0f, 30.0f, 0.0f, 0.0f, -1.5f);
    CSHOT_EFFECT &effect = Fireballs();
    int           slot = ReturnAtMonster([&](int n) { Steps(n); }, effect);
    ASSERT_GE(slot, 0);
    float hit_at[4];
    // Passing beside it, close as it may be, it misses.
    float beside[4] = {9.5f, 6.0f, 80.0f, 1.0f};
    EXPECT_EQ(expanded::ShotEffectCollision(effect, slot, hit_at, beside), SHOT_COLLISION_NONE);
    EXPECT_EQ(Planted(), -1);
    // Within its own flying radius (4) and the sphere's (5) of the sphere: met.
    float on[4] = {0.0f, 6.0f, 80.0f - 8.5f, 1.0f};
    EXPECT_EQ(expanded::ShotEffectCollision(effect, slot, hit_at, on), SHOT_COLLISION_NONE); // no impact: its burst
    EXPECT_EQ(effect.phase_delay[slot], 0);
    int planted = Planted();
    ASSERT_GE(planted, 0);
    const COLLISION_HIT &hit = NowColData->hit[planted];
    EXPECT_EQ(hit.damage, 10); // attack 140 x (0 + 1) / 14, off the dungeons
    EXPECT_EQ(hit.flags, 0x1); // fire
    EXPECT_EQ(hit.life, gear::kHitLife);
    EXPECT_FLOAT_EQ(hit.radius, 4.0f + gear::kHitMargin);
    EXPECT_FLOAT_EQ(hit.pos[2], 80.0f); // on the sphere
}

TEST_F(GuardianReflector, ItsDamageRoundsHalvesToEven) {
    wielding_.weapon().attack = 35; // 2.5
    Raise();
    Stand(0, 0.0f, 80.0f);
    Fire(0, 0.0f, 10.0f, 30.0f, 0.0f, 0.0f, -1.5f);
    CSHOT_EFFECT &effect = Fireballs();
    int           slot = ReturnAtMonster([&](int n) { Steps(n); }, effect);
    ASSERT_GE(slot, 0);
    float on[4] = {0.0f, 6.0f, 79.0f, 1.0f}, hit_at[4];
    expanded::ShotEffectCollision(effect, slot, hit_at, on);
    ASSERT_GE(Planted(), 0);
    EXPECT_EQ(NowColData->hit[Planted()].damage, 2);
}

TEST_F(GuardianReflector, AGooShotsSlowComesAsItMeetsTheMonster) {
    Raise();
    fireball.hit_flags = kGoo;
    Stand(0, 0.0f, 80.0f);
    Fire(0, 0.0f, 10.0f, 30.0f, 0.0f, 0.0f, -1.5f);
    CSHOT_EFFECT &effect = Fireballs();
    int           slot = ReturnAtMonster([&](int n) { Steps(n); }, effect);
    float         on[4] = {0.0f, 6.0f, 79.0f, 1.0f}, hit_at[4];
    expanded::ShotEffectCollision(effect, slot, hit_at, on);
    fireball.hit_flags = 0x1;
    ASSERT_GE(Planted(), 0);
    EXPECT_EQ(NowColData->hit[Planted()].flags, 0); // an ailment and no element: no element
    EXPECT_EQ(floor_.unit().monster[0].slow_timer, 180);
}

TEST_F(GuardianReflector, ItGlowsASteadyGold) {
    Steps(1);
    float lit[3] = {}, her[3] = {};
    expanded::GuardianReflectorLight(*gear::PropCharacter(), lit);
    expanded::GuardianReflectorLight(CharaMain, her);
    for (int i = 0; i < 3; i++) {
        EXPECT_FLOAT_EQ(lit[i], expanded::kReflectorTint[i]);
        EXPECT_FLOAT_EQ(her[i], 0.0f);
    }
}

TEST_F(GuardianReflector, WithTheHealAbilityEachTickHealsTheRestOfTheParty) {
    CUserStatus &status = floor_.status();
    for (int chara = 0; chara < 6; chara++) {
        status.max_hp[chara] = 100;
        status.hp[chara] = 40;
    }
    status.hp[CHARA_TOAN] = 0;   // fallen
    status.hp[CHARA_RUBY] = 100; // full
    expanded::HealTicked();      // no Heal ability (stripped): no one
    EXPECT_EQ(status.hp[CHARA_GORO], 40);
    wielding_.weapon().flags = expanded::kAbilityHeal;
    expanded::HealTicked();
    EXPECT_EQ(status.hp[CHARA_TOAN], 0);
    EXPECT_EQ(status.hp[CHARA_RUBY], 100);
    EXPECT_EQ(status.hp[CHARA_GORO], 41);
    EXPECT_EQ(status.hp[CHARA_UNGAGA], 41);
    EXPECT_EQ(status.hp[CHARA_OSMOND], 41);
    EXPECT_EQ(status.hp[CHARA_XIAO], 40); // the tick heals her itself
    BtActStatus.in_presentation = 1;      // a chest opening
    expanded::HealTicked();
    EXPECT_EQ(status.hp[CHARA_GORO], 41);
}

TEST_F(GuardianReflectorBySuperSteve, ItsSpheresHealHealsTheParty) {
    floor_.status().max_hp[CHARA_GORO] = 100;
    floor_.status().hp[CHARA_GORO] = 40;
    expanded::HealTicked();
    EXPECT_EQ(floor_.status().hp[CHARA_GORO], 40);
    wielding_.weapon().flags = expanded::kAbilityHeal; // the sphere's Heal, among Super Steve's live abilities
    expanded::HealTicked();
    EXPECT_EQ(floor_.status().hp[CHARA_GORO], 41);
}

TEST_F(GuardianReflectorWithTheShooter, AnotherWeaponHealsNoOneElse) {
    wielding_.weapon().flags = expanded::kAbilityHeal;
    floor_.status().max_hp[CHARA_GORO] = 100;
    floor_.status().hp[CHARA_GORO] = 40;
    expanded::HealTicked();
    EXPECT_EQ(floor_.status().hp[CHARA_GORO], 40);
}
