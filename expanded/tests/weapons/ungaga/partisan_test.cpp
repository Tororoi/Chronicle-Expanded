#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <memory>
#include <set>

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/toan/shamshir.hpp"
#include "expanded/weapons/ungaga/partisan.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

void UngagaKey_On();
void UngagaKey_Play();

using expanded::test::Wielding;

namespace {

// Ungaga's combo swings as his dungeon set has them: 670-678 at a step of 0.28, 678-694 and 695-714 at 0.32; every other
// motion steps 0.5.
void UngagaKeys(MOTION_INFO (&keys)[64]) {
    for (MOTION_INFO &key : keys) {
        key = {0, 10, 0.5f, 0};
    }
    keys[0x25] = {670, 678, 0.28f, 0};
    keys[0x26] = {678, 694, 0.32f, 0};
    keys[0x27] = {695, 714, 0.32f, 0};
}

// A character playing motion_no from Ungaga's table.
struct Playing {
    std::unique_ptr<CMonstorUnit> unit = std::make_unique<CMonstorUnit>();
    MOTION_INFO                   keys[64] = {};

    explicit Playing(int motion_no, float motion_speed = -1.0f) {
        UngagaKeys(keys);
        Chara().motion_type.motion_info = keys;
        Chara().motion_no = motion_no;
        Chara().motion_speed = motion_speed;
    }

    CCharacter &Chara() { return unit->chara[0][0]; }
};

void Frame(CFrameVu1 &frame, const char *name, CFrame *parent) {
    frame.Initialize();
    std::strncpy(frame.name, name, sizeof(frame.name) - 1);
    if (parent != nullptr) {
        frame.SetParent(parent);
    }
}

// What a combo did: the frames from the first swing to the last one handing back, the swings it reached, and the
// swings that planted hits.
struct ComboRun {
    int           frames = 0;
    std::set<int> swings;
    std::set<int> hit_swings;
};

// Ungaga out on a floor with weapon_no in hand and a full gauge, cross pressed every frame: each frame runs as the
// dungeon's does (UngagaKey_On, UngagaKey_Play, then motionDrive handing the motion over, speed set by `speed`, and the
// character stepped), until the combo hands back.
class Combo {
public:
    explicit Combo(int weapon_no) : wielding_(weapon_no) {
        floor_.status().cur_chara = CHARA_UNGAGA;
        NowColData = hits_.get();
        Frame(weapon_frames_[0], "w", nullptr);
        Frame(weapon_frames_[1], "dcol0", &weapon_frames_[0]);
        Frame(weapon_frames_[2], "dcol1", &weapon_frames_[0]);
        weapon_.frame = &weapon_frames_[0];
        NowWeapon = &weapon_;
        Frame(body_, "body", nullptr);
        UngagaKeys(keys_);
        CharaMain.frame = &body_;
        CharaMain.motion_type.motion_info = keys_;
        CharaMain.motion_type.proc_list = nullptr;
        CharaMain.motion_type.proc_list2 = nullptr;
        CharaMain.motion_type.state = {};
        CharaMain.motion_no = 0;
        CharaMain.motion_speed = -1.0f;
        CharaMain.motion_flags = 0;
        BtActStatus = {};
        BtActStatus.action_gauge = 100.0f;
        driveNoInterpolate = 0;
    }

    ~Combo() {
        BtActStatus = {};
        driveNoInterpolate = 0;
        CharaMain.frame = saved_frame_;
        CharaMain.motion_type = saved_motion_;
        CharaMain.motion_no = saved_motion_no_;
        CharaMain.motion_speed = saved_speed_;
        CharaMain.motion_flags = saved_flags_;
        NowWeapon = saved_weapon_;
        NowColData = saved_hits_;
    }

    Combo(const Combo &) = delete;
    Combo &operator=(const Combo &) = delete;

    ComboRun Run(const std::function<void(CCharacter &)> &speed) {
        ComboRun run;
        for (int frame = 0; frame < 600; frame++) {
            UngagaKey_On();
            if (BtActStatus.action_on == 1) {
                UngagaKey_Play();
            }
            if (frame > 0 && BtActStatus.action_on == 0) {
                break;
            }
            if (std::any_of(std::begin(hits_->active), std::end(hits_->active), [](s32 active) { return active != 0; })) {
                run.hit_swings.insert(BtActStatus.action_no);
            }
            std::fill(std::begin(hits_->active), std::end(hits_->active), 0);
            run.swings.insert(BtActStatus.action_no);
            run.frames++;
            Drive(speed);
        }
        return run;
    }

private:
    // motionDrive's part: the motion asked for handed over (restarted at its first frame after a chained swing), its
    // speed set, the character stepped.
    void Drive(const std::function<void(CCharacter &)> &speed) {
        int blend = 0;
        if (driveNoInterpolate != 0) {
            blend = 4;
            driveNoInterpolate = 0;
        }
        if (BtActStatus.main_motion != BtActStatus.motion_no) {
            CharaMain.motion_no = BtActStatus.motion_no;
            CharaMain.motion_flags = blend;
            CharaMain.motion_speed = -1.0f;
            BtActStatus.main_motion = BtActStatus.motion_no;
        }
        speed(CharaMain);
        CharaMain.Step();
    }

    expanded::test::TestFloor       floor_;
    Wielding                        wielding_;
    std::unique_ptr<CCollisionData> hits_ = std::make_unique<CCollisionData>();
    CFrameVu1                       weapon_frames_[3];
    CFrameVu1                       body_;
    CCharacter                      weapon_;
    MOTION_INFO                     keys_[64] = {};
    CCollisionData                 *saved_hits_ = NowColData;
    CCharacter                     *saved_weapon_ = NowWeapon;
    decltype(CharaMain.frame)       saved_frame_ = CharaMain.frame;
    tagMOTION_TYPE                  saved_motion_ = CharaMain.motion_type;
    int                             saved_motion_no_ = CharaMain.motion_no;
    float                           saved_speed_ = CharaMain.motion_speed;
    int                             saved_flags_ = CharaMain.motion_flags;
};

void QuickCombo(CCharacter &chara) {
    expanded::QuickComboMotion(chara, CHARA_UNGAGA);
}

} // namespace

TEST(Partisan, ComboSwingsPlayFaster) {
    Wielding partisan(ITEM_WEAPON_PARTISAN);
    EXPECT_TRUE(expanded::QuickComboWielded());
    const float stock[3] = {0.28f, 0.32f, 0.32f};
    for (int swing = 0x25; swing <= 0x27; swing++) {
        Playing playing(swing);
        expanded::QuickComboMotion(playing.Chara(), CHARA_UNGAGA);
        EXPECT_FLOAT_EQ(playing.Chara().motion_speed, stock[swing - 0x25] * expanded::kSwiftStrikesFactor);
    }
}

// Swing 2 hands over to swing 3 only within half a frame of its end: its raised step stays under that.
TEST(Partisan, SwingTwoStepsUnderItsHandOverWindow) {
    Wielding partisan(ITEM_WEAPON_PARTISAN);
    Playing  swing(0x26);
    expanded::QuickComboMotion(swing.Chara(), CHARA_UNGAGA);
    EXPECT_LT(swing.Chara().motion_speed, 0.5f);
}

TEST(Partisan, ChargeAndOtherMotionsPlayAsRetail) {
    Wielding partisan(ITEM_WEAPON_PARTISAN);
    // Standing, locked on, the charge's wind-up, hold and release, and the numbers either side of the swings.
    for (int motion : {0, 0x12, 13, 14, 15, 0x24, 0x28}) {
        Playing playing(motion);
        expanded::QuickComboMotion(playing.Chara(), CHARA_UNGAGA);
        EXPECT_FLOAT_EQ(playing.Chara().motion_speed, -1.0f) << motion;
    }

    // A speed the game set stands.
    Playing set(0x25, 0.2f);
    expanded::QuickComboMotion(set.Chara(), CHARA_UNGAGA);
    EXPECT_FLOAT_EQ(set.Chara().motion_speed, 0.2f);
}

TEST(Partisan, OnlyUngagasSwingsWithThePartisanInHand) {
    for (int weapon : {ITEM_WEAPON_FIGHTING_STICK, ITEM_WEAPON_HALBERT, ITEM_WEAPON_MIRAGE}) {
        Wielding other(weapon);
        EXPECT_FALSE(expanded::QuickComboWielded());
        Playing playing(0x25);
        expanded::QuickComboMotion(playing.Chara(), CHARA_UNGAGA);
        EXPECT_FLOAT_EQ(playing.Chara().motion_speed, -1.0f);
    }

    // Another character's motions of the same numbers.
    Wielding partisan(ITEM_WEAPON_PARTISAN);
    for (int chara : {CHARA_TOAN, CHARA_XIAO, CHARA_GORO, CHARA_RUBY, CHARA_OSMOND}) {
        Playing playing(0x26);
        expanded::QuickComboMotion(playing.Chara(), chara);
        EXPECT_FLOAT_EQ(playing.Chara().motion_speed, -1.0f) << chara;
    }
}

// Super Steve carrying a Partisan's SynthSphere: Swift Strikes' draw and shot, not the swings.
TEST(Partisan, SuperSteveSphereDrawsAndShootsFaster) {
    Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_PARTISAN);
    EXPECT_FALSE(expanded::QuickComboWielded());
    EXPECT_TRUE(expanded::SwiftStrikesSphere());

    Playing draw(11);
    draw.keys[11] = {240, 251, 0.7f, 0};
    expanded::SwiftStrikesMotion(draw.Chara(), CHARA_XIAO);
    EXPECT_FLOAT_EQ(draw.Chara().motion_speed, 0.7f * expanded::kSwiftDrawFactor);

    Playing shot(13);
    expanded::SwiftStrikesMotion(shot.Chara(), CHARA_XIAO);
    EXPECT_FLOAT_EQ(shot.Chara().motion_speed, expanded::kSwiftShotSpeed);

    Playing swing(0x25);
    expanded::QuickComboMotion(swing.Chara(), CHARA_XIAO);
    EXPECT_FLOAT_EQ(swing.Chara().motion_speed, -1.0f);
}

// The whole combo through UngagaKey_Play: at the Partisan's pace all three swings still chain and land their hits, in
// about three quarters of the frames.
TEST(Partisan, FasterSwingsStillChainAllThree) {
    ComboRun stock;
    {
        Combo combo(ITEM_WEAPON_FIGHTING_STICK);
        stock = combo.Run(QuickCombo);
    }
    ComboRun fast;
    {
        Combo combo(ITEM_WEAPON_PARTISAN);
        fast = combo.Run(QuickCombo);
    }

    const std::set<int> all = {0x25, 0x26, 0x27};
    EXPECT_EQ(stock.swings, all);
    EXPECT_EQ(stock.hit_swings, all);
    EXPECT_EQ(fast.swings, all);
    EXPECT_EQ(fast.hit_swings, all);
    EXPECT_NEAR(static_cast<float>(fast.frames) / static_cast<float>(stock.frames), 0.75f, 0.03f)
        << fast.frames << " / " << stock.frames;
}

// The run catches a broken chain: swing 2 at twice its step steps over its half-frame hand-over and never reaches swing 3.
TEST(Partisan, TwiceTheStepWouldNeverReachSwingThree) {
    Combo    combo(ITEM_WEAPON_FIGHTING_STICK);
    ComboRun run = combo.Run([](CCharacter &chara) {
        if (chara.motion_no >= 0x25 && chara.motion_no <= 0x27) {
            chara.motion_speed = chara.motion_type.motion_info[chara.motion_no].speed * 2.0f;
        }
    });
    EXPECT_EQ(run.swings, (std::set<int>{0x25, 0x26}));
}
