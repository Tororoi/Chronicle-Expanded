#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dataalloc.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/weapons/xiao/divine_beast_title.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::SpiritBeastPhase;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// The cat's pack, stood in for: a character with no model.
bool FakeLoad(CCharacter &chara, CDataAlloc2<1> &, const char *, const char *) {
    chara.Initialize();
    return true;
}

// Xiao with weapon_no (carrying sphere when given), attack 50, on a floor with ground 0, a pellet ready in slot 0.
class SpiritBeast : public ::testing::Test {
protected:
    explicit SpiritBeast(int weapon_no = ITEM_WEAPON_DIVINEBEASTTITLE, int sphere = 0) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_XIAO;
        wielding_.weapon().attack = 50;
        saved_data_ = NowColData;
        NowColData = data_.get();
        saved_pos_[0] = CharaMain.pos[0];
        saved_pos_[1] = CharaMain.pos[1];
        saved_pos_[2] = CharaMain.pos[2];
        CharaMain.pos[0] = CharaMain.pos[1] = CharaMain.pos[2] = 0.0f;
        expanded::ShotEffectsClear();
        expanded::SetCharacterLoader(FakeLoad);
        expanded::SpiritBeastReset();
        *shots_ = CSHOT{};
        shots_->used[0] = 1;
        shots_->pos[0][1] = 10.0f;
        shots_->vector[0][2] = 4.0f;
        shots_->damage[0] = 50;
    }

    void TearDown() override {
        expanded::SpiritBeastReset();
        expanded::SetCharacterLoader(nullptr);
        expanded::ShotEffectsClear();
        NowColData = saved_data_;
        CharaMain.pos[0] = saved_pos_[0];
        CharaMain.pos[1] = saved_pos_[1];
        CharaMain.pos[2] = saved_pos_[2];
        BtActStatus = {};
    }

    // monster standing at (x, 0, z) with a body sphere of radius 5 at height 6.
    void Stand(int monster, float x, float z) {
        MONSTOR &live = floor_.unit().monster[monster];
        live.state = 2;
        live.revealed = 1;
        live.hp = 100;
        CCharacter &body = floor_.unit().chara[monster][0];
        body.pos[0] = x;
        body.pos[2] = z;
        body.body_height = 12.0f;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.position[0][0] = x;
        effect.position[0][1] = 6.0f;
        effect.position[0][2] = z;
        effect.radius[0] = 5.0f;
        effect.timer[0] = 1;
    }

    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::SpiritBeastStep();
        }
    }

    // The slingshot held drawn for frames, then let go into slot 0.
    void HoldAndFire(int frames) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 12;
        Steps(frames);
        BtActStatus.action_on = 0;
        Steps(1);
        expanded::PelletFired(*shots_, 0);
    }

    // The hit the cat planted, or nullptr.
    COLLISION_HIT *Planted() {
        for (int i = 0; i < 96; i++) {
            if (NowColData->active[i] != 0 && expanded::SpiritBeastHit(NowColData->hit[i])) {
                return &NowColData->hit[i];
            }
        }
        return nullptr;
    }

    TestFloor                       floor_;
    Wielding                        wielding_;
    std::unique_ptr<CSHOT>          shots_ = std::make_unique<CSHOT>();
    std::unique_ptr<CCollisionData> data_ = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_data_ = nullptr;
    float                           saved_pos_[3] = {};
};

class SpiritBeastWithAnotherWeapon : public SpiritBeast {
protected:
    SpiritBeastWithAnotherWeapon() : SpiritBeast(ITEM_WEAPON_ANGEL_SHOOTER) {}
};

class SpiritBeastBySuperSteve : public SpiritBeast {
protected:
    SpiritBeastBySuperSteve() : SpiritBeast(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DIVINEBEASTTITLE) {}
};

} // namespace

TEST_F(SpiritBeast, AChargedShotLetsTheCatGoInThePelletsPlace) {
    HoldAndFire(expanded::kSpiritBeastFrames);

    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kFlying);
    EXPECT_EQ(shots_->used[0], 0);
    // It falls off the shot's line to the floor.
    Steps(60);
    EXPECT_NE(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kFlying);
    EXPECT_FLOAT_EQ(expanded::SpiritBeastPosition()[1], 0.0f);
    EXPECT_GT(expanded::SpiritBeastPosition()[2], 4.0f);
}

TEST_F(SpiritBeast, AShortHoldIsAPellet) {
    HoldAndFire(expanded::kSpiritBeastFrames - 1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
    EXPECT_EQ(shots_->used[0], 1);
}

TEST_F(SpiritBeast, WithNothingToChaseItSitsThenFades) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    Steps(120);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kSitting);
    Steps(expanded::kSpiritBeastLifeFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
}

TEST_F(SpiritBeast, ItRunsAtAMonsterPouncesAndStrikesItOnce) {
    Stand(0, 60.0f, 80.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);

    bool ran = false;
    for (int frame = 0; frame < 600 && Planted() == nullptr; frame++) {
        expanded::SpiritBeastStep();
        ran = ran || expanded::SpiritBeastPhaseNow() == SpiritBeastPhase::kRunning;
    }
    EXPECT_TRUE(ran);
    COLLISION_HIT *hit = Planted();
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->damage, static_cast<int>(expanded::kSpiritBeastDamage * 50.0f));
    EXPECT_EQ(hit->owner, CHARA_XIAO);
    EXPECT_EQ(hit->knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit->knockback_speed, expanded::kSpiritBeastKickSpeed);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kFading);
    // Through a guard, and staggering as a blow does.
    EXPECT_FALSE(expanded::MonsterGuardBlocks(floor_.unit(), 0, *hit));
    EXPECT_TRUE(expanded::PlayerHitStaggers(*hit));

    // Gone after its fade, with no second hit.
    NowColData->active[hit - NowColData->hit] = 0;
    Steps(expanded::kSpiritBeastFadeFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
    EXPECT_EQ(Planted(), nullptr);
}

TEST_F(SpiritBeastWithAnotherWeapon, NeverLetsACatGo) {
    HoldAndFire(expanded::kSpiritBeastFrames * 2);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
    EXPECT_EQ(shots_->used[0], 1);
}

TEST_F(SpiritBeastBySuperSteve, LetsTheSameCatGoWithTheSphere) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kFlying);
}

TEST(SpiritBeastHits, AnotherHitNeitherPassesAGuardNorStaggers) {
    COLLISION_HIT plain = {};
    plain.weapon_flags = 1;
    EXPECT_FALSE(expanded::SpiritBeastHit(plain));
    EXPECT_FALSE(expanded::PlayerHitStaggers(plain));
}
