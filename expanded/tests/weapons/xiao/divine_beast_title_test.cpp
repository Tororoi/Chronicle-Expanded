#include <gtest/gtest.h>

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dataalloc.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/weapons/xiao/divine_beast_title.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

extern s32 lockOnTargetNo;

namespace {

using expanded::SpiritBeastPhase;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// The cat's clips, as its cfg keys them: first and last frame, rate.
struct Clip {
    int   key;
    float start, end, rate;
};

constexpr Clip kClips[] = {
    {64, 10.0f,  20.0f,  0.1f },
    {65, 95.0f,  105.0f, 0.4f },
    {68, 205.0f, 214.0f, 0.5f },
    {69, 215.0f, 227.0f, 0.36f},
    {70, 60.0f,  80.0f,  1.0f },
    {71, 285.0f, 294.0f, 0.75f},
    {72, 30.0f,  40.0f,  0.1f },
};

// The cat, stood in for: a character with no model.
bool FakeLoad(CCharacter &chara, CDataAlloc2<1> &, const char *, const char *) {
    chara.Initialize();
    return true;
}

// Xiao with weapon_no (carrying sphere when given), attack 50, on a floor at height 0; a pellet of 50 ready in slot 0,
// 10 up, flying along +z at 4 a frame.
class SpiritBeast : public ::testing::Test {
protected:
    explicit SpiritBeast(int weapon_no = ITEM_WEAPON_DIVINEBEASTTITLE, int sphere = 0) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_XIAO;
        wielding_.weapon().attack = 50;
        saved_data_ = NowColData;
        NowColData = data_.get();
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        saved_lock_ = lockOnTargetFlag;
        lockOnTargetFlag = 0;
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(FakeLoad);
        expanded::SpiritBeastReset();
        playing_ = -1;
        fading_ = 0;
        saved_event_ = BtEventMode;
        BtEventMode = 0;
        saved_target_ = lockOnTargetNo;
        lockOnTargetNo = -1;
        *shots_ = CSHOT{};
        shots_->used[0] = 1;
        shots_->pos[0][1] = 10.0f;
        shots_->vector[0][2] = 4.0f;
        shots_->damage[0] = 50;
    }

    void TearDown() override {
        expanded::SpiritBeastReset();
        expanded::SetFloorCharacterLoader(nullptr);
        expanded::FloorCharactersClear();
        NowColData = saved_data_;
        for (int i = 0; i < 3; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        lockOnTargetFlag = saved_lock_;
        lockOnTargetNo = saved_target_;
        BtEventMode = saved_event_;
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
        body.scale[0] = 1.0f;
        MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[monster];
        effect.position[0][0] = x;
        effect.position[0][1] = 6.0f;
        effect.position[0][2] = z;
        effect.radius[0] = 5.0f;
        effect.timer[0] = 1;
    }

    // The engine's playback of the cat's motion, stood in for: a key change fades over 1 / blend steps with the clip held,
    // a cut starts the new clip at once; a clip held on its last frame stops short of its end, any other loops.
    void Play() {
        CCharacter *cat = expanded::SpiritBeastModel();
        if (cat == nullptr) {
            return;
        }
        const Clip *clip = nullptr;
        for (const Clip &c : kClips) {
            clip = c.key == cat->motion_no ? &c : clip;
        }
        if (clip == nullptr) {
            return;
        }
        float &time = cat->motion_type.state.time;
        if (cat->motion_no != playing_ || (cat->motion_flags & 4) != 0) {
            float blend = cat->motion_type.state.blend_step > 0.0f ? cat->motion_type.state.blend_step : 0.1f;
            fading_ = (cat->motion_flags & 4) != 0 ? 0 : static_cast<int>(std::lround(1.0f / blend));
            playing_ = cat->motion_no;
            cat->motion_flags &= ~4;
            if (fading_ == 0) {
                time = clip->start;
            }
            return;
        }
        if (fading_ > 0) {
            if (--fading_ == 0) {
                time = clip->start;
            }
            return;
        }
        float rate = cat->motion_speed > 0.0f ? cat->motion_speed : clip->rate;
        if ((cat->motion_flags & 2) != 0) {
            time = time + rate < clip->end ? time + rate : time;
        } else {
            time += rate;
            time = time >= clip->end ? clip->start + (time - clip->end) : time;
        }
    }

    float Frame() { return expanded::SpiritBeastModel()->motion_type.state.time; }

    void Steps(int frames) {
        for (int frame = 0; frame < frames; frame++) {
            expanded::SpiritBeastStep();
            Play();
        }
    }

    // Steps until the cat is in phase (or frames run out): the frames it took.
    int StepsUntil(SpiritBeastPhase phase, int frames = 2000) {
        for (int frame = 1; frame <= frames; frame++) {
            Steps(1);
            if (expanded::SpiritBeastPhaseNow() == phase) {
                return frame;
            }
        }
        return -1;
    }

    // The slingshot held drawn for frames, then let go into slot 0.
    void HoldAndFire(int frames) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 12;
        Steps(frames);
        BtActStatus.action_on = 0;
        Steps(1);
        expanded::PelletFired(*shots_, 0);
        Steps(1); // the pellets step, and the cat binds
    }

    // The monster's CheckDmg on the cat's planted hit: taken or not, it is spent.
    void Consume(COLLISION_HIT *hit, bool taken) {
        if (taken) {
            expanded::MonsterHitTaken(floor_.unit(), 0, *hit);
        }
        NowColData->active[hit - NowColData->hit] = 0;
    }

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
    s32                             saved_lock_ = 0;
    s32                             saved_target_ = -1;
    s32                             saved_event_ = 0;
    int                             playing_ = -1;
    int                             fading_ = 0;
};

class SpiritBeastWithAnotherWeapon : public SpiritBeast {
protected:
    SpiritBeastWithAnotherWeapon() : SpiritBeast(ITEM_WEAPON_MATADOR) {}
};

class SpiritBeastBySuperSteve : public SpiritBeast {
protected:
    SpiritBeastBySuperSteve() : SpiritBeast(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DIVINEBEASTTITLE) {}
};

class SpiritBeastByAngelShooter : public SpiritBeast {
protected:
    SpiritBeastByAngelShooter() : SpiritBeast(ITEM_WEAPON_ANGEL_SHOOTER) {}
};

class SpiritBeastBySuperSteveWithTheAngelShooter : public SpiritBeast {
protected:
    SpiritBeastBySuperSteveWithTheAngelShooter() : SpiritBeast(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_ANGEL_SHOOTER) {}
};

// Where a cat lands when its target steps aside just past the leap's apex.
float LandingAfterASidestep(SpiritBeast &test, std::function<void(int)> steps, std::function<int(SpiritBeastPhase, int)> until,
                            CMonstorUnit &unit) {
    (void) test;
    until(SpiritBeastPhase::kLeaping, 2000);
    float height = expanded::SpiritBeastPosition()[1];
    while (true) {
        steps(1);
        if (expanded::SpiritBeastPosition()[1] < height) {
            break;
        }
        height = expanded::SpiritBeastPosition()[1];
    }
    unit.chara[0][0].pos[0] += 20.0f;
    unit.effect[0].position[0][0] += 20.0f;
    until(SpiritBeastPhase::kLanding, 200);
    while (expanded::SpiritBeastPhaseNow() == SpiritBeastPhase::kLanding) {
        steps(1);
    }
    return expanded::SpiritBeastPosition()[0];
}

} // namespace

TEST_F(SpiritBeast, ItGrowsOnTheHeldPelletThenBreaksAwayOnItsVelocity) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kRiding);
    EXPECT_EQ(shots_->halted[0], 1);

    // The pellet shrinks away as the cat grows on it.
    Steps(3);
    EXPECT_FLOAT_EQ(shots_->size[0], 0.5f);
    Steps(expanded::kSpiritBeastGrowFrames - 4);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kRiding);
    EXPECT_EQ(shots_->used[0], 1);
    Steps(1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kFalling);
    EXPECT_EQ(shots_->life[0], 1); // gone on its next step

    // Off the pellet's line at its speed: 4 a frame along z until the paws touch.
    float z = expanded::SpiritBeastPosition()[2];
    Steps(1);
    EXPECT_NEAR(expanded::SpiritBeastPosition()[2] - z, 4.0f, 1e-4f);
    EXPECT_GT(StepsUntil(SpiritBeastPhase::kSitting), 0);
    EXPECT_FLOAT_EQ(expanded::SpiritBeastPosition()[1], 0.0f);
}

TEST_F(SpiritBeast, AHalfSecondHoldIsTheCatAShorterOneAPellet) {
    HoldAndFire(expanded::kSpiritBeastFrames - 1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
    EXPECT_EQ(shots_->used[0], 1);
    EXPECT_EQ(expanded::kSpiritBeastFrames, 30);
}

TEST_F(SpiritBeast, WithNothingToChaseItSitsThenFadesAtTwentySeconds) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    int sat = StepsUntil(SpiritBeastPhase::kSitting, 200);
    ASSERT_GT(sat, 0);
    Steps(expanded::kSpiritBeastLifeFrames - sat - 1);
    EXPECT_FALSE(expanded::SpiritBeastFading());
    Steps(1);
    EXPECT_TRUE(expanded::SpiritBeastFading());
    Steps(expanded::kSpiritBeastFadeFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
}

TEST_F(SpiritBeast, ItWalksCrouchesWindsUpAndLeapsAsTheModTimesIt) {
    Stand(0, 0.0f, 160.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);

    ASSERT_GT(StepsUntil(SpiritBeastPhase::kWalking), 0);
    float z = expanded::SpiritBeastPosition()[2];
    Steps(1);
    EXPECT_NEAR(expanded::SpiritBeastPosition()[2] - z, expanded::kSpiritBeastWalk, 1e-4f);

    ASSERT_GT(StepsUntil(SpiritBeastPhase::kReady), 0);
    EXPECT_LE(160.0f - expanded::SpiritBeastPosition()[2], expanded::kSpiritBeastPounce);
    // The crouch, faded into, holds until its clip is a frame from its end; the float-up until its feet leave the floor.
    float frame = 0.0f;
    int   crouch = 0;
    for (; crouch < 200 && expanded::SpiritBeastPhaseNow() == SpiritBeastPhase::kReady; crouch++) {
        frame = Frame();
        Steps(1);
    }
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kWindUp);
    EXPECT_GE(frame, expanded::kSpiritBeastReadyEnd - 1.0f);
    EXPECT_EQ(crouch, 10 + 23 + 1); // the fade, the clip's 95 to 104.2 at 0.4, the frame that sees it
    int windup = 0;
    for (; windup < 200 && expanded::SpiritBeastPhaseNow() == SpiritBeastPhase::kWindUp; windup++) {
        frame = Frame();
        Steps(1);
    }
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kLeaping);
    EXPECT_GE(frame, expanded::kSpiritBeastFloatLaunch);
    EXPECT_EQ(windup, 10 + 11 + 1); // the fade, 285 to 293.25 at 0.75, the frame that sees it
}

TEST_F(SpiritBeast, ItsOneHitIsThePelletsDamageAndTheAttackThroughAGuard) {
    Stand(0, 0.0f, 160.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);
    for (int frame = 0; frame < 1000 && Planted() == nullptr; frame++) {
        expanded::SpiritBeastStep();
    }
    COLLISION_HIT *hit = Planted();
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->damage, 50 + 50);
    EXPECT_EQ(hit->owner, CHARA_XIAO);
    EXPECT_FLOAT_EQ(hit->radius, expanded::kSpiritBeastHitRadius);
    EXPECT_EQ(hit->knockback_mode, 2);
    EXPECT_FLOAT_EQ(hit->knockback_speed, expanded::kSpiritBeastKickSpeed);
    EXPECT_FLOAT_EQ(hit->knockback_decay, expanded::kSpiritBeastKickDecay);
    EXPECT_FALSE(expanded::MonsterGuardBlocks(floor_.unit(), 0, 0, *hit));
    EXPECT_TRUE(expanded::PlayerHitStaggers(*hit));
    EXPECT_EQ(hit->kind, 2);
    EXPECT_FALSE(expanded::SpiritBeastFading()); // not until the monster takes it

    // Taken, it fades as it goes on, with no second hit.
    Consume(hit, true);
    Steps(1);
    EXPECT_TRUE(expanded::SpiritBeastFading());
    Steps(expanded::kSpiritBeastFadeFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
    EXPECT_EQ(Planted(), nullptr);
}

TEST_F(SpiritBeast, ItHoldsItsCrouchWhileTheMonsterCannotBeHit) {
    Stand(0, 0.0f, 160.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kReady), 0);
    floor_.unit().monster[0].invincible_timer = 100;
    Steps(100);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kReady);
    floor_.unit().monster[0].invincible_timer = 0;
    EXPECT_GT(StepsUntil(SpiritBeastPhase::kWindUp, 5), 0);
}

TEST_F(SpiritBeast, NothingIsTouchedUntilItBreaksAway) {
    Stand(0, 0.0f, 0.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);
    Steps(expanded::kSpiritBeastGrowFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kFalling);
    EXPECT_EQ(Planted(), nullptr);
    Steps(1);
    EXPECT_NE(Planted(), nullptr);
}

TEST_F(SpiritBeastWithAnotherWeapon, NeverLetsACatGo) {
    HoldAndFire(expanded::kSpiritBeastFrames * 2);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
    EXPECT_EQ(shots_->used[0], 1);
    EXPECT_EQ(shots_->halted[0], 0);
}

TEST_F(SpiritBeastBySuperSteve, LetsTheSameCatGoWithTheSphere) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kRiding);
}

TEST(SpiritBeastHits, AnotherHitNeitherPassesAGuardNorStaggers) {
    COLLISION_HIT plain = {};
    plain.weapon_flags = 1;
    EXPECT_FALSE(expanded::SpiritBeastHit(plain));
    EXPECT_FALSE(expanded::PlayerHitStaggers(plain));
}

TEST_F(SpiritBeast, ItHeadsForTheLargestSphereXiaosHitsDamage) {
    Stand(0, 0.0f, 160.0f);
    MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[0];
    effect.position[0][0] = 40.0f; // a small sphere her hits damage, off to one side
    effect.parameter[0][CHARA_XIAO] = 100;
    effect.position[1][0] = -40.0f; // a larger one they do not
    effect.position[1][1] = 6.0f;
    effect.position[1][2] = 160.0f;
    effect.radius[1] = 8.0f;
    effect.timer[1] = 1;
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kWalking), 0);
    float x = expanded::SpiritBeastPosition()[0];
    Steps(30);
    EXPECT_GT(expanded::SpiritBeastPosition()[0], x);
}

TEST_F(SpiritBeast, ItLeapsThroughTheSpheresHeightAsTheMod) {
    // A sphere 6 up: a 32-frame arc aimed through it, 1.4675 a frame up at the launch (6 / 32 + 0.08 * 16).
    Stand(0, 0.0f, 160.0f);
    floor_.unit().effect[0].radius[0] = 0.5f;
    floor_.unit().effect[0].position[0][0] = 60.0f; // beside the walk, so the cat leaps rather than touching it on foot
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kLeaping), 0);
    float apex = 0.0f;
    for (int frame = 0; frame < 40 && expanded::SpiritBeastPhaseNow() == SpiritBeastPhase::kLeaping; frame++) {
        Steps(1);
        apex = std::max(apex, expanded::SpiritBeastPosition()[1]);
    }
    EXPECT_NEAR(apex, 14.2f, 0.01f); // it moves, then falls: 19 rising frames, 19 * 1.4675 - 0.08 * 171
}

TEST_F(SpiritBeast, ALargeMonsterGetsTheHigherArc) {
    auto leap = [&](float scale) {
        expanded::SpiritBeastReset();
        Stand(0, 0.0f, 160.0f);
        floor_.unit().chara[0][0].scale[0] = scale;
        floor_.unit().effect[0].timer[0] = 0; // no sphere to aim at or touch: it lands at the feet
        *shots_ = CSHOT{};
        shots_->used[0] = 1;
        shots_->pos[0][1] = 10.0f;
        shots_->vector[0][2] = 4.0f;
        shots_->damage[0] = 50;
        HoldAndFire(expanded::kSpiritBeastFrames);
        EXPECT_GT(StepsUntil(SpiritBeastPhase::kLeaping), 0);
        return StepsUntil(SpiritBeastPhase::kLanding, 200);
    };
    int low = leap(1.0f), high = leap(1.5f);
    EXPECT_GT(low, 0);
    EXPECT_EQ(high - low, expanded::kSpiritBeastLeapFramesTall - expanded::kSpiritBeastLeapFrames);
}

TEST_F(SpiritBeast, ItWaitsCrouchedByAShutMimicAndPouncesOnceItIsOut) {
    Stand(0, 0.0f, 160.0f);
    MONSTOR &mimic = floor_.unit().monster[0];
    mimic.kind = MONSTER_KIND_MIMIC_SMALL;
    mimic.revealed = 0; // in its chest
    mimic.state = 1;
    floor_.unit().effect[0].timer[0] = 0;
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kReady), 0);
    EXPECT_LE(160.0f - expanded::SpiritBeastPosition()[2], expanded::kSpiritBeastPounce);
    EXPECT_NEAR(expanded::SpiritBeastPosition()[0], 0.0f, 0.5f); // at the chest, not off after a sphere
    Steps(100);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kReady);

    // Opened, not yet in play: still waited for.
    mimic.revealed = 1;
    floor_.unit().effect[0].timer[0] = 1;
    floor_.unit().effect[0].position[0][0] = 300.0f; // a sphere left from another monster: not aimed at
    Steps(1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kReady);
    floor_.unit().effect[0].position[0][0] = 0.0f;
    // Out and in play, untouchable a while as it wakes.
    mimic.revealed = -1;
    mimic.state = 2;
    mimic.invincible_timer = 100;
    floor_.unit().effect[0].timer[0] = 1;
    Steps(50);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kReady);
    mimic.invincible_timer = 0;
    floor_.unit().effect[0].timer[0] = 1;
    EXPECT_GT(StepsUntil(SpiritBeastPhase::kWindUp, 5), 0);
}

TEST_F(SpiritBeast, AnotherCharacterTakesTheCatAwayAtOnce) {
    Stand(0, 0.0f, 160.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kWalking), 0);
    floor_.status().cur_chara = CHARA_TOAN;
    Steps(1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
}

TEST_F(SpiritBeast, TakenAwayOnThePelletItLetsThePelletGo) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kRiding);
    floor_.status().cur_chara = CHARA_TOAN;
    Steps(1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
    EXPECT_EQ(shots_->used[0], 1);
    EXPECT_EQ(shots_->halted[0], 0);
    EXPECT_FLOAT_EQ(shots_->size[0], 1.0f);
}

TEST_F(SpiritBeast, ATouchTheMonsterRefusesLeavesTheCatToTouchAgain) {
    Stand(0, 0.0f, 0.0f);
    floor_.unit().effect[0].radius[0] = 30.0f; // still in reach a few frames on
    HoldAndFire(expanded::kSpiritBeastFrames);
    COLLISION_HIT *hit = nullptr;
    for (int frame = 0; frame < 20 && (hit = Planted()) == nullptr; frame++) {
        Steps(1);
    }
    ASSERT_NE(hit, nullptr);
    Consume(hit, false); // its invincibility, or a sphere out of its hurt frames
    Steps(1);
    EXPECT_FALSE(expanded::SpiritBeastFading());
    Steps(1);
    EXPECT_NE(Planted(), nullptr);
}

TEST_F(SpiritBeast, AHitNotTakenInTimeIsWithdrawn) {
    Stand(0, 0.0f, 0.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);
    COLLISION_HIT *hit = nullptr;
    for (int frame = 0; frame < 20 && (hit = Planted()) == nullptr; frame++) {
        Steps(1);
    }
    ASSERT_NE(hit, nullptr);
    int index = static_cast<int>(hit - NowColData->hit);
    Steps(expanded::kSpiritBeastHitWait);
    EXPECT_EQ(NowColData->active[index], 0);
    EXPECT_FALSE(expanded::SpiritBeastFading());
}

TEST_F(SpiritBeast, TheMonsterSheWouldLockOnToIsItsTarget) {
    Stand(0, 0.0f, 100.0f);
    Stand(1, 120.0f, 120.0f);
    lockOnTargetNo = 1; // not locked on, but the one she faces
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kWalking), 0);
    float x = expanded::SpiritBeastPosition()[0];
    Steps(20);
    EXPECT_GT(expanded::SpiritBeastPosition()[0], x + 5.0f);
}

TEST_F(SpiritBeast, ALockOnMadeAsItCrouchesSendsItWalking) {
    Stand(0, 0.0f, 160.0f);
    Stand(1, 150.0f, 160.0f);
    floor_.unit().monster[0].invincible_timer = 1000; // held in its crouch
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kReady), 0);
    lockOnTargetNo = 1;
    EXPECT_GT(StepsUntil(SpiritBeastPhase::kWalking, expanded::kSpiritBeastRetarget + 1), 0);
}

TEST_F(SpiritBeast, AMonsterGoneTooFarByTheLaunchIsWalkedTo) {
    Stand(0, 0.0f, 160.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kWindUp), 0);
    floor_.unit().chara[0][0].pos[2] = 260.0f;
    floor_.unit().effect[0].position[0][2] = 260.0f;
    EXPECT_GT(StepsUntil(SpiritBeastPhase::kWalking, 40), 0);
}

TEST_F(SpiritBeast, AnEventTakesTheCatAway) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    BtEventMode = 1;
    Steps(1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
}

TEST_F(SpiritBeast, AWeaponWithoutElementsHitsWithNone) {
    Stand(0, 0.0f, 0.0f);
    wielding_.weapon().best_elem = 0;
    for (s8 &level : wielding_.weapon().elem) {
        level = 0;
    }
    HoldAndFire(expanded::kSpiritBeastFrames);
    COLLISION_HIT *hit = nullptr;
    for (int frame = 0; frame < 20 && (hit = Planted()) == nullptr; frame++) {
        Steps(1);
    }
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->flags, 0);
}

TEST_F(SpiritBeast, ASphereNamingTheCatsKickTakesItsOwnPercentage) {
    MONSTOR_EFFECT_STATE &effect = floor_.unit().effect[0];
    effect.parameter[0][CHARA_XIAO] = 0; // Minotaur Joe's face, to her pellets
    effect.body_parameter[0][0] = 100;
    effect.body_parameter[0][1] = 2;
    COLLISION_HIT cat = {}, pellet = {};
    cat.knockback_mode = 2;
    EXPECT_EQ(expanded::MonsterSpherePercent(floor_.unit(), 0, 0, CHARA_XIAO, cat, 0), 100);
    EXPECT_EQ(expanded::MonsterSpherePercent(floor_.unit(), 0, 0, CHARA_XIAO, pellet, 0), 0);
    EXPECT_EQ(expanded::MonsterSpherePercent(floor_.unit(), 0, 0, CHARA_TOAN, cat, 30), 30);
}

TEST_F(SpiritBeast, TheTitlesCatIsPlainAndItsOwnSize) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    EXPECT_FALSE(expanded::SpiritBeastWinged());
    EXPECT_FALSE(expanded::SpiritBeastCaped());
    EXPECT_FLOAT_EQ(expanded::SpiritBeastScale(), 1.0f);
}

TEST_F(SpiritBeastByAngelShooter, LetsAWingedCatGoALittleLarger) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kRiding);
    EXPECT_TRUE(expanded::SpiritBeastWinged());
    EXPECT_FALSE(expanded::SpiritBeastCaped());
    EXPECT_FLOAT_EQ(expanded::SpiritBeastScale(), expanded::kAngelShooterCatScale);
}

TEST_F(SpiritBeastByAngelShooter, ItWalksAsItsSizeAndCrouchesFromFarther) {
    Stand(0, 0.0f, 160.0f);
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kWalking), 0);
    float z = expanded::SpiritBeastPosition()[2];
    Steps(1);
    EXPECT_NEAR(expanded::SpiritBeastPosition()[2] - z, expanded::kSpiritBeastWalk * expanded::kAngelShooterCatScale, 1e-4f);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kReady), 0);
    float distance = 160.0f - expanded::SpiritBeastPosition()[2];
    EXPECT_LE(distance, expanded::kSpiritBeastPounceWinged);
    EXPECT_GT(distance, expanded::kSpiritBeastPounce);
}

TEST_F(SpiritBeastByAngelShooter, ByAShutMimicItCrouchesFromThePlainReach) {
    Stand(0, 0.0f, 160.0f);
    MONSTOR &mimic = floor_.unit().monster[0];
    mimic.kind = MONSTER_KIND_MIMIC_SMALL;
    mimic.revealed = 0; // in its chest
    mimic.state = 1;
    floor_.unit().effect[0].timer[0] = 0;
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_GT(StepsUntil(SpiritBeastPhase::kReady), 0);
    EXPECT_LE(160.0f - expanded::SpiritBeastPosition()[2], expanded::kSpiritBeastPounce);
}

TEST_F(SpiritBeastByAngelShooter, ItSteersPastTheApex) {
    Stand(0, 0.0f, 160.0f);
    floor_.unit().effect[0].timer[0] = 0; // nothing to touch: it lands
    HoldAndFire(expanded::kSpiritBeastFrames);
    float winged = LandingAfterASidestep(*this, [&](int n) { Steps(n); }, [&](SpiritBeastPhase p, int n) { return StepsUntil(p, n); }, floor_.unit());
    EXPECT_GT(winged, 8.0f);
}

TEST_F(SpiritBeast, ItsPounceIsSetAtTheApex) {
    Stand(0, 0.0f, 160.0f);
    floor_.unit().effect[0].timer[0] = 0; // nothing to touch: it lands
    HoldAndFire(expanded::kSpiritBeastFrames);
    float plain = LandingAfterASidestep(*this, [&](int n) { Steps(n); }, [&](SpiritBeastPhase p, int n) { return StepsUntil(p, n); }, floor_.unit());
    EXPECT_LT(plain, 2.0f);
}

TEST_F(SpiritBeastBySuperSteveWithTheAngelShooter, LetsACapedCatGo) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kRiding);
    EXPECT_TRUE(expanded::SpiritBeastCaped());
    EXPECT_FALSE(expanded::SpiritBeastWinged());
    EXPECT_FLOAT_EQ(expanded::SpiritBeastScale(), expanded::kAngelShooterCatScale);
}

TEST_F(SpiritBeastByAngelShooter, AChangeOfLookTakesTheCatAway) {
    HoldAndFire(expanded::kSpiritBeastFrames);
    ASSERT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kRiding);
    wielding_.weapon().item_no = ITEM_WEAPON_DIVINEBEASTTITLE;
    Steps(1);
    EXPECT_EQ(expanded::SpiritBeastPhaseNow(), SpiritBeastPhase::kNone);
}
