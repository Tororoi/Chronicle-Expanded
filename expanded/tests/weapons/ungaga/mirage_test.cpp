#include <gtest/gtest.h>
#include <libpad.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include "btactstatus.hpp"
#include "btitem.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/weapons/abilities.hpp"
#include "expanded/weapons/ungaga/mirage.hpp"
#include "expanded/weapons/ungaga/mirage/clone.hpp"
#include "expanded/weapons/ungaga/mirage/heat_haze.hpp"
#include "framevu1.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "platform/input.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

namespace mirage = expanded::mirage;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// The copy's model, stood in for: a root and, under it, the bone the weapon hangs from, one block of frames as a model's
// are; its keys' 9th the guard loop.
struct Body {
    CFrameVu1   frames[2];
    MOTION_INFO keys[16] = {};
};

Body        body;
std::string loaded_path, loaded_cfg;

bool FakeBody(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *cfg) {
    chara.Initialize();
    loaded_path = path;
    loaded_cfg = cfg;
    for (CFrameVu1 &frame : body.frames) {
        frame.Initialize();
    }
    std::strncpy(body.frames[1].name, "weapon", sizeof(body.frames[1].name) - 1);
    body.frames[1].local[3][1] = 10.0f;
    body.frames[1].SetParent(&body.frames[0]);
    for (MOTION_INFO &key : body.keys) {
        key = {1, 2, 1.0f, 0};
    }
    body.keys[expanded::kMirageGuardLoop] = {215, 225, 0.2f, 0};
    body.keys[15] = {-1, -1, -1.0f, 0};
    chara.frame = &body.frames[0];
    chara.motion_type.motion_info = body.keys;
    return true;
}

struct alignas(64) PadDmaBuffer {
    std::array<unsigned char, 1024> bytes;
};

std::array<std::unique_ptr<PadDmaBuffer>, 2> pad_dma = {std::make_unique<PadDmaBuffer>(), std::make_unique<PadDmaBuffer>()};

// The guard button down or up, as the game reads it.
void GuardButton(bool down) {
    InputPadState state;
    state.connected = true;
    state.buttons = down ? PAD_R1 : 0;
    InputSetOverride(0, &state);
    for (int i = 0; i < 4; i++) {
        GamePad.UpDate();
    }
}

// Ungaga with weapon_no (carrying sphere when given) at the origin facing +x, holding the guard; monsters 0 and 1 in play
// 100 out, monster 2 not set up; the copy's model faked, and the weapon in hand a single frame.
class Mirage : public ::testing::Test {
protected:
    explicit Mirage(int weapon_no = ITEM_WEAPON_MIRAGE, int sphere = 0, int chara = CHARA_UNGAGA) : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        ASSERT_TRUE(scePadInit(0) == 1);
        ASSERT_TRUE(scePadPortOpen(0, 0, pad_dma[0]->bytes.data()) == 1);
        ASSERT_TRUE(scePadPortOpen(1, 0, pad_dma[1]->bytes.data()) == 1);
        BtActStatus = {};
        floor_.status().cur_chara = chara_;
        saved_event_ = BtEventMode;
        BtEventMode = 0;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
        }
        CharaMain.GetRotation(saved_rot_);
        Move(0.0f, 0.0f);
        CharaMain.SetRotation(0.0f, 1.5f, 0.0f);
        saved_weapon_ = NowWeapon;
        weapon_.Initialize();
        weapon_frame_.Initialize();
        weapon_.frame = &weapon_frame_;
        NowWeapon = &weapon_;
        loaded_path.clear();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeBody);
        expanded::MirageReset();
        Stand(0, 0.0f, 100.0f);
        Stand(1, 100.0f, 0.0f);
        GuardButton(true);
        Guard(true);
    }

    void TearDown() override {
        GuardButton(false);
        InputSetOverride(0, nullptr);
        expanded::MirageReset();
        expanded::ConfuseFloorReset(false);
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        NowWeapon = saved_weapon_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        BtEventMode = saved_event_;
        BtActStatus = {};
    }

    void Guard(bool on) { BtActStatus.guard_mode = on ? 5 : 0; }

    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::MirageStep();
        }
    }

    // The guard held until the decoy is planted.
    void Plant() { Steps(expanded::kMirageGuardFrames + 1); }

    // The guard let go, then raised and held until the next plant.
    void Replant() {
        GuardButton(false);
        Guard(false);
        Steps(1);
        GuardButton(true);
        Guard(true);
        Plant();
    }

    void Move(float x, float z) {
        CharaMain.pos[0] = x;
        CharaMain.pos[1] = 0.0f;
        CharaMain.pos[2] = z;
        CharaMain.pos[3] = 1.0f;
    }

    // monster in play standing at (x, 0, z).
    void Stand(int monster, float x, float z) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = -1;
        live.hp = 100;
        floor_.unit().chara[monster][0].pos[0] = x;
        floor_.unit().chara[monster][0].pos[2] = z;
    }

    // Where monster's script finds the player.
    std::array<float, 4> Finds(int monster) {
        std::array<float, 4> found = {};
        NowMonstorUnit->current_monster = monster;
        expanded::ScriptPlayerPosition(found.data());
        return found;
    }

    void Hit(int monster) {
        COLLISION_HIT hit = {};
        expanded::MonsterHitTaken(floor_.unit(), monster, hit);
    }

    TestFloor   floor_;
    Wielding    wielding_;
    int         chara_;
    CCharacter  weapon_;
    CFrameVu1   weapon_frame_;
    CCharacter *saved_weapon_ = nullptr;
    float       saved_pos_[3] = {};
    float       saved_rot_[4] = {};
    s32         saved_event_ = 0;
};

class MirageOfHercules : public Mirage {
protected:
    MirageOfHercules() : Mirage(ITEM_WEAPON_HERCULES_WRATH) {}
};

class MirageWithTheHalberd : public Mirage {
protected:
    MirageWithTheHalberd() : Mirage(ITEM_WEAPON_HALBERT) {}
};

class MirageBySuperSteve : public Mirage {
protected:
    MirageBySuperSteve() : Mirage(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_MIRAGE, CHARA_XIAO) {}
};

} // namespace

TEST_F(Mirage, AQuarterSecondOfGuardPlantsTheDecoyWhereUngagaStands) {
    Steps(expanded::kMirageGuardFrames);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    Move(30.0f, 40.0f); // where the last frame of the hold finds him
    Steps(1);
    ASSERT_TRUE(expanded::MirageDecoyUp());
    float at[4];
    expanded::MirageDecoyPlace(at);
    EXPECT_FLOAT_EQ(at[0], 30.0f);
    EXPECT_FLOAT_EQ(at[2], 40.0f);
    EXPECT_EQ(expanded::MirageDecoyLife(), expanded::kMirageFrames);
}

TEST_F(Mirage, TheClockRunsWhileTheButtonStaysDown) {
    Steps(5);
    Guard(false); // out of the pose a moment (a blocked hit), the button down
    Steps(5);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    Guard(true);
    Steps(5);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    Steps(1);
    EXPECT_TRUE(expanded::MirageDecoyUp());
}

TEST_F(Mirage, LettingTheButtonGoStartsTheClockAgain) {
    Steps(expanded::kMirageGuardFrames - 1);
    GuardButton(false);
    Guard(false);
    Steps(1);
    GuardButton(true);
    Guard(true);
    Steps(expanded::kMirageGuardFrames);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    Steps(1);
    EXPECT_TRUE(expanded::MirageDecoyUp());
}

TEST_F(Mirage, MonstersGoForTheDecoyForTwelveSeconds) {
    Plant();
    Move(-200.0f, -200.0f);
    for (int monster : {0, 1}) {
        auto found = Finds(monster);
        EXPECT_FLOAT_EQ(found[0], 0.0f);
        EXPECT_FLOAT_EQ(found[2], 0.0f);
    }
    // One plant a guard: holding on plants nothing more.
    Steps(expanded::kMirageFrames - 1);
    ASSERT_TRUE(expanded::MirageDecoyUp());
    EXPECT_EQ(expanded::MirageDecoyAge(), expanded::kMirageFrames - 1);
    EXPECT_FLOAT_EQ(Finds(0)[0], 0.0f);
    Steps(1);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    EXPECT_FLOAT_EQ(Finds(0)[0], -200.0f);
    EXPECT_EQ(mirage::CloneCharacter(), nullptr);
}

TEST_F(MirageOfHercules, HerculesWrathsDecoyStandsEighteenSeconds) {
    Plant();
    EXPECT_EQ(expanded::MirageDecoyLife(), expanded::kHerculesMirageFrames);
    Steps(expanded::kHerculesMirageFrames - 1);
    EXPECT_TRUE(expanded::MirageDecoyUp());
    Steps(1);
    EXPECT_FALSE(expanded::MirageDecoyUp());
}

TEST_F(Mirage, AMonsterHitTurnsBackOnUngagaAlone) {
    Plant();
    Move(-200.0f, 0.0f);
    Hit(0);
    EXPECT_FLOAT_EQ(Finds(0)[0], -200.0f);
    EXPECT_FLOAT_EQ(Finds(1)[0], 0.0f);
    // One not in play finds him where he is; one set up while the decoy stands goes for it.
    EXPECT_FLOAT_EQ(Finds(2)[0], -200.0f);
    Stand(2, 50.0f, 50.0f);
    EXPECT_FLOAT_EQ(Finds(2)[0], 0.0f);
    Steps(10);
    EXPECT_FLOAT_EQ(Finds(0)[0], -200.0f);
}

TEST_F(Mirage, ANewHoldHandsOffToAFreshDecoy) {
    Plant();
    Steps(100);
    Hit(0);
    Move(80.0f, 0.0f);
    Replant();
    ASSERT_TRUE(expanded::MirageHandingOff());
    float at[4];
    expanded::MirageDecoyPlace(at);
    EXPECT_FLOAT_EQ(at[0], 80.0f);
    EXPECT_EQ(expanded::MirageDecoyAge(), 0); // the new decoy's time starts at once
    // The old copy dissolves where it stands, the shimmer rising for the new one.
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 1.0f);
    float root[3];
    ASSERT_TRUE(mirage::CloneRoot(root));
    EXPECT_FLOAT_EQ(root[0], 0.0f);
    // The monsters keep to the old spot, and the one hit keeps after Ungaga.
    EXPECT_FLOAT_EQ(Finds(1)[0], 0.0f);
    EXPECT_FLOAT_EQ(Finds(0)[0], 80.0f);
    Steps(expanded::kMirageHandoffFrames - 1);
    EXPECT_NEAR(expanded::MirageCloneAlpha(), 1.0f / expanded::kMirageHandoffFrames, 1e-5f);
    Steps(1);
    EXPECT_FALSE(expanded::MirageHandingOff());
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 0.0f); // the copy at the new spot, still clear
    ASSERT_TRUE(mirage::CloneRoot(root));
    EXPECT_FLOAT_EQ(root[0], 80.0f);
    // Until the new copy has fully appeared.
    Steps(expanded::kMirageAggroHoldFrames - expanded::kMirageHandoffFrames - 1);
    EXPECT_FLOAT_EQ(Finds(1)[0], 0.0f);
    EXPECT_LT(expanded::MirageCloneAlpha(), 1.0f);
    Steps(1);
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 1.0f);
    EXPECT_FLOAT_EQ(Finds(1)[0], 80.0f);
    EXPECT_FLOAT_EQ(Finds(0)[0], 80.0f); // the one hit falls for the new one too
}

TEST_F(Mirage, TheCopyAndItsShimmerComeAndGoInTurn) {
    Plant();
    EXPECT_FLOAT_EQ(expanded::MirageHazeGain(), 0.0f);
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 0.0f);
    Steps(expanded::kMirageHazeRampFrames);
    EXPECT_FLOAT_EQ(expanded::MirageHazeGain(), 1.0f);
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 0.0f);
    Steps(expanded::kMirageFadeFrames / 2);
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 0.5f);
    Steps(expanded::kMirageFadeFrames / 2);
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 1.0f);
    // At the end the copy goes first, the shimmer last.
    Steps(expanded::kMirageFrames - expanded::kMirageHazeRampFrames - expanded::kMirageFadeFrames - expanded::kMirageAggroHoldFrames);
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 1.0f);
    Steps(expanded::kMirageFadeFrames);
    EXPECT_FLOAT_EQ(expanded::MirageCloneAlpha(), 0.0f);
    EXPECT_FLOAT_EQ(expanded::MirageHazeGain(), 1.0f);
    Steps(expanded::kMirageHazeRampFrames / 3);
    EXPECT_NEAR(expanded::MirageHazeGain(), 2.0f / 3.0f, 1e-5f);
}

TEST_F(Mirage, TheCopyHoldsTheGuardWithTheWeaponInHand) {
    Move(30.0f, 40.0f);
    Plant();
    EXPECT_EQ(loaded_path, charaNameTbl[CHARA_UNGAGA]);
    EXPECT_EQ(loaded_cfg, "base.cfg");
    CCharacter *copy = mirage::CloneCharacter();
    ASSERT_NE(copy, nullptr);
    EXPECT_NE(copy, &CharaMain);
    EXPECT_EQ(copy->motion_no, expanded::kMirageGuardLoop);
    EXPECT_FLOAT_EQ(copy->pos[0], 30.0f);
    EXPECT_FLOAT_EQ(copy->pos[2], 40.0f);
    EXPECT_FLOAT_EQ(copy->rotation.y, 1.5f);
    // He walks on and turns; the copy stands.
    Move(-50.0f, 0.0f);
    CharaMain.SetRotation(0.0f, -1.0f, 0.0f);
    Steps(20);
    EXPECT_FLOAT_EQ(copy->pos[0], 30.0f);
    EXPECT_FLOAT_EQ(copy->rotation.y, 1.5f);
    CCharacter *held = mirage::CloneWeapon();
    ASSERT_NE(held, nullptr);
    EXPECT_NE(held->frame, NowWeapon->frame);
    EXPECT_EQ(held->frame->parent, &body.frames[1]);
    // The shimmer sits on it.
    float root[3];
    ASSERT_TRUE(mirage::CloneRoot(root));
    EXPECT_FLOAT_EQ(root[0], 30.0f);
    EXPECT_FLOAT_EQ(root[2], 40.0f);
}

TEST_F(Mirage, TheDecoyOwnsTheTargetsOverConfusion) {
    Stand(1, 0.0f, 110.0f); // beside monster 0, nearer it than Ungaga
    std::srand(1);
    expanded::ConfuseStep(floor_.unit(), 0);
    floor_.unit().effect[0].hit_attributes = expanded::kAbilityConfuse;
    for (int i = 0; i < 400 && Finds(0)[2] != 110.0f; i++) {
        expanded::ConfuseRoll(floor_.unit(), 0);
    }
    ASSERT_FLOAT_EQ(Finds(0)[2], 110.0f); // confused: it goes for monster 1
    Plant();
    Move(-200.0f, 0.0f);
    EXPECT_FLOAT_EQ(Finds(0)[2], 0.0f);
    Hit(0);
    EXPECT_FLOAT_EQ(Finds(0)[0], -200.0f); // the player, not its confusion's target
    Steps(expanded::kMirageFrames);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    EXPECT_FLOAT_EQ(Finds(0)[2], 110.0f);
}

TEST_F(Mirage, TheDecoyGoesWithTheWeapon) {
    Plant();
    {
        Wielding halberd(ITEM_WEAPON_HALBERT);
        Steps(1);
        EXPECT_FALSE(expanded::MirageDecoyUp());
    }
    Steps(10);
    EXPECT_FALSE(expanded::MirageDecoyUp()); // the guard still held from before
}

TEST_F(Mirage, TheDecoyGoesWithThePartyMember) {
    Plant();
    floor_.status().cur_chara = CHARA_XIAO;
    Steps(1);
    EXPECT_FALSE(expanded::MirageDecoyUp());
}

TEST_F(Mirage, TheDecoyGoesWithTheFloor) {
    Plant();
    Move(-200.0f, 0.0f);
    expanded::MirageReset();
    EXPECT_FALSE(expanded::MirageDecoyUp());
    EXPECT_EQ(mirage::CloneCharacter(), nullptr);
    EXPECT_FLOAT_EQ(Finds(0)[0], -200.0f); // the player again
}

TEST_F(MirageWithTheHalberd, OtherWeaponsPlantNothing) {
    Plant();
    Steps(30);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.0f), 1.0f);
}

TEST_F(MirageBySuperSteve, SuperSteveCarryingItsSpherePlantsXiaosDecoy) {
    Plant();
    ASSERT_TRUE(expanded::MirageDecoyUp());
    EXPECT_EQ(loaded_path, charaNameTbl[CHARA_XIAO]);
    EXPECT_EQ(expanded::MirageDecoyLife(), expanded::kMirageFrames);
    Move(-200.0f, 0.0f);
    EXPECT_FLOAT_EQ(Finds(0)[0], 0.0f);
}

TEST_F(MirageBySuperSteve, AHerculesWrathSphereStandsItsDecoyEighteenSeconds) {
    wielding_.weapon().attach[0].sphere_weapon_no = ITEM_WEAPON_HERCULES_WRATH;
    Plant();
    EXPECT_EQ(expanded::MirageDecoyLife(), expanded::kHerculesMirageFrames);
}

TEST(MirageLine, LocksOnFromTwiceAsFar) {
    TestFloor floor;
    floor.status().cur_chara = CHARA_UNGAGA;
    for (int weapon : {ITEM_WEAPON_MIRAGE, ITEM_WEAPON_TERRA_SWORD, ITEM_WEAPON_HERCULES_WRATH, ITEM_WEAPON_BABEL_S_SPEAR}) {
        Wielding held(weapon);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.0f), expanded::kMirageLineReach) << weapon;
    }
    for (int weapon : {ITEM_WEAPON_HALBERT, ITEM_WEAPON_SCORPION, ITEM_WEAPON_CACTUS}) {
        Wielding held(weapon);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.0f), 1.0f) << weapon;
    }
    floor.status().cur_chara = CHARA_XIAO;
    {
        Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_BABEL_S_SPEAR);
        EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.4f), 1.4f * expanded::kMirageLineReach);
    }
    Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HALBERT);
    EXPECT_FLOAT_EQ(expanded::PlayerLockOnRange(1.4f), 1.4f);
}

TEST(MirageHaze, TheWaveStepsAtHalfAFiresPace) {
    mirage::HazeReset();
    float last = mirage::HazePhase();
    for (int i = 0; i < 200; i++) {
        mirage::HazeStep();
        float now = mirage::HazePhase();
        if (now >= last) {
            EXPECT_LE(now - last, mirage::kHazeWaveStep);
        } else {
            EXPECT_FLOAT_EQ(now, 0.0f); // wrapped
        }
        EXPECT_LT(now, mirage::kHazeWaveCycle);
        last = now;
    }
    mirage::HazeReset();
}
