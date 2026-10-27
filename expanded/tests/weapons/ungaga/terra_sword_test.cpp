#include <gtest/gtest.h>
#include <libpad.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "audio/mixer.hpp"
#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/ground_shadow.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/ungaga/terra_sword.hpp"
#include "framevu1.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "platform/input.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern s32 lockOnTargetNo;
} // namespace dc::game

namespace {

using expanded::BigRockPhase;
using expanded::test::StepsUnpaused;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// The drops' and the shadow disc's characters, stood in for: a root and a child.
struct Rig {
    CFrameVu1 frames[2];
};

Rig                      rock_rig, nut_rig, disc_rig;
std::vector<std::string> loaded; // the packs loaded, in order

bool FakeProp(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    loaded.emplace_back(path);
    std::string name = path;
    Rig        &rig = name == expanded::kShadowDiscFile ? disc_rig : name == expanded::kNutFile ? nut_rig
                                                                                                : rock_rig;
    for (CFrameVu1 &frame : rig.frames) {
        frame.Initialize();
    }
    rig.frames[1].SetParent(&rig.frames[0]);
    chara.frame = &rig.frames[0];
    if (&rig == &disc_rig) {
        chara.shadow_frame = &rig.frames[1];
    }
    return true;
}

bool Loaded(const std::string &path) {
    return std::count(loaded.begin(), loaded.end(), path) == 1;
}

// The shockwave, stood in for: one slot with its clip 2-31.
CFrameVu1   shock_frames[2];
MOTION_INFO shock_keys[1] = {
    {2, 31, 0.5f, 0}
};

bool FakeShock(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    for (CFrameVu1 &frame : shock_frames) {
        frame.Initialize();
    }
    shock_frames[1].SetParent(&shock_frames[0]);
    effect.chara[0].Initialize();
    effect.chara[0].frame = &shock_frames[0];
    effect.chara[0].motion_type.motion_info = shock_keys;
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

constexpr float kTargetZ = 60.0f;

// Ungaga with weapon_no (attack 100, carrying sphere when given; 100 WHP left on his equipped record) at the origin
// facing +z on a floor of no polygons (so what drops comes down at its monster's height), a Skeleton Soldier in play at
// (0, 0, kTargetZ); the drops' characters, the shadow disc and the shockwave stood in for.
class BigRock : public ::testing::Test {
protected:
    explicit BigRock(int weapon_no = ITEM_WEAPON_TERRA_SWORD, int sphere = 0, int chara = CHARA_UNGAGA) : wielding_(weapon_no, sphere), chara_(chara) {}

    void SetUp() override {
        ASSERT_TRUE(scePadInit(0) == 1);
        ASSERT_TRUE(scePadPortOpen(0, 0, pad_dma[0]->bytes.data()) == 1);
        ASSERT_TRUE(scePadPortOpen(1, 0, pad_dma[1]->bytes.data()) == 1);
        BtActStatus = {};
        CUserStatus &status = floor_.status();
        status.cur_chara = chara_;
        wielding_.weapon().attack = 100;
        status.equipped_weapon_slot[chara_] = 0;
        status.chara_weapons[chara_][0] = wielding_.weapon();
        status.chara_weapons[chara_][0].durability = 100;
        status.chara_weapons[chara_][0].durability_f = 100.0f;
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_weapon_ = NowWeapon;
        weapon_.Initialize();
        NowWeapon = &weapon_;
        saved_map_ = NowDngMap;
        NowDngMap = nullptr;
        saved_data_ = NowColData;
        NowColData = data_.get();
        saved_lock_ = lockOnTargetFlag;
        saved_lock_no_ = lockOnTargetNo;
        lockOnTargetFlag = 0;
        lockOnTargetNo = 0;
        loaded.clear();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeProp);
        expanded::SetShotEffectLoader(FakeShock);
        expanded::ShotEffectsClear();
        expanded::EnterShotEffect(expanded::BigRockShockwave(), expanded::kShockShots, expanded::kShockFile);
        expanded::GroundShadowReset();
        expanded::ConfuseFloorReset(false);
        expanded::BigRockReset();
        floor_.SetUpModel(0, expanded::Monster::SKELETON_SOLDIER);
        Stand(0, 0.0f, kTargetZ);
    }

    void TearDown() override {
        GuardButton(false);
        InputSetOverride(0, nullptr);
        expanded::BigRockReset();
        expanded::ConfuseFloorReset(false);
        expanded::GroundShadowReset();
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        lockOnTargetFlag = saved_lock_;
        lockOnTargetNo = saved_lock_no_;
        NowColData = saved_data_;
        NowDngMap = saved_map_;
        NowWeapon = saved_weapon_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        BtActStatus = {};
    }

    void Guard(bool on) {
        GuardButton(on);
        BtActStatus.guard_mode = on ? 5 : 0;
    }

    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::BigRockStep();
        }
    }

    // The guard held until the drop is primed (the first frame in the pose starts the clock), then let go.
    void Prime() {
        Guard(true);
        Steps(expanded::kRockChargeFrames + 1);
        Guard(false);
    }

    void LockOn(int monster) {
        lockOnTargetFlag = 1;
        lockOnTargetNo = monster;
    }

    // Primed, locked on to monster 0: dropped this frame, its first frame of the fall with it.
    void Drop() {
        Prime();
        LockOn(0);
        Steps(1);
    }

    // Stepped until the fall ends: the frames it took, its first included.
    int FallToTheEnd() {
        int frames = 0;
        while (expanded::BigRockFalling() && frames < 1000) {
            Steps(1);
            frames++;
        }
        return frames + 1;
    }

    // monster in play standing at (x, 0, z), its life 5000, a body sphere of radius 5 at height 5.
    void Stand(int monster, float x, float z) {
        floor_.Spawn(monster, 0);
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = -1;
        live.hp = live.max_hp = 5000;
        live.knockback[2] = 1.0f;
        Move(monster, x, z);
    }

    void Move(int monster, float x, float z) {
        floor_.unit().chara[monster][0].SetPosition(x, 0.0f, z);
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.timer[0] = 1;
        effect.radius[0] = 5.0f;
        effect.position[0][0] = x;
        effect.position[0][1] = 5.0f;
        effect.position[0][2] = z;
        effect.position[0][3] = 1.0f;
    }

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

    // Every planted hit spent, as the game's step after the monsters' ends a hit of one step's life.
    void ClearHits() {
        for (int &active : NowColData->active) {
            active = 0;
        }
    }

    std::array<float, 3> At() {
        std::array<float, 3> at;
        expanded::BigRockAt(at.data());
        return at;
    }

    float Green(const CCharacter &character) {
        float ambient[4] = {};
        expanded::BigRockLight(character, ambient);
        return ambient[1];
    }

    float Whp() { return floor_.status().chara_weapons[chara_][0].durability_f; }

    CCharacter &Target() { return floor_.unit().chara[0][0]; }

    CSHOT_EFFECT &Shock() { return *expanded::ShotEffectAt(expanded::ShotEffectIndex(expanded::BigRockShockwave())); }

    StepsUnpaused                   unpaused_;
    TestFloor                       floor_;
    Wielding                        wielding_;
    int                             chara_;
    CCharacter                      weapon_;
    CCharacter                     *saved_weapon_ = nullptr;
    std::unique_ptr<CCollisionData> data_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_data_ = nullptr;
    CDungeonMap                    *saved_map_ = nullptr;
    float                           saved_pos_[3] = {};
    float                           saved_rot_[4] = {};
    s32                             saved_lock_ = 0;
    s32                             saved_lock_no_ = 0;
};

class BigRockWithTheHalberd : public BigRock {
protected:
    BigRockWithTheHalberd() : BigRock(ITEM_WEAPON_HALBERT) {}
};

class BigRockBySuperSteve : public BigRock {
protected:
    BigRockBySuperSteve() : BigRock(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_TERRA_SWORD, CHARA_XIAO) {}
};

class BigRockBySuperSteveWithTheHalberd : public BigRock {
protected:
    BigRockBySuperSteveWithTheHalberd() : BigRock(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HALBERT, CHARA_XIAO) {}
};

constexpr float kRockRest = expanded::kRockRadius - expanded::kRockSink; // its centre at rest over a floor at 0
constexpr float kRockStart = expanded::kRockRadius + expanded::kDropHeight;

// The frame the fall ends from a start: the first n with g n (n + 1) / 2 past the drop.
int FallFrames(float drop) {
    int   n = 0;
    float fallen = 0.0f;
    while (fallen < drop) {
        n++;
        fallen += n * expanded::kDropGravity;
    }
    return n;
}

} // namespace

TEST_F(BigRock, FiveSecondsOfGuardTurnTheSwordGreenAndPrimeIt) {
    Guard(true);
    Steps(1); // the first frame in the pose starts the clock
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kCharging);
    Steps(expanded::kRockChargeFrames / 2);
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 0.5f);
    EXPECT_FLOAT_EQ(Green(weapon_), expanded::kRockGreen[1] * 0.5f);
    EXPECT_FLOAT_EQ(Green(Target()), 0.0f); // the weapon in hand only
    Steps(expanded::kRockChargeFrames / 2 - 1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kCharging);
    Steps(1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kPrimed);
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 1.0f);
    EXPECT_TRUE(Loaded(expanded::kRockFile)); // loaded as it primes
    // Primed, it waits with the guard let go and no lock on.
    Guard(false);
    Steps(200);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kPrimed);
    EXPECT_FALSE(expanded::BigRockUp());
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 1.0f);
}

TEST_F(BigRock, LettingTheGuardGoFirstDropsTheCharge) {
    Guard(true);
    Steps(200);
    Guard(false);
    Steps(1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kIdle);
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 0.0f);
    Guard(true);
    Steps(expanded::kRockChargeFrames);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kCharging);
    Steps(1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kPrimed);
}

TEST_F(BigRock, LockedOnTheBoulderDropsOnTheMonster) {
    Prime();
    Steps(10);
    EXPECT_FALSE(expanded::BigRockUp());
    LockOn(0);
    Steps(1);
    ASSERT_TRUE(expanded::BigRockUp());
    EXPECT_TRUE(expanded::BigRockFalling());
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kIdle);
    // Its bottom 500 over the floor under the monster, at nothing of its size, its first frame of the fall taken; the
    // monster darkening.
    std::array<float, 3> at = At();
    EXPECT_FLOAT_EQ(at[0], 0.0f);
    EXPECT_FLOAT_EQ(at[1], kRockStart - expanded::kDropGravity);
    EXPECT_FLOAT_EQ(at[2], kTargetZ);
    EXPECT_FLOAT_EQ(expanded::BigRockScale(), expanded::kDropLeast);
    EXPECT_EQ(Target().fade_out, 1);
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 1.0f);
}

TEST_F(BigRock, ItFallsAfterTheMonsterGrowingAndDarkeningIt) {
    Drop();
    // From rest at kDropGravity a frame.
    Steps(19);
    float fallen = expanded::kDropGravity * 20.0f * 21.0f / 2.0f;
    EXPECT_NEAR(At()[1], kRockStart - fallen, 1e-3f);
    EXPECT_NEAR(expanded::BigRockScale(), fallen / expanded::kDropGrowDrop, 1e-4f);
    // Over the monster as it walks.
    Move(0, 15.0f, 70.0f);
    Steps(1);
    EXPECT_FLOAT_EQ(At()[0], 15.0f);
    EXPECT_FLOAT_EQ(At()[2], 70.0f);
    // Its size full past 150, the monster's light lowered with the fall to 0.55 at the floor.
    Steps(60);
    EXPECT_FLOAT_EQ(expanded::BigRockScale(), 1.0f);
    float drop = kRockStart - At()[1];
    EXPECT_NEAR(Target().fade[3], 1.0f - 0.45f * drop / (kRockStart - kRockRest), 1e-4f);
    EXPECT_TRUE(expanded::GroundShadowShown());
    EXPECT_TRUE(expanded::BigRockFalling());
}

// Its round shadow on the floor under the monster, growing from nothing to the boulder's radius over the first 300 of
// the fall, and staying at its radius while it rests.
TEST_F(BigRock, ItsShadowGrowsOnTheFloorUnderItAsItFalls) {
    Drop();
    Steps(29);
    ASSERT_TRUE(expanded::GroundShadowShown());
    float fallen = kRockStart - At()[1];
    EXPECT_NEAR(expanded::GroundShadowRadius(), expanded::kRockRadius * fallen / expanded::kShadowGrowDrop, 1e-3f);
    EXPECT_LT(expanded::GroundShadowRadius(), expanded::kRockRadius / 2.0f);
    Move(0, 15.0f, 70.0f);
    Steps(1);
    float at[3];
    expanded::GroundShadowAt(at);
    EXPECT_FLOAT_EQ(at[0], 15.0f);
    EXPECT_FLOAT_EQ(at[1], expanded::kGroundShadowLift); // the floor under it, not the boulder
    EXPECT_FLOAT_EQ(at[2], 70.0f);
    // Full once it has fallen 300.
    while (kRockStart - At()[1] < expanded::kShadowGrowDrop) {
        Steps(1);
    }
    EXPECT_FLOAT_EQ(expanded::GroundShadowRadius(), expanded::kRockRadius);
    FallToTheEnd();
    Steps(10);
    EXPECT_TRUE(expanded::GroundShadowShown());
    EXPECT_FLOAT_EQ(expanded::GroundShadowRadius(), expanded::kRockRadius);
}

TEST_F(BigRock, ItLandsInAboutTwoAndAHalfSecondsAndBlasts) {
    Stand(1, 20.0f, kTargetZ + 20.0f); // its body 23.3 from the spot: in reach
    Stand(2, 0.0f, kTargetZ + 40.0f);  // 35: out of reach
    Drop();
    int frames = FallToTheEnd();
    EXPECT_EQ(frames, FallFrames(kRockStart - kRockRest));
    EXPECT_GT(frames, 140);
    EXPECT_LT(frames, 150);
    EXPECT_FLOAT_EQ(At()[1], kRockRest);
    EXPECT_FLOAT_EQ(expanded::BigRockScale(), 1.0f);
    EXPECT_EQ(Target().fade_out, 0);
    // Four times the attack on each in reach, thrown away from it, through any guard, wearing nothing.
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 2u);
    for (COLLISION_HIT *hit : hits) {
        EXPECT_EQ(hit->damage, 400);
        EXPECT_EQ(hit->owner, CHARA_UNGAGA);
        EXPECT_EQ(hit->flags, 0); // no element
        EXPECT_TRUE(expanded::BigRockHit(*hit));
        EXPECT_FALSE(expanded::HitWearsWeapon(*hit));
        EXPECT_FALSE(expanded::MonsterGuardBlocks(floor_.unit(), 0, 0, *hit));
        EXPECT_EQ(hit->weapon_flags & 0xFFFF, 0); // none of the weapon's abilities
        EXPECT_EQ(hit->knockback_mode, 2);
        EXPECT_FLOAT_EQ(hit->knockback_speed, expanded::kRockKick.speed);
        EXPECT_FLOAT_EQ(hit->knockback_decay, expanded::kRockKick.decay);
    }
    EXPECT_FLOAT_EQ(hits[1]->pos[0], 20.0f);
    EXPECT_FLOAT_EQ(hits[1]->knockback_origin[0], 0.0f);
    EXPECT_FLOAT_EQ(hits[1]->knockback_origin[2], kTargetZ);
    // The monster right under it is thrown toward Ungaga: from 1 past it, on its far side from him.
    EXPECT_FLOAT_EQ(hits[0]->knockback_origin[2], kTargetZ + expanded::kRockKickLead);
    // The shockwave on the floor there, the bill, the column.
    CSHOT_EFFECT &shock = Shock();
    ASSERT_NE(shock.active[0], 0);
    EXPECT_FLOAT_EQ(shock.chara[0].scale[0], expanded::kShockScale);
    EXPECT_FLOAT_EQ(shock.chara[0].motion_speed, expanded::kShockRate);
    float shock_at[4];
    shock.chara[0].GetPosition(shock_at);
    EXPECT_FLOAT_EQ(shock_at[1], 0.0f);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - expanded::kRockWhp);
    EXPECT_TRUE(expanded::SolidColumnArmed());
    // The green fades from here.
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 1.0f);
    Steps(expanded::kRockGreenFadeFrames / 2);
    EXPECT_NEAR(expanded::BigRockGreen(), 0.5f, 1e-5f);
    Steps(expanded::kRockGreenFadeFrames / 2);
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 0.0f);
}

// DC_TEST_DATA, an extracted data directory: the crash of Master Utan's boulder goes off the frame it lands.
TEST_F(BigRock, ItLandsWithACrash) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    expanded::test::DataRootScope root(data);
    ASSERT_TRUE(expanded::LoadOwnSound(expanded::kRockCrashSound));
    audio::Mixer &mixer = audio::DefaultMixer();
    Drop();
    int before = mixer.ActiveVoices();
    while (expanded::BigRockFalling()) {
        EXPECT_EQ(mixer.ActiveVoices(), before);
        Steps(1);
    }
    EXPECT_EQ(mixer.ActiveVoices(), before + 1);
}

TEST_F(BigRock, AHitItsMonsterDidNotTakeIsPlantedAgain) {
    Drop();
    FallToTheEnd();
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    // Not taken (invincible that frame): planted again, up to four plants in all.
    for (int plant = 2; plant <= expanded::kDropHitTries; plant++) {
        ClearHits();
        Steps(1);
        EXPECT_EQ(Planted().size(), 1u) << plant;
    }
    ClearHits();
    Steps(1);
    EXPECT_TRUE(Planted().empty());
}

TEST_F(BigRock, AHitTakenIsNotPlantedAgain) {
    Drop();
    FallToTheEnd();
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    expanded::MonsterHitTaken(floor_.unit(), 0, *hits[0]);
    ClearHits();
    Steps(1);
    EXPECT_TRUE(Planted().empty());
}

TEST_F(BigRock, ItRestsTwentySecondsPinningWhatIsUnderItThenFades) {
    Drop();
    FallToTheEnd();
    expanded::MonsterHitTaken(floor_.unit(), 0, *Planted()[0]);
    ClearHits();
    Stand(1, 50.0f, kTargetZ); // not under it
    Steps(expanded::kRockPinFrames - 1);
    EXPECT_TRUE(Planted().empty());
    Steps(1);
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0]->damage, 100);
    EXPECT_EQ(hits[0]->knockback_mode, 0); // no throw
    EXPECT_TRUE(expanded::BigRockHit(*hits[0]));
    EXPECT_FALSE(expanded::MonsterGuardBlocks(floor_.unit(), 0, 0, *hits[0]));
    ClearHits();
    Steps(1);
    EXPECT_TRUE(Planted().empty()); // not watched
    // Solid and shadowed to its last second, then fading.
    Steps(expanded::kDropRestFrames - expanded::kRockPinFrames - 2);
    EXPECT_EQ(expanded::BigRockRestAge(), expanded::kDropRestFrames - 1);
    EXPECT_FLOAT_EQ(expanded::BigRockAlpha(), 1.0f);
    EXPECT_TRUE(expanded::GroundShadowShown());
    Steps(1 + expanded::kDropFadeFrames / 2);
    EXPECT_NEAR(expanded::BigRockAlpha(), 0.5f, 1e-5f);
    EXPECT_FALSE(expanded::GroundShadowShown());
    EXPECT_TRUE(expanded::SolidColumnArmed());
    Steps(expanded::kDropFadeFrames / 2 - 1);
    EXPECT_TRUE(expanded::BigRockUp());
    Steps(1);
    EXPECT_FALSE(expanded::BigRockUp());
    EXPECT_FALSE(expanded::SolidColumnArmed());
}

TEST_F(BigRock, NoChargeWhileItIsUp) {
    Drop();
    FallToTheEnd();
    Guard(true);
    Steps(expanded::kRockChargeFrames + 10);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kIdle);
    // Gone, the guard still held charges from then.
    while (expanded::BigRockUp()) {
        Steps(1);
    }
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kIdle);
    Steps(1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kCharging);
    Steps(expanded::kRockChargeFrames - 1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kCharging);
    Steps(1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kPrimed);
}

TEST_F(BigRock, AMonsterKilledAsItFallsLeavesItToFallWhereItIs) {
    Drop();
    Steps(50);
    floor_.unit().monster[0].hp = 0;
    Move(0, 40.0f, 0.0f);
    Steps(1);
    EXPECT_FLOAT_EQ(At()[2], kTargetZ);
    FallToTheEnd();
    EXPECT_FLOAT_EQ(At()[0], 0.0f);
    EXPECT_FLOAT_EQ(At()[1], kRockRest);
    EXPECT_FLOAT_EQ(At()[2], kTargetZ);
    EXPECT_TRUE(expanded::SolidColumnArmed());
    EXPECT_FLOAT_EQ(Whp(), 100.0f - expanded::kRockWhp);
}

TEST_F(BigRock, ItGoesWithTheWeaponOrTheWielder) {
    Drop();
    FallToTheEnd();
    ASSERT_TRUE(expanded::SolidColumnArmed());
    Wielding halberd(ITEM_WEAPON_HALBERT);
    Steps(1);
    EXPECT_FALSE(expanded::BigRockUp());
    EXPECT_FALSE(expanded::SolidColumnArmed());
    EXPECT_FALSE(expanded::GroundShadowShown());
    EXPECT_FALSE(expanded::ShotEffectAt(expanded::ShotEffectIndex(expanded::BigRockShockwave()))->active[0] != 0);
}

TEST_F(BigRock, AnotherMemberTakesItDown) {
    Prime();
    floor_.status().cur_chara = CHARA_GORO;
    Steps(1);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kIdle);
    EXPECT_FLOAT_EQ(expanded::BigRockGreen(), 0.0f);
}

TEST_F(BigRockWithTheHalberd, NothingCharges) {
    Guard(true);
    Steps(expanded::kRockChargeFrames + 10);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kIdle);
    LockOn(0);
    Steps(1);
    EXPECT_FALSE(expanded::BigRockUp());
}

TEST_F(BigRockBySuperSteve, TheSlingshotGoesGreenAndANutDrops) {
    Prime();
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kPrimed);
    EXPECT_FLOAT_EQ(Green(weapon_), expanded::kRockGreen[1]);
    EXPECT_TRUE(Loaded(expanded::kNutFile));
    LockOn(0);
    Steps(1);
    ASSERT_TRUE(expanded::BigRockUp());
    EXPECT_FLOAT_EQ(At()[1], expanded::kNutRadius + expanded::kDropHeight - expanded::kDropGravity);
    EXPECT_FLOAT_EQ(expanded::BigRockScale(), expanded::kDropLeast * expanded::kNutScale);
    // Its shadow on the floor under the monster as it falls, growing to the nut's radius over the first 300.
    Steps(30);
    EXPECT_TRUE(expanded::BigRockFalling());
    ASSERT_TRUE(expanded::GroundShadowShown());
    float fallen = expanded::kNutRadius + expanded::kDropHeight - At()[1];
    EXPECT_NEAR(expanded::GroundShadowRadius(), expanded::kNutRadius * fallen / expanded::kShadowGrowDrop, 1e-4f);
    float at[3];
    expanded::GroundShadowAt(at);
    EXPECT_FLOAT_EQ(at[1], expanded::kGroundShadowLift);
    EXPECT_FLOAT_EQ(at[2], kTargetZ);
    while (expanded::BigRockFalling()) {
        Steps(1);
    }
    // It follows the nut as it hops off, at its radius.
    Steps(5);
    expanded::GroundShadowAt(at);
    EXPECT_FLOAT_EQ(at[0], At()[0]);
    EXPECT_FLOAT_EQ(at[2], At()[2]);
    EXPECT_FLOAT_EQ(expanded::GroundShadowRadius(), expanded::kNutRadius);
}

TEST_F(BigRockBySuperSteve, TheNutBonksTheHeadAndConfusesIt) {
    Drop();
    float head = expanded::MonsterHeight(floor_.unit(), 0) + expanded::kNutRadius;
    FallToTheEnd();
    EXPECT_TRUE(expanded::BigRockUp());
    // The frame it reached the head: the bonk, half the attack, shoved from Xiao as a swing.
    auto hits = Planted();
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0]->damage, 50);
    EXPECT_EQ(hits[0]->owner, CHARA_XIAO);
    EXPECT_TRUE(expanded::BigRockHit(*hits[0]));
    EXPECT_FALSE(expanded::MonsterGuardBlocks(floor_.unit(), 0, 0, *hits[0]));
    EXPECT_EQ(hits[0]->knockback_mode, 2);
    EXPECT_FLOAT_EQ(hits[0]->knockback_speed, expanded::kNutKick.speed);
    EXPECT_FLOAT_EQ(hits[0]->knockback_origin[2], 0.0f);
    EXPECT_TRUE(expanded::confuse::Confused(floor_.unit(), 0));
    EXPECT_EQ(Target().fade_out, 0);
    expanded::MonsterHitTaken(floor_.unit(), 0, *hits[0]);
    ClearHits();
    // It hops off from the head, square to Xiao's line.
    EXPECT_NEAR(At()[1], head, 1e-4f);
    EXPECT_FLOAT_EQ(expanded::BigRockScale(), expanded::kNutScale);
    EXPECT_FALSE(expanded::SolidColumnArmed());
    Steps(10);
    std::array<float, 3> at = At();
    EXPECT_GT(at[1], head);
    EXPECT_NEAR(std::fabs(at[0]), expanded::kNutHopSide * 11.0f, 1e-3f);
    EXPECT_FLOAT_EQ(at[2], kTargetZ);
    // Down on the floor where it comes out, at rest and solid, harmless.
    int frames = 0;
    while (expanded::BigRockRestAge() < 0 && frames < 300) {
        Steps(1);
        frames++;
    }
    EXPECT_FLOAT_EQ(At()[1], expanded::kNutRadius - expanded::kNutSink);
    EXPECT_TRUE(expanded::SolidColumnArmed());
    EXPECT_TRUE(Planted().empty());
    Steps(expanded::kRockPinFrames * 3);
    EXPECT_TRUE(Planted().empty());
    EXPECT_FLOAT_EQ(Whp(), 100.0f); // nothing billed
    // Confused while it stays, its senses back as it goes.
    Steps(expanded::kDropRestFrames + expanded::kDropFadeFrames - expanded::kRockPinFrames * 3 - 1);
    EXPECT_TRUE(expanded::BigRockUp());
    EXPECT_TRUE(expanded::confuse::Confused(floor_.unit(), 0));
    Steps(1);
    EXPECT_FALSE(expanded::BigRockUp());
    EXPECT_FALSE(expanded::confuse::Confused(floor_.unit(), 0));
}

// DC_TEST_DATA, an extracted data directory: the nut's whistle goes off as it comes within the whistle's length of the
// head, so the knock of the bonk comes as the whistle ends.
TEST_F(BigRockBySuperSteve, ItWhistlesDownAndKnocks) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    expanded::test::DataRootScope root(data);
    ASSERT_TRUE(expanded::LoadOwnSound(expanded::kNutFallSound));
    ASSERT_TRUE(expanded::LoadOwnSound(expanded::kNutBonkSound));
    audio::Mixer &mixer = audio::DefaultMixer();
    Drop();
    int before = mixer.ActiveVoices();
    int frames = 1, whistled = -1;
    while (expanded::BigRockFalling()) {
        Steps(1);
        frames++;
        if (whistled < 0 && mixer.ActiveVoices() == before + 1) {
            whistled = frames;
        }
    }
    ASSERT_GT(whistled, 1); // not as it drops: the fall is longer than the whistle
    EXPECT_GE(frames - whistled, expanded::kNutFallSoundFrames - 1);
    EXPECT_LE(frames - whistled, expanded::kNutFallSoundFrames);
    EXPECT_EQ(Planted().size(), 1u);
    EXPECT_EQ(mixer.ActiveVoices(), before + 2);
}

TEST_F(BigRockBySuperSteve, TheNutFollowsTheHeadDown) {
    floor_.unit().chara[0][0].SetPosition(0.0f, 30.0f, kTargetZ); // the monster up on a ledge
    Drop();
    float head = 30.0f + expanded::MonsterHeight(floor_.unit(), 0) + expanded::kNutRadius;
    FallToTheEnd();
    EXPECT_EQ(Planted().size(), 1u);
    EXPECT_NEAR(At()[1], head, 1e-4f);
}

TEST_F(BigRockBySuperSteve, ANutWithNoHeadUnderItJustRests) {
    Drop();
    Steps(30);
    floor_.unit().monster[0].hp = 0;
    FallToTheEnd();
    EXPECT_TRUE(Planted().empty());
    EXPECT_FALSE(expanded::confuse::Confused(floor_.unit(), 0));
    EXPECT_EQ(expanded::BigRockRestAge(), 0);
    EXPECT_FLOAT_EQ(At()[1], expanded::kNutRadius - expanded::kNutSink);
    EXPECT_TRUE(expanded::SolidColumnArmed());
}

TEST_F(BigRockBySuperSteveWithTheHalberd, NothingCharges) {
    Guard(true);
    Steps(expanded::kRockChargeFrames + 10);
    EXPECT_EQ(expanded::BigRockState(), BigRockPhase::kIdle);
}

// A weapon's confusion keeps the later end; ending one monster's leaves the others'.
TEST_F(BigRock, ConfusionByAWeaponsEffect) {
    Stand(1, 30.0f, 0.0f);
    expanded::ConfuseMonster(floor_.unit(), 0, 100);
    expanded::ConfuseMonster(floor_.unit(), 0, 10);
    expanded::ConfuseMonster(floor_.unit(), 1, 100);
    for (int step = 0; step < 50; step++) {
        expanded::ConfuseTick();
    }
    EXPECT_TRUE(expanded::confuse::Confused(floor_.unit(), 0));
    expanded::ConfuseEnd(0);
    EXPECT_FALSE(expanded::confuse::Confused(floor_.unit(), 0));
    EXPECT_TRUE(expanded::confuse::Confused(floor_.unit(), 1));
    for (int step = 0; step < 50; step++) {
        expanded::ConfuseTick();
    }
    EXPECT_FALSE(expanded::confuse::Confused(floor_.unit(), 1));
}
