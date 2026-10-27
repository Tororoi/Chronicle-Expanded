#include <gtest/gtest.h>
#include <libpad.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "audio/mixer.hpp"
#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/ungaga/hercules_wrath.hpp"
#include "expanded/weapons/ungaga/mirage.hpp"
#include "expanded/weapons/ungaga/mirage/clone.hpp"
#include "framevu1.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "platform/input.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

namespace mirage = expanded::mirage;
using expanded::AirStrikePhase;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// The decoy's model, stood in for: a root and, 10 over it, the bone the weapon hangs from.
struct Body {
    CFrameVu1   frames[2];
    MOTION_INFO keys[16] = {};
};

Body body;

bool FakeBody(CCharacter &chara, CDataAlloc2<1> &, const char *, const char *) {
    chara.Initialize();
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

// The sparkle, stood in for: one slot with e508_ex's clips, and an unlit frame under its root.
CFrameVu1   sparkle_frames[2];
MOTION_INFO sparkle_keys[4] = {
    {2,  16,  0.3f, 0},
    {21, 80,  0.7f, 0},
    {81, 96,  0.3f, 0},
    {96, 141, 0.2f, 0},
};

bool FakeSparkle(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    for (CFrameVu1 &frame : sparkle_frames) {
        frame.Initialize();
    }
    sparkle_frames[1].SetParent(&sparkle_frames[0]);
    sparkle_frames[1].attr.use_color = 1;
    for (float &channel : sparkle_frames[1].attr.color) {
        channel = 128.0f;
    }
    effect.chara[0].Initialize();
    effect.chara[0].frame = &sparkle_frames[0];
    effect.chara[0].motion_type.motion_info = sparkle_keys;
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

// Ungaga with weapon_no (attack 10, carrying sphere when given; 100 WHP left on his equipped record) at the origin facing
// +z, the spear's tip 15.66 along it; the decoy's model and the sparkle faked.
class AirStrike : public ::testing::Test {
protected:
    explicit AirStrike(int weapon_no = ITEM_WEAPON_HERCULES_WRATH, int sphere = 0, int chara = CHARA_UNGAGA) : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        ASSERT_TRUE(scePadInit(0) == 1);
        ASSERT_TRUE(scePadPortOpen(0, 0, pad_dma[0]->bytes.data()) == 1);
        ASSERT_TRUE(scePadPortOpen(1, 0, pad_dma[1]->bytes.data()) == 1);
        BtActStatus = {};
        CUserStatus &status = floor_.status();
        status.cur_chara = chara_;
        wielding_.weapon().attack = 10;
        status.equipped_weapon_slot[chara_] = 0;
        status.chara_weapons[chara_][0] = wielding_.weapon();
        status.chara_weapons[chara_][0].durability = 100;
        status.chara_weapons[chara_][0].durability_f = 100.0f;
        saved_event_ = BtEventMode;
        BtEventMode = 0;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_cursor_ = CharaMain.motion_type.state.time;
        saved_weapon_ = NowWeapon;
        weapon_.Initialize();
        for (CFrameVu1 &frame : weapon_frames_) {
            frame.Initialize();
        }
        std::strncpy(weapon_frames_[1].name, "dcol0", sizeof(weapon_frames_[1].name) - 1);
        weapon_frames_[1].local[3][2] = 15.66f;
        weapon_frames_[1].SetParent(&weapon_frames_[0]);
        weapon_.frame = &weapon_frames_[0];
        NowWeapon = &weapon_;
        saved_map_ = NowDngMap;
        NowDngMap = nullptr;
        saved_data_ = NowColData;
        NowColData = data_.get();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeBody);
        expanded::SetShotEffectLoader(FakeSparkle);
        expanded::ShotEffectsClear();
        expanded::EnterShotEffect(expanded::AirStrikeSparkle(), expanded::kSparkleShots, expanded::kSparkleFile);
        expanded::MirageReset();
        expanded::AirStrikeReset();
        expanded::SceneLightReset();
        GuardButton(true);
        Guard(true);
    }

    void TearDown() override {
        GuardButton(false);
        InputSetOverride(0, nullptr);
        expanded::AirStrikeReset();
        expanded::MirageReset();
        expanded::SceneLightReset();
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        NowColData = saved_data_;
        NowDngMap = saved_map_;
        NowWeapon = saved_weapon_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        CharaMain.motion_type.state.time = saved_cursor_;
        BtEventMode = saved_event_;
        BtActStatus = {};
    }

    void Guard(bool on) { BtActStatus.guard_mode = on ? 5 : 0; }

    void LetGo() {
        GuardButton(false);
        Guard(false);
    }

    // The frames, as the game steps them: the strike, then the decoy.
    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::AirStrikeStep();
            expanded::MirageStep();
        }
    }

    // The guard held until the decoy stands and the strike is charging, then until it is primed.
    void Plant() { Steps(expanded::kMirageGuardFrames + 1); }

    void Prime() {
        Plant();
        Steps(expanded::kAirStrikeChargeFrames);
    }

    // Ungaga's first combo swing at its cursor.
    void Swing(float cursor) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 0x24;
        CharaMain.motion_type.state.time = cursor;
    }

    void SwingDone() {
        BtActStatus.action_on = 0;
        BtActStatus.action_no = 0;
        CharaMain.motion_type.state.time = 0.0f;
    }

    CSHOT_EFFECT &Effect() { return *expanded::ShotEffectAt(expanded::ShotEffectIndex(expanded::AirStrikeSparkle())); }

    CCharacter &Sparkle() { return Effect().chara[0]; }

    // The sparkle's clip on frame, as the effect's draw steps it.
    void SparkleAt(float frame) {
        Sparkle().motion_type.state.time = frame;
        Steps(1);
    }

    // The spear the decoy holds: its tip in the world.
    std::array<float, 3> CopysTip() {
        CCharacter *held = mirage::CloneWeapon();
        EXPECT_NE(held, nullptr);
        CFrame *tip = held->frame->SearchFrame(const_cast<char *>("dcol0"));
        EXPECT_NE(tip, nullptr);
        float origin[4] = {0.0f, 0.0f, 0.0f, 1.0f}, world[4];
        tip->GetWorldPosition(world, origin);
        return {world[0], world[1], world[2]};
    }

    // monster in play standing at (x, 0, z), one body sphere of radius 5 there.
    void Stand(int monster, float x, float z) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = -1;
        live.hp = 1000;
        live.knockback[2] = 1.0f;
        floor_.unit().chara[monster][0].SetPosition(x, 0.0f, z);
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.timer[0] = 1;
        effect.radius[0] = 5.0f;
        effect.position[0][0] = x;
        effect.position[0][2] = z;
    }

    // The planted hit on a monster at (x, z), or null.
    const COLLISION_HIT *HitAt(float x, float z) {
        for (int i = 0; i < 96; i++) {
            if (NowColData->active[i] != 0 && std::fabs(NowColData->hit[i].pos[0] - x) < 0.01f &&
                std::fabs(NowColData->hit[i].pos[2] - z) < 0.01f) {
                return &NowColData->hit[i];
            }
        }
        return nullptr;
    }

    float Whp() { return floor_.status().chara_weapons[chara_][0].durability_f; }

    float Gold(const CCharacter &character) {
        float ambient[4] = {};
        expanded::AirStrikeLight(character, ambient);
        return ambient[0];
    }

    TestFloor                       floor_;
    Wielding                        wielding_;
    int                             chara_;
    CCharacter                      weapon_;
    CFrameVu1                       weapon_frames_[2];
    CCharacter                     *saved_weapon_ = nullptr;
    std::unique_ptr<CCollisionData> data_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_data_ = nullptr;
    CDungeonMap                    *saved_map_ = nullptr;
    float                           saved_pos_[3] = {};
    float                           saved_rot_[4] = {};
    float                           saved_cursor_ = 0.0f;
    s32                             saved_event_ = 0;
};

class AirStrikeWithTheMirage : public AirStrike {
protected:
    AirStrikeWithTheMirage() : AirStrike(ITEM_WEAPON_MIRAGE) {}
};

// Xiao with Super Steve carrying Hercules' Wrath's sphere, a pellet of hers in slot 3 at (0, 10, 0).
class AirStrikeBySuperSteve : public AirStrike {
protected:
    AirStrikeBySuperSteve() : AirStrike(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HERCULES_WRATH, CHARA_XIAO) {}

    void Fire() {
        *shots_ = CSHOT{};
        shots_->used[3] = 1;
        shots_->pos[3][1] = 10.0f;
        expanded::PelletFired(*shots_, 3);
    }

    void Ended(float x, float y, float z) {
        float point[4] = {x, y, z, 1.0f};
        shots_->used[3] = 0;
        expanded::PelletEnded(*shots_, 3, point, true);
    }

    std::unique_ptr<CSHOT> shots_ = std::make_unique<CSHOT>();
};

class AirStrikeBySuperSteveWithTheMirage : public AirStrike {
protected:
    AirStrikeBySuperSteveWithTheMirage() : AirStrike(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_MIRAGE, CHARA_XIAO) {}
};

} // namespace

TEST_F(AirStrike, FiveSecondsMoreOfGuardDarkenTheRoomAndGildTheSpear) {
    Plant();
    ASSERT_TRUE(expanded::MirageDecoyUp());
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kCharging);
    EXPECT_FLOAT_EQ(expanded::SceneDarkness(), 0.0f);
    Steps(expanded::kAirStrikeChargeFrames / 2);
    EXPECT_NEAR(expanded::SceneDarkness(), expanded::kAirStrikeDim * 0.5f, 1e-5f);
    // The gold barely moves at first: (e^2 - 1) / (e^4 - 1) at half the charge.
    float half = static_cast<float>((std::exp(2.0) - 1.0) / (std::exp(4.0) - 1.0));
    EXPECT_NEAR(expanded::AirStrikeGold(), half, 1e-5f);
    EXPECT_NEAR(Gold(weapon_), expanded::kAirStrikeGold[0] * half, 1e-3f);
    Steps(expanded::kAirStrikeChargeFrames / 2 - 1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kCharging);
    Steps(1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kPrimed);
    EXPECT_FLOAT_EQ(expanded::SceneDarkness(), expanded::kAirStrikeDim);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 1.0f);
}

TEST_F(AirStrike, TheGoldIsOnTheSpearAndTheDecoysAlone) {
    Prime();
    float ambient[4] = {};
    expanded::AirStrikeLight(weapon_, ambient);
    EXPECT_FLOAT_EQ(ambient[0], 150.0f);
    EXPECT_FLOAT_EQ(ambient[1], 130.0f);
    EXPECT_FLOAT_EQ(ambient[2], 50.0f);
    ASSERT_NE(mirage::CloneWeapon(), nullptr);
    EXPECT_FLOAT_EQ(Gold(*mirage::CloneWeapon()), 150.0f);
    EXPECT_FLOAT_EQ(Gold(CharaMain), 0.0f);
    EXPECT_FLOAT_EQ(Gold(*mirage::CloneCharacter()), 0.0f);
}

TEST_F(AirStrike, LettingTheGuardGoEarlyEasesItAllBack) {
    Plant();
    Steps(100);
    float dark = expanded::SceneDarkness(), gold = expanded::AirStrikeGold();
    ASSERT_GT(dark, 0.0f);
    ASSERT_GT(gold, 0.0f);
    LetGo();
    Steps(1); // the charge dropped where it stood
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
    EXPECT_FLOAT_EQ(expanded::SceneDarkness(), dark);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), gold);
    Steps(expanded::kAirStrikeReleaseFrames / 2);
    EXPECT_NEAR(expanded::SceneDarkness(), dark * 0.5f, 1e-5f);
    EXPECT_NEAR(expanded::AirStrikeGold(), gold * 0.5f, 1e-5f);
    Steps(expanded::kAirStrikeReleaseFrames / 2);
    EXPECT_FLOAT_EQ(expanded::SceneDarkness(), 0.0f);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 0.0f);
    EXPECT_TRUE(expanded::MirageDecoyUp()); // the first level stands
}

TEST_F(AirStrike, ANewChargeTakesOverTheEase) {
    Plant();
    Steps(100);
    LetGo();
    Steps(5);
    GuardButton(true);
    Guard(true);
    Plant(); // a new hold: a hand-off to a new decoy, and a new charge
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kCharging);
    Steps(expanded::kAirStrikeReleaseFrames);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kCharging);
    EXPECT_GT(expanded::AirStrikeGold(), 0.0f); // the charge's own gold, not eased away
}

TEST_F(AirStrike, PrimedItWaitsAsLongAsADecoyStandsAndGoesAsItDissolves) {
    Prime();
    LetGo(); // primed, the guard is free
    // Each frame reads the decoy's age before it steps: the last of these reads it with half its fade left.
    int left = expanded::MirageDecoyLife() - expanded::MirageDecoyAge();
    Steps(left - expanded::kMirageHazeRampFrames - expanded::kMirageFadeFrames / 2 + 1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kPrimed);
    EXPECT_NEAR(expanded::AirStrikeGold(), 0.5f, 1e-5f);
    EXPECT_NEAR(expanded::SceneDarkness(), expanded::kAirStrikeDim * 0.5f, 1e-5f);
    Steps(expanded::kMirageFadeFrames / 2);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 0.0f);
    // A swing as it has dissolved calls nothing.
    Swing(expanded::kAirStrikeSwingFrame);
    Steps(1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kPrimed);
    SwingDone();
    Steps(expanded::kMirageHazeRampFrames + 1);
    EXPECT_FALSE(expanded::MirageDecoyUp());
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
    EXPECT_FLOAT_EQ(expanded::SceneDarkness(), 0.0f);
}

TEST_F(AirStrike, ANewDecoyKeepsItPrimed) {
    Prime();
    LetGo();
    Steps(600);
    GuardButton(true);
    Guard(true);
    Plant(); // a hand-off to a new decoy
    EXPECT_TRUE(expanded::MirageHandingOff());
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kPrimed);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 1.0f);
}

TEST_F(AirStrike, TheSwingCallsTheSparkleAtTheDecoysSpearTip) {
    CharaMain.pos[0] = 30.0f;
    CharaMain.pos[2] = 40.0f;
    Prime();
    CharaMain.pos[0] = -100.0f; // he walks off; the strike is on the decoy
    // Not before the swing's hit window has ended.
    Swing(expanded::kAirStrikeSwingFrame - 0.3f);
    Steps(1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kPrimed);
    Swing(expanded::kAirStrikeSwingFrame);
    Steps(1);
    ASSERT_EQ(expanded::AirStrikeState(), AirStrikePhase::kStriking);
    CCharacter &sparkle = Sparkle();
    EXPECT_NE(Effect().active[0], 0);
    EXPECT_EQ(sparkle.motion_no, 0);
    EXPECT_FLOAT_EQ(sparkle.motion_speed, -1.0f); // its own 0.3
    EXPECT_FLOAT_EQ(sparkle.motion_type.state.time, expanded::kSparkleFirstStart);
    auto tip = CopysTip();
    EXPECT_NEAR(tip[0], 30.0f, 1e-4f);
    EXPECT_NEAR(tip[1], 10.0f, 1e-4f);
    EXPECT_NEAR(tip[2], 40.0f + 15.66f, 1e-4f);
    EXPECT_FLOAT_EQ(sparkle.pos[0], tip[0]);
    EXPECT_FLOAT_EQ(sparkle.pos[1], tip[1]);
    EXPECT_FLOAT_EQ(sparkle.pos[2], tip[2]);
    // It rides the tip through its first clip; then its second plays where the tip was, at the decoy's height.
    SparkleAt(10.0f);
    EXPECT_EQ(sparkle.motion_no, 0);
    EXPECT_FLOAT_EQ(sparkle.pos[2], tip[2]);
    SparkleAt(expanded::kSparkleFirstEnd - 0.5f);
    EXPECT_EQ(sparkle.motion_no, 1);
    EXPECT_FLOAT_EQ(sparkle.motion_speed, expanded::kSparkleSecondRate);
    EXPECT_FLOAT_EQ(sparkle.motion_type.state.time, expanded::kSparkleSecondStart);
    SparkleAt(expanded::kSparkleSecondStart);
    EXPECT_FLOAT_EQ(sparkle.pos[0], tip[0]);
    EXPECT_FLOAT_EQ(sparkle.pos[1], 0.0f);
    EXPECT_FLOAT_EQ(sparkle.pos[2], tip[2]);
}

TEST_F(AirStrike, TheRoomPlungesToBlackThenTheBlastLandsThroughAnyGuard) {
    Prime();
    Swing(expanded::kAirStrikeSwingFrame);
    Steps(1);
    SparkleAt(expanded::kSparkleFirstEnd);
    float x = 0.0f, z = 15.66f; // where the second clip plays
    // Within 20, 50, 80 and 100 of it (to the near edges of their bodies), and past it.
    Stand(0, x + 15.0f, z);
    Stand(1, x, z + 45.0f);
    Stand(2, x - 75.0f, z);
    Stand(3, x, z - 95.0f);
    Stand(4, x + 110.0f, z);
    SparkleAt(expanded::kSparkleDarkFrom);
    EXPECT_NEAR(expanded::SceneDarkness(), expanded::kAirStrikeDim, 1e-5f);
    SparkleAt((expanded::kSparkleDarkFrom + expanded::kSparkleBlastFrame) / 2.0f);
    float darker = expanded::SceneDarkness();
    EXPECT_GT(darker, expanded::kAirStrikeDim);
    EXPECT_LT(darker, (expanded::kAirStrikeDim + 1.0f) / 2.0f); // barely moving at first
    SparkleAt(expanded::kSparkleBlastFrame - 0.2f);
    EXPECT_GT(expanded::SceneDarkness(), 0.9f);
    EXPECT_FALSE(expanded::SceneFlashing());
    EXPECT_TRUE(expanded::MirageDecoyUp());
    SparkleAt(expanded::kSparkleBlastFrame);
    const int   damages[] = {40, 30, 20, 10};
    const float at[][2] = {
        {x + 15.0f, z        },
        {x,         z + 45.0f},
        {x - 75.0f, z        },
        {x,         z - 95.0f},
    };
    for (int monster = 0; monster < 4; monster++) {
        const COLLISION_HIT *hit = HitAt(at[monster][0], at[monster][1]);
        ASSERT_NE(hit, nullptr) << monster;
        EXPECT_EQ(hit->damage, damages[monster]) << monster;
        EXPECT_TRUE(expanded::BlastHit(*hit));
        EXPECT_FALSE(expanded::HitWearsWeapon(*hit)); // the strike bills its own
    }
    EXPECT_EQ(HitAt(x + 110.0f, z), nullptr);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - expanded::kAirStrikeWhp);
    EXPECT_FALSE(expanded::MirageDecoyUp()); // dispelled
    EXPECT_TRUE(expanded::SceneFlashing());
    // Once.
    SparkleAt(40.0f);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - expanded::kAirStrikeWhp);
}

TEST_F(AirStrike, TheSparkleAndTheGoldFadeOutTogether) {
    Prime();
    Swing(expanded::kAirStrikeSwingFrame);
    Steps(1);
    SparkleAt(expanded::kSparkleFirstEnd);
    SparkleAt(expanded::kSparkleBlastFrame);
    SparkleAt(expanded::kSparkleFadeFrom - 1.0f);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 1.0f);
    EXPECT_FLOAT_EQ(sparkle_frames[1].attr.color[0], 128.0f);
    SparkleAt((expanded::kSparkleFadeFrom + expanded::kSparkleFadeTo) / 2.0f);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 0.5f);
    EXPECT_FLOAT_EQ(sparkle_frames[1].attr.color[0], 64.0f);
    SparkleAt(expanded::kSparkleFadeTo);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
    EXPECT_EQ(Effect().active[0], 0);
    EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 0.0f);
    EXPECT_FLOAT_EQ(sparkle_frames[1].attr.color[0], 128.0f); // handed back at full
    EXPECT_TRUE(expanded::SceneFlashing());                   // the flash eases on
}

TEST_F(AirStrike, HoldingTheGuardOnChargesNothingMore) {
    Prime();
    Swing(expanded::kAirStrikeSwingFrame);
    Steps(1);
    SwingDone();
    SparkleAt(expanded::kSparkleFirstEnd);
    SparkleAt(expanded::kSparkleFadeTo);
    ASSERT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
    Steps(30);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
    LetGo();
    Steps(1);
    GuardButton(true);
    Guard(true);
    Plant();
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kCharging);
}

TEST_F(AirStrike, TheSparkleNotLoadedBlastsAtOnce) {
    Prime();
    expanded::ShotEffectsClear();
    Stand(0, 0.0f, 5.0f);
    Swing(expanded::kAirStrikeSwingFrame);
    Steps(1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
    ASSERT_NE(HitAt(0.0f, 5.0f), nullptr);
    EXPECT_EQ(HitAt(0.0f, 5.0f)->damage, 40); // on the decoy's spot
    EXPECT_FLOAT_EQ(Whp(), 100.0f - expanded::kAirStrikeWhp);
    EXPECT_TRUE(expanded::SceneFlashing());
}

TEST_F(AirStrike, PuttingTheSpearAwayEndsItAll) {
    Prime();
    Swing(expanded::kAirStrikeSwingFrame);
    Steps(1);
    {
        Wielding halberd(ITEM_WEAPON_HALBERT);
        Steps(1);
        EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
        EXPECT_EQ(Effect().active[0], 0);
        EXPECT_FLOAT_EQ(expanded::SceneDarkness(), 0.0f);
        EXPECT_FLOAT_EQ(expanded::AirStrikeGold(), 0.0f);
    }
    EXPECT_FLOAT_EQ(Whp(), 100.0f); // no blast
}

TEST_F(AirStrikeWithTheMirage, TheMirageChargesNoStrike) {
    Prime();
    EXPECT_TRUE(expanded::MirageDecoyUp());
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
    EXPECT_FLOAT_EQ(expanded::SceneDarkness(), 0.0f);
    EXPECT_FLOAT_EQ(Gold(weapon_), 0.0f);
}

TEST_F(AirStrikeBySuperSteve, HerFirstPrimedPelletStrikesWhereItEnds) {
    Plant();
    Steps(10);
    Fire(); // charging: not the strike
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kCharging);
    Steps(expanded::kAirStrikeChargeFrames);
    ASSERT_EQ(expanded::AirStrikeState(), AirStrikePhase::kPrimed);
    EXPECT_FLOAT_EQ(Gold(weapon_), 150.0f); // the slingshot
    Swing(expanded::kAirStrikeSwingFrame);  // no swing of hers calls it
    Steps(1);
    SwingDone();
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kPrimed);
    Fire();
    Steps(1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kFlying);
    EXPECT_FLOAT_EQ(expanded::SceneDarkness(), expanded::kAirStrikeDim);
    Ended(60.0f, 12.0f, 80.0f);
    ASSERT_EQ(expanded::AirStrikeState(), AirStrikePhase::kStriking);
    CCharacter &sparkle = Sparkle();
    EXPECT_FLOAT_EQ(sparkle.pos[0], 60.0f);
    EXPECT_FLOAT_EQ(sparkle.pos[1], 12.0f);
    EXPECT_FLOAT_EQ(sparkle.pos[2], 80.0f);
    // No floor found under it: the second clip and the blast at the decoy's height.
    Stand(0, 70.0f, 80.0f);
    SparkleAt(expanded::kSparkleFirstEnd);
    SparkleAt(expanded::kSparkleBlastFrame);
    EXPECT_FLOAT_EQ(sparkle.pos[0], 60.0f);
    EXPECT_FLOAT_EQ(sparkle.pos[1], 0.0f);
    EXPECT_FLOAT_EQ(sparkle.pos[2], 80.0f);
    ASSERT_NE(HitAt(70.0f, 80.0f), nullptr);
    EXPECT_EQ(HitAt(70.0f, 80.0f)->damage, 40);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - expanded::kAirStrikeWhp); // Super Steve's
    EXPECT_FALSE(expanded::MirageDecoyUp());
}

TEST_F(AirStrikeBySuperSteve, APelletStillOutAfterThreeSecondsStrikesWhereItIs) {
    Prime();
    Fire();
    shots_->pos[3][0] = 200.0f;
    Steps(expanded::kAirStrikeMissFrames - 1);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kFlying);
    Steps(1);
    ASSERT_EQ(expanded::AirStrikeState(), AirStrikePhase::kStriking);
    EXPECT_FLOAT_EQ(Sparkle().pos[0], 200.0f);
}

TEST_F(AirStrikeBySuperSteve, OnceAwayItStrikesThoughTheDecoyGoes) {
    Prime();
    Fire();
    Steps(1);
    expanded::MirageDispel();
    Steps(5);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kFlying);
    Ended(10.0f, 10.0f, 10.0f);
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kStriking);
}

TEST_F(AirStrikeBySuperSteveWithTheMirage, AMirageSphereChargesNoStrike) {
    Prime();
    EXPECT_TRUE(expanded::MirageDecoyUp());
    EXPECT_EQ(expanded::AirStrikeState(), AirStrikePhase::kIdle);
}

TEST(AirStrikeSound, TheLightsSoundIsCutAndFadedOut) {
    // A cut sounds for its own stretch of the note (an own sound's frames only time the duck).
    ASSERT_EQ(expanded::kAirStrikeSound.cut_count, 1);
    const expanded::OwnCut &cut = expanded::kAirStrikeSound.cuts[0];
    EXPECT_EQ(cut.note, expanded::kAirStrikeSound.note);
    EXPECT_FLOAT_EQ(cut.from, 0.0f);
    EXPECT_FLOAT_EQ(cut.to * 60.0f, static_cast<float>(expanded::kAirStrikeSoundFrames));
    EXPECT_FLOAT_EQ(cut.fade * 60.0f, static_cast<float>(expanded::kAirStrikeSoundFadeFrames));
    // The glint's twinkle plays whole.
    EXPECT_EQ(expanded::kAirStrikeTwinkle.cut_count, 0);
}

// The loudest sample of the mixer's next seconds of output.
float Peak(float seconds) {
    audio::Mixer      &mixer = audio::DefaultMixer();
    int                frames = static_cast<int>(seconds * static_cast<float>(mixer.Rate()));
    std::vector<float> out(2 * static_cast<std::size_t>(frames));
    mixer.Render(out.data(), frames);
    float peak = 0.0f;
    for (float sample : out) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

// DC_TEST_DATA, an extracted data directory: both sounds load from their sets' banks and sound, the light's going
// silent as its cut ends.
TEST(AirStrikeSound, BothSoundsPlayFromTheirBanks) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    expanded::test::DataRootScope root(data);
    ASSERT_TRUE(expanded::LoadOwnSound(expanded::kAirStrikeTwinkle));
    ASSERT_TRUE(expanded::LoadOwnSound(expanded::kAirStrikeSound));
    ASSERT_TRUE(expanded::PlayOwnSound(expanded::kAirStrikeTwinkle, false));
    EXPECT_GT(Peak(0.2f), 0.0f);
    Peak(1.0f); // played out
    ASSERT_TRUE(expanded::PlayOwnSound(expanded::kAirStrikeSound, false));
    EXPECT_GT(Peak(0.5f), 0.0f);
    Peak(static_cast<float>(expanded::kAirStrikeSoundFrames) / 60.0f - 0.5f + 0.05f);
    EXPECT_EQ(Peak(1.0f), 0.0f);
}
