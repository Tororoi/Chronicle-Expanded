#include <gtest/gtest.h>
#include <libpad.h>

#include <array>
#include <memory>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/motion_layer.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/xiao/rapid_fire.hpp"
#include "gamepad.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "platform/input.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::kXiaoDraw;
using expanded::kXiaoHold;
using expanded::kXiaoShot;

constexpr int kDamage = 40;

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

// Xiao out with Super Steve carrying sphere's SynthSphere, X held, her slingshot action on.
class RapidFire : public ::testing::Test {
protected:
    explicit RapidFire(int sphere = ITEM_WEAPON_MACHINE_GUN) : wielding_(ITEM_WEAPON_SUPER_STEVE, sphere) {}

    void SetUp() override {
        ASSERT_TRUE(scePadInit(0) == 1);
        ASSERT_TRUE(scePadPortOpen(0, 0, pad_dma[0]->bytes.data()) == 1);
        ASSERT_TRUE(scePadPortOpen(1, 0, pad_dma[1]->bytes.data()) == 1);
        saved_ok_ = PadInput_OK;
        PadInput_OK = PAD_CROSS;
        saved_status_ = BtActStatus;
        BtActStatus = {};
        BtActStatus.action_gauge = expanded::kFullGauge;
        floor_.status().cur_chara = CHARA_XIAO;
        saved_weapon_ = NowWeapon;
        NowWeapon = &weapon_;
        driveNoInterpolate = 0;
        ShotButton(true);
        Act(kXiaoHold);
    }

    void TearDown() override {
        BtActStatus = {};
        expanded::RapidFireFrame(); // the stream over
        ShotButton(false);
        InputSetOverride(0, nullptr);
        NowWeapon = saved_weapon_;
        PadInput_OK = saved_ok_;
        BtActStatus = saved_status_;
        driveNoInterpolate = 0;
    }

    static void Act(int action) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = action;
        BtActStatus.motion_no = action;
    }

    bool Shot() { return expanded::RapidFireShot(chara_); }

    // Drawn with X held: held kRapidHold frames, then into the shot.
    void Start() {
        for (int i = 0; i < expanded::kRapidHold; i++) {
            EXPECT_FALSE(Shot());
            EXPECT_EQ(BtActStatus.action_no, kXiaoHold) << i;
        }
        Shot();
    }

    // A pellet fired: its damage as the stream leaves it.
    int Pellet() {
        shots_->damage[0] = kDamage;
        expanded::RapidFirePellet(*shots_, 0);
        return shots_->damage[0];
    }

    expanded::test::TestFloor floor_;
    expanded::test::Wielding  wielding_;
    CCharacter                chara_;
    CCharacter                weapon_;
    CCharacter               *saved_weapon_ = nullptr;
    std::unique_ptr<CSHOT>    shots_ = std::make_unique<CSHOT>();
    BT_ACT_STATUS             saved_status_ = {};
    s32                       saved_ok_ = 0;
};

TEST_F(RapidFire, DrawnWithXHeldLetsGoAfterItsHold) {
    for (int i = 0; i < expanded::kRapidHold; i++) {
        EXPECT_FALSE(Shot());
        EXPECT_EQ(BtActStatus.action_no, kXiaoHold) << i;
    }
    EXPECT_FALSE(Shot());
    EXPECT_TRUE(expanded::RapidFiring());
    EXPECT_EQ(BtActStatus.action_no, kXiaoShot);
    EXPECT_EQ(BtActStatus.motion_no, kXiaoShot);
    EXPECT_EQ(driveNoInterpolate, 1); // cut to, not blended into
}

TEST_F(RapidFire, PelletsAtAShotsDamageAndWear) {
    Start();
    EXPECT_EQ(Pellet(), kDamage);
    EXPECT_EQ(expanded::RapidFireShots(), 1);
    EXPECT_FLOAT_EQ(expanded::XiaoShotWear(1.0f), 1.0f);
}

TEST_F(RapidFire, AfterThePelletAndItsFollowThroughPullsBack) {
    Start();
    // Still in the shot until its pellet is gone, and for its follow-through.
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoShot);

    Pellet();
    driveNoInterpolate = 0;
    for (int i = 0; i < expanded::kRapidFollowThrough; i++) {
        EXPECT_FALSE(Shot());
        EXPECT_EQ(BtActStatus.action_no, kXiaoShot) << i;
    }
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoDraw);
    EXPECT_EQ(BtActStatus.motion_no, kXiaoDraw);
    EXPECT_EQ(BtActStatus.action_step, 0); // the draw goes on into the hold, not a shot of the game's
    EXPECT_EQ(driveNoInterpolate, 0);      // faded into, not cut to

    EXPECT_FALSE(Shot());
    EXPECT_FLOAT_EQ(chara_.motion_speed, expanded::kRapidDrawSpeed);
    EXPECT_FLOAT_EQ(weapon_.motion_speed, expanded::kRapidDrawSpeed);

    // Drawn again, the next pellet, and its own follow-through before the next pull-back.
    Act(kXiaoHold);
    Start();
    EXPECT_EQ(BtActStatus.action_no, kXiaoShot);
    Pellet();
    EXPECT_EQ(expanded::RapidFireShots(), 2);
    for (int i = 0; i < expanded::kRapidFollowThrough; i++) {
        EXPECT_FALSE(Shot());
        EXPECT_EQ(BtActStatus.action_no, kXiaoShot) << i;
    }
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoDraw);
}

TEST_F(RapidFire, LetGoInThePullBackEndsTheStream) {
    Start();
    Pellet();
    for (int i = 0; i <= expanded::kRapidFollowThrough; i++) {
        Shot();
    }
    ASSERT_EQ(BtActStatus.action_no, kXiaoDraw);
    ShotButton(false);
    EXPECT_TRUE(Shot());
    EXPECT_FALSE(expanded::RapidFiring());
}

TEST_F(RapidFire, LetGoInTheShotFinishesIt) {
    Start();
    ShotButton(false);
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoShot);
    EXPECT_EQ(Pellet(), kDamage);
    // Its pellet gone, the shot plays out.
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoShot);
}

TEST_F(RapidFire, ATapFiresAnOrdinaryShot) {
    Act(kXiaoDraw);
    ShotButton(false);
    EXPECT_FALSE(Shot());
    EXPECT_FALSE(expanded::RapidFiring());
    EXPECT_NE(chara_.motion_speed, expanded::kRapidDrawSpeed);
    Act(kXiaoShot);
    EXPECT_FALSE(Shot());
    EXPECT_EQ(Pellet(), kDamage);
    EXPECT_FLOAT_EQ(expanded::XiaoShotWear(1.0f), 1.0f);
}

TEST_F(RapidFire, TheFirstDrawIsHerOwn) {
    Act(kXiaoDraw);
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoDraw);
    EXPECT_NE(chara_.motion_speed, expanded::kRapidDrawSpeed);
}

TEST_F(RapidFire, EndsWithTheAction) {
    Start();
    BtActStatus.action_on = 0;
    expanded::RapidFireFrame();
    EXPECT_FALSE(expanded::RapidFiring());
    EXPECT_EQ(Pellet(), kDamage);
}

TEST_F(RapidFire, OnlyForXiao) {
    floor_.status().cur_chara = CHARA_OSMOND;
    EXPECT_FALSE(Shot());
    EXPECT_FALSE(expanded::RapidFiring());
    EXPECT_EQ(BtActStatus.action_no, kXiaoHold);
}

// With her motions: KEY 11 the draw (240 to 251), KEY 13 the shot (251 to 255), one set as the game's.
class RapidFireMotions : public RapidFire {
protected:
    void SetUp() override {
        RapidFire::SetUp();
        info_[kXiaoDraw] = {240, 251, 0.7f, 0};
        info_[kXiaoShot] = {251, 255, 0.7f, 0};
        chara_.motion[0] = &chara_.motion_type;
        chara_.motion_start[0] = 0;
        chara_.motion_end[0] = 64;
        chara_.motion_type.motion_info = info_.data();
        State() = {};
        State().blend_step = 0.1f;
    }

    MOTION_STATE &State() { return chara_.motion_type.state; }

    // Into the pull-back, as the stream gets there: the shot, its pellet, its follow-through.
    void ToPullBack() {
        Start();
        Pellet();
        for (int i = 0; i <= expanded::kRapidFollowThrough; i++) {
            Shot();
        }
        ASSERT_EQ(BtActStatus.action_no, kXiaoDraw);
    }

    // SetMotionEX's hook as Step calls it, the game's blend step put back after as Step does.
    void Posing() {
        expanded::MotionPosing(&chara_.motion_type, &State());
        posed_step_ = State().blend_step;
        State().blend_step = 0.1f;
    }

    std::array<MOTION_INFO, 64> info_ = {};
    float                       posed_step_ = 0.0f;
};

TEST_F(RapidFireMotions, FadesIntoThePullBackPartWayIn) {
    ToPullBack();
    // The step the key changes: the fade arrives kRapidPullFrom frames into the draw, in 1 / kRapidBlendStep steps.
    State().time = 253.1f;
    State().playing_no = kXiaoShot;
    State().motion_no = kXiaoDraw;
    State().next_frame = 240;
    Posing();
    EXPECT_EQ(State().next_frame, 240u + expanded::kRapidPullFrom);
    EXPECT_FLOAT_EQ(posed_step_, expanded::kRapidBlendStep);

    // The fade's next step keeps its pace; once it is done, the game's is back.
    State().playing_no = kXiaoDraw;
    State().blending = 1;
    Posing();
    EXPECT_FLOAT_EQ(posed_step_, expanded::kRapidBlendStep);
    State().blending = 0;
    Posing();
    EXPECT_FLOAT_EQ(posed_step_, 0.1f);
}

TEST_F(RapidFireMotions, WaitsWhileItFades) {
    ToPullBack();
    State().time = 253.1f;
    State().motion_no = State().playing_no = kXiaoDraw;
    State().blending = 1;
    chara_.motion_speed = -1.0f;
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoDraw);
    EXPECT_FLOAT_EQ(chara_.motion_speed, -1.0f);
}

TEST_F(RapidFireMotions, PullsToFullDrawExactlyAndLetsGo) {
    ToPullBack();
    State().motion_no = State().playing_no = kXiaoDraw;
    State().time = 240.0f + expanded::kRapidPullFrom;
    EXPECT_FALSE(Shot());
    EXPECT_FLOAT_EQ(chara_.motion_speed, expanded::kRapidDrawSpeed);
    EXPECT_FLOAT_EQ(weapon_.motion_speed, expanded::kRapidDrawSpeed);

    // Short of full draw by less than a step, the step lands on it: the draw never runs past its end.
    State().time = 249.5f;
    EXPECT_FALSE(Shot());
    EXPECT_FLOAT_EQ(chara_.motion_speed, 0.5f);

    // At full draw: held there kRapidHold frames, then let go.
    State().time = 240.0f + expanded::kRapidPullTo;
    driveNoInterpolate = 0;
    for (int i = 0; i < expanded::kRapidHold; i++) {
        EXPECT_FALSE(Shot());
        EXPECT_EQ(BtActStatus.action_no, kXiaoDraw) << i;
        EXPECT_LT(chara_.motion_speed, 0.01f) << i;
    }
    EXPECT_FALSE(Shot());
    EXPECT_EQ(BtActStatus.action_no, kXiaoShot);
    EXPECT_EQ(driveNoInterpolate, 1);
}

TEST_F(RapidFire, EachPelletSpendsTheGaugeAsAShotDoes) {
    Start();
    Pellet();
    EXPECT_FLOAT_EQ(BtActStatus.action_gauge, 0.0f);
    // The stream does not wait on it: the next pull-back goes on with the gauge empty.
    for (int i = 0; i <= expanded::kRapidFollowThrough; i++) {
        Shot();
    }
    EXPECT_EQ(BtActStatus.action_no, kXiaoDraw);
    EXPECT_TRUE(expanded::RapidFiring());
}

class RapidFireWithoutTheSphere : public RapidFire {
protected:
    RapidFireWithoutTheSphere() : RapidFire(ITEM_WEAPON_HEXA_BLASTER) {}
};

TEST_F(RapidFireWithoutTheSphere, HoldStaysHeld) {
    EXPECT_FALSE(Shot());
    EXPECT_FALSE(expanded::RapidFiring());
    EXPECT_EQ(BtActStatus.action_no, kXiaoHold);
    EXPECT_EQ(Pellet(), kDamage);
}

} // namespace

// The machine guns' spheres fire so, the launchers' and the flamethrowers' do not.
TEST(RapidFireSpheres, EveryMachineGun) {
    expanded::test::TestFloor floor;
    floor.status().cur_chara = CHARA_XIAO;
    for (int sphere : {ITEM_WEAPON_MACHINE_GUN, ITEM_WEAPON_JACKAL, ITEM_WEAPON_SWALLOW, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_STAR_BREAKER}) {
        expanded::test::Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_TRUE(expanded::RapidFireSphere()) << sphere;
    }
    for (int sphere : {ITEM_WEAPON_HEXA_BLASTER, ITEM_WEAPON_SUPERNOVA, ITEM_WEAPON_SNAIL, ITEM_WEAPON_BLESSING_GUN, ITEM_WEAPON_SKUNK,
                       ITEM_WEAPON_DIVINEBEASTTITLE}) {
        expanded::test::Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_FALSE(expanded::RapidFireSphere()) << sphere;
    }
}
