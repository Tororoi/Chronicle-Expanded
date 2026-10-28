#include <gtest/gtest.h>
#include <libpad.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/flame_gun.hpp"
#include "expanded/weapons/xiao/moves_while_firing.hpp"
#include "framevu1.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "motionmodel.hpp"
#include "platform/input.hpp"
#include "shot_firebar.hpp"
#include "test_support.hpp"
#include "texture.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace {

using expanded::kXiaoDraw;
using expanded::kXiaoHold;

constexpr int   kParticles = 24;
constexpr int   kDrawn = 18; // alive after the step: Set's opacity 180 - 8i, less the step's 4, above 32
constexpr int   kAttack = 40;
constexpr int   kHitInterval = 30;
constexpr float kMuzzle[3] = {0.0f, 8.0f, 3.0f};

struct alignas(64) PadDmaBuffer {
    std::array<unsigned char, 1024> bytes;
};

std::array<std::unique_ptr<PadDmaBuffer>, 2> pad_dma = {std::make_unique<PadDmaBuffer>(), std::make_unique<PadDmaBuffer>()};

// The shot button down or up, as the game reads it.
void ShotButton(bool down) {
    InputPadState state;
    state.connected = true;
    state.buttons = down ? PAD_CROSS : 0;
    InputSetOverride(0, &state);
    for (int i = 0; i < 4; i++) {
        GamePad.UpDate();
    }
}

// Xiao out with Super Steve (attack kAttack, endurance 0, fire its best element; 100 WHP left on her equipped record)
// carrying sphere's SynthSphere, at the origin facing +z, her slingshot's muzzle at kMuzzle, X held; the flame's picture
// in as a texture entry of the test's own; the flame's hits planted in a collision table of its own.
class FlameGun : public ::testing::Test {
protected:
    explicit FlameGun(int sphere = ITEM_WEAPON_SKUNK, int weapon_no = ITEM_WEAPON_SUPER_STEVE) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        ASSERT_TRUE(scePadInit(0) == 1);
        ASSERT_TRUE(scePadPortOpen(0, 0, pad_dma[0]->bytes.data()) == 1);
        ASSERT_TRUE(scePadPortOpen(1, 0, pad_dma[1]->bytes.data()) == 1);
        saved_ok_ = PadInput_OK;
        PadInput_OK = PAD_CROSS;
        BtActStatus = {};
        CUserStatus &status = floor_.status();
        status.cur_chara = CHARA_XIAO;
        WEAPON_HAVE &weapon = wielding_.weapon();
        weapon.attack = kAttack;
        weapon.endurance = 0;
        weapon.best_elem = WEAPON_ELEMENT_FIRE;
        weapon.flags = 0x20;
        status.equipped_weapon_slot[CHARA_XIAO] = 0;
        status.chara_weapons[CHARA_XIAO][0] = weapon;
        status.chara_weapons[CHARA_XIAO][0].durability = 100;
        status.chara_weapons[CHARA_XIAO][0].durability_f = 100.0f;
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_lock_ = lockOnTargetFlag;
        lockOnTargetFlag = 0;
        saved_weapon_ = NowWeapon;
        weapon_.Initialize();
        for (CFrameVu1 &frame : weapon_frames_) {
            frame.Initialize();
        }
        std::strncpy(weapon_frames_[1].name, "eff30", sizeof(weapon_frames_[1].name) - 1);
        for (int i = 0; i < 3; i++) {
            weapon_frames_[1].local[3][i] = kMuzzle[i];
        }
        weapon_frames_[1].SetParent(&weapon_frames_[0]);
        weapon_.frame = &weapon_frames_[0];
        NowWeapon = &weapon_;
        saved_hits_ = NowColData;
        NowColData = hits_.get();
        OzumondFire.Rset();
        EnterPicture();
        ShotButton(true);
        Act(kXiaoHold);
    }

    void TearDown() override {
        ShotButton(false);
        expanded::FlameGunShot(0); // put out
        InputSetOverride(0, nullptr);
        OzumondFire.Rset();
        if (picture_ >= 0) {
            TexManager.textures[picture_].Initialize();
        }
        TexManager.texture_max = saved_max_;
        NowColData = saved_hits_;
        NowWeapon = saved_weapon_;
        lockOnTargetFlag = saved_lock_;
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
        PadInput_OK = saved_ok_;
        BtActStatus = {};
    }

    // The flame's picture, as the game has it in with Osmond out.
    void EnterPicture() {
        saved_max_ = TexManager.texture_max;
        for (int i = 1; i < 196; i++) {
            if (TexManager.textures[i].name[0] == 0) {
                picture_ = i;
                break;
            }
        }
        ASSERT_GE(picture_, 0);
        std::strcpy(TexManager.textures[picture_].name, expanded::kFlamePicture);
        TexManager.textures[picture_].block = 0x46;
        TexManager.texture_max = std::max(TexManager.texture_max, picture_);
    }

    // Her slingshot action at action (BtActStatus.action_no).
    void Act(int action) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = action;
    }

    // The frames of her shot, as the game steps them: her action (the hook), then the flame's step.
    void Frames(int frames) {
        for (int i = 0; i < frames; i++) {
            expanded::DungeonFrameBegins();
            expanded::XiaoShotPlaying(CharaMain, 0);
            OzumondFire.Step();
        }
    }

    // How many of the flame's particles are alive after the frame's step.
    int Burning() const {
        int burning = 0;
        for (int i = 0; i < kParticles; i++) {
            burning += OzumondFire.state[i] == 0;
        }
        return burning;
    }

    float Whp() { return floor_.status().chara_weapons[CHARA_XIAO][0].durability_f; }

    // The flame hits planted, each checked against what Xiao's flame plants: her hit, her weapon's attack, element,
    // flags and anti-monster values, at radius.
    int HerHits(float radius) const {
        int count = 0;
        for (int i = 0; i < 96; i++) {
            if (hits_->active[i] == 0) {
                continue;
            }
            const COLLISION_HIT &hit = hits_->hit[i];
            EXPECT_EQ(hit.owner, CHARA_XIAO) << i;
            EXPECT_EQ(hit.attack_no, 6) << i;
            EXPECT_EQ(hit.damage, kAttack) << i;
            EXPECT_FLOAT_EQ(hit.radius, radius) << i;
            EXPECT_EQ(hit.flags, GetWeaponElementAttr(WEAPON_ELEMENT_FIRE)) << i;
            EXPECT_EQ(hit.weapon_flags, 0x20) << i;
            EXPECT_EQ(hit.vs_monster, NowWeaponHave->vs_monster) << i;
            count++;
        }
        return count;
    }

    // How far particle lies from the muzzle along +z, and off it.
    float Along(int particle) const { return OzumondFire.position[particle][2] - kMuzzle[2]; }

    float Off(int particle) const {
        return std::hypot(OzumondFire.position[particle][0] - kMuzzle[0], OzumondFire.position[particle][1] - kMuzzle[1]);
    }

    expanded::test::TestFloor       floor_;
    expanded::test::Wielding        wielding_;
    std::unique_ptr<CCollisionData> hits_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits_ = nullptr;
    CCharacter                      weapon_;
    CFrameVu1                       weapon_frames_[2];
    CCharacter                     *saved_weapon_ = nullptr;
    float                           saved_rot_[4] = {};
    s32                             saved_lock_ = 0;
    s32                             saved_ok_ = 0;
    int                             saved_max_ = 0;
    int                             picture_ = -1;
};

class BlessingGunFlame : public FlameGun {
protected:
    BlessingGunFlame() : FlameGun(ITEM_WEAPON_BLESSING_GUN) {}
};

class JackalSphere : public FlameGun {
protected:
    JackalSphere() : FlameGun(ITEM_WEAPON_JACKAL) {}
};

class NoSphere : public FlameGun {
protected:
    NoSphere() : FlameGun(0) {}
};

} // namespace

// The Blessing Gun's and the Skunk's spheres on Super Steve, no other.
TEST(FlameGunSphere, TheFlameGunsSpheresOnSuperSteve) {
    for (int sphere : {ITEM_WEAPON_BLESSING_GUN, ITEM_WEAPON_SKUNK}) {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_TRUE(expanded::FlameGunSphere()) << sphere;
    }
    for (int sphere : {ITEM_WEAPON_JACKAL, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_HEXA_BLASTER, ITEM_WEAPON_STAR_BREAKER, ITEM_WEAPON_SUPERNOVA,
                       ITEM_WEAPON_SNAIL, ITEM_WEAPON_DIVINEBEASTTITLE}) {
        expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_FALSE(expanded::FlameGunSphere()) << sphere;
    }
    {
        expanded::test::Wielding bare(ITEM_WEAPON_SUPER_STEVE);
        EXPECT_FALSE(expanded::FlameGunSphere());
    }
    expanded::test::Wielding skunk(ITEM_WEAPON_SKUNK);
    EXPECT_FALSE(expanded::FlameGunSphere());
}

TEST(FlameGunSphere, NotWithTheWeaponAbilitiesOff) {
    expanded::test::Wielding    steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SKUNK);
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_FALSE(expanded::FlameGunSphere());
}

// kFlameIgnition frames into the hold the flame lights from her muzzle along her aim, the Skunk's 4 apart.
TEST_F(FlameGun, LightsEightFramesIntoTheHold) {
    Frames(expanded::kFlameIgnition - 1);
    EXPECT_FALSE(expanded::FlameGunLit());
    EXPECT_EQ(Burning(), 0);
    Frames(1);
    EXPECT_TRUE(expanded::FlameGunLit());
    EXPECT_EQ(Burning(), kDrawn);
    Frames(60);
    for (int particle = 0; particle < kParticles; particle++) {
        EXPECT_NEAR(Along(particle), 4.0f * static_cast<float>(particle), 1e-3f) << particle;
        EXPECT_NEAR(Off(particle), 0.0f, 1e-3f) << particle;
    }
}

TEST_F(BlessingGunFlame, TheBlessingGunsSphereThrowsTheGamesFlame) {
    Frames(60);
    for (int particle = 0; particle < kParticles; particle++) {
        EXPECT_NEAR(Along(particle), 2.0f * static_cast<float>(particle), 1e-3f) << particle;
    }
}

// Every puff carries her weapon's attack and element; its hits, every kHitInterval frames, are hers, at the Skunk's
// radius 8 or the game's 4.
TEST_F(FlameGun, ItsHitsAreHers) {
    Frames(expanded::kFlameIgnition);
    for (int particle = 0; particle < kParticles; particle++) {
        EXPECT_EQ(OzumondFire.damage[particle], kAttack) << particle;
        EXPECT_EQ(OzumondFire.particle_element[particle], WEAPON_ELEMENT_FIRE) << particle;
    }
    Frames(kHitInterval);
    EXPECT_EQ(HerHits(8.0f), kParticles);
    EXPECT_EQ(expanded::FlameOwner(5), CHARA_XIAO);
}

TEST_F(BlessingGunFlame, ItsHitsAreHersAtTheGamesRadius) {
    Frames(expanded::kFlameIgnition + kHitInterval);
    EXPECT_EQ(HerHits(4.0f), kParticles);
}

// A poisoned weapon's flame hits twice as hard, as Osmond's does.
TEST_F(FlameGun, APoisonedWeaponBurnsTwiceAsHard) {
    floor_.status().ailments[CHARA_XIAO] = AILMENT_STAMINA;
    Frames(expanded::kFlameIgnition);
    EXPECT_EQ(OzumondFire.damage[0], 2 * kAttack);
}

// Osmond's flame is his own.
TEST_F(FlameGun, OsmondsFlameIsHis) {
    Frames(expanded::kFlameIgnition);
    floor_.status().cur_chara = CHARA_OSMOND;
    EXPECT_EQ(expanded::FlameOwner(5), 5);
}

// Locked on, the flame goes at her target.
TEST_F(FlameGun, LockedOnItGoesAtHerTarget) {
    lockOnTargetFlag = 1;
    BtActStatus.target_position[0] = 30.0f;
    BtActStatus.target_position[1] = kMuzzle[1];
    BtActStatus.target_position[2] = kMuzzle[2] + 40.0f;
    Frames(60);
    float last[3];
    for (int i = 0; i < 3; i++) {
        last[i] = OzumondFire.position[kParticles - 1][i] - kMuzzle[i];
    }
    EXPECT_NEAR(std::atan2(last[0], last[2]), std::atan2(30.0f, 40.0f), 1e-3f);
    EXPECT_NEAR(last[1], 0.0f, 1e-3f);
}

// It wears her slingshot as his flame wears his gun: kIgnitionWear as it lights, kBurnWear each kBurnWearInterval
// frames it burns (BattleSubWeaponDmg's 1.5 - endurance/100 a unit, endurance 0 here).
TEST_F(FlameGun, WearsHerSlingshotAsHisFlameWearsHisGun) {
    Frames(expanded::kFlameIgnition - 1);
    EXPECT_FLOAT_EQ(Whp(), 100.0f);
    Frames(1);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - 1.5f * expanded::kIgnitionWear);
    Frames(expanded::kBurnWearInterval - 2);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - 1.5f * expanded::kIgnitionWear);
    Frames(1);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - 1.5f * (expanded::kIgnitionWear + expanded::kBurnWear));
    Frames(expanded::kBurnWearInterval);
    EXPECT_FLOAT_EQ(Whp(), 100.0f - 1.5f * (expanded::kIgnitionWear + 2.0f * expanded::kBurnWear));
}

// Letting go puts the flame out (the game's hold then fires her pellet).
TEST_F(FlameGun, LettingGoPutsItOut) {
    Frames(40);
    ASSERT_EQ(Burning(), kDrawn);
    ShotButton(false);
    Frames(1);
    EXPECT_FALSE(expanded::FlameGunLit());
    EXPECT_EQ(Burning(), 0);
    EXPECT_EQ(BtActStatus.action_no, kXiaoHold); // the game's own branch moves on to the shot
}

// A shot let go in the draw, or in the hold before the flame lights, is a plain pellet: no flame, no wear.
TEST_F(FlameGun, AQuickShotHasNoFlame) {
    Act(kXiaoDraw);
    Frames(12);
    ShotButton(false);
    Frames(1);
    EXPECT_EQ(Burning(), 0);

    ShotButton(true);
    Act(kXiaoHold);
    Frames(expanded::kFlameIgnition - 1);
    ShotButton(false);
    Frames(1);
    EXPECT_FALSE(expanded::FlameGunLit());
    EXPECT_EQ(Burning(), 0);
    EXPECT_FLOAT_EQ(Whp(), 100.0f);
}

// The hold counts afresh each shot.
TEST_F(FlameGun, EachHoldLightsAfresh) {
    Frames(20);
    Act(expanded::kXiaoShot);
    Frames(5);
    EXPECT_EQ(expanded::FlameGunHeld(), 0);
    Act(kXiaoHold);
    Frames(expanded::kFlameIgnition - 1);
    EXPECT_FALSE(expanded::FlameGunLit());
}

// The flame is her slingshot action: she moves under it.
TEST_F(FlameGun, SheMovesWhileItBurns) {
    Frames(20);
    ASSERT_TRUE(expanded::FlameGunLit());
    EXPECT_TRUE(expanded::FiringLeavesMove());
}

TEST_F(JackalSphere, OtherOsmondSpheresMoveWithNoFlame) {
    Frames(60);
    EXPECT_FALSE(expanded::FlameGunLit());
    EXPECT_EQ(Burning(), 0);
    EXPECT_FLOAT_EQ(Whp(), 100.0f);
    EXPECT_TRUE(expanded::FiringLeavesMove());
}

TEST_F(NoSphere, NoSphereNoFlame) {
    Frames(60);
    EXPECT_EQ(Burning(), 0);
    EXPECT_FLOAT_EQ(Whp(), 100.0f);
}

TEST_F(FlameGun, NoFlameWithTheWeaponAbilitiesOff) {
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Frames(60);
    EXPECT_EQ(Burning(), 0);
    EXPECT_FLOAT_EQ(Whp(), 100.0f);
}
