#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "bt_shot_effect.hpp"
#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/blade_scale.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/heavens_cloud_shot.hpp"
#include "expanded/weapons/toan/heavens_cloud.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"

namespace {

// Toan with weapon_no in hand, his sword's grip at the origin.
struct Toan {
    expanded::test::TestFloor floor;
    expanded::test::Wielding  wielding;
    CFrame                    grip;
    CFrame                   *saved_frame = MainWeapon.frame;
    CCharacter               *saved_weapon = NowWeapon;

    explicit Toan(int weapon_no) : wielding(weapon_no) {
        MainWeapon.frame = &grip;
        NowWeapon = &MainWeapon;
        BtActStatus = {};
    }

    ~Toan() {
        BtActStatus = {};
        expanded::TyphoonStep();
        MainWeapon.frame = saved_frame;
        NowWeapon = saved_weapon;
    }

    // frames of the whirlwind's charge.
    void Charge(int frames) {
        BtActStatus.action_no = 0xE;
        BtActStatus.charge_level = 2;
        for (int i = 0; i < frames; i++) {
            expanded::TyphoonStep();
        }
    }
};

} // namespace

TEST(HeavensCloud, TheWhirlwindChargeGrowsTheSwordToThreeTimes) {
    Toan toan(ITEM_WEAPON_HEAVEN_S_CLOUD);
    toan.Charge(120); // half way: (1/2)^2.5 of the growth
    EXPECT_NEAR(expanded::TyphoonScale(), 1.0f + 2.0f * std::pow(0.5f, 2.5f), 1e-4f);
    toan.Charge(expanded::kTyphoonFrames);
    EXPECT_FLOAT_EQ(expanded::TyphoonScale(), expanded::kTyphoonScale);
    EXPECT_FLOAT_EQ(toan.grip.scale[0], expanded::kTyphoonScale);
    // The whirlwind keeps it, and it is its own size again after.
    BtActStatus.action_no = 0x18;
    expanded::TyphoonStep();
    EXPECT_FLOAT_EQ(expanded::TyphoonScale(), expanded::kTyphoonScale);
    BtActStatus.action_no = 0;
    expanded::TyphoonStep();
    EXPECT_FLOAT_EQ(expanded::TyphoonScale(), 1.0f);
    EXPECT_FLOAT_EQ(toan.grip.scale[0], 1.0f);
}

TEST(HeavensCloud, OnlyTheWhirlwindsChargeAndOnlyTheHeavensCloud) {
    {
        Toan toan(ITEM_WEAPON_HEAVEN_S_CLOUD);
        BtActStatus.action_no = 0xE;
        BtActStatus.charge_level = 1; // the lunge's
        expanded::TyphoonStep();
        EXPECT_FLOAT_EQ(expanded::TyphoonScale(), 1.0f);
    }
    Toan toan(ITEM_WEAPON_DAGGER);
    toan.Charge(expanded::kTyphoonFrames);
    EXPECT_FLOAT_EQ(expanded::TyphoonScale(), 1.0f);
}

TEST(HeavensCloud, AThreeTimesBladeHitsAlongItsWholeLength) {
    Toan                            toan(ITEM_WEAPON_HEAVEN_S_CLOUD);
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved = NowColData;
    NowColData = hits.get();
    float tip[4] = {0.0f, 0.0f, 27.0f, 1.0f};
    hits->hit[hits->Set(tip, 20, 1, 12.0f, 0.0f, 2, 2, 0, 0)].owner = 0;
    expanded::PlantAlongBlade(tip, 3.0f);
    EXPECT_EQ(hits->active[1] + hits->active[2], 2);
    EXPECT_FLOAT_EQ(hits->hit[1].pos[2], 9.0f);
    EXPECT_FLOAT_EQ(hits->hit[2].pos[2], 18.0f);
    EXPECT_EQ(hits->active[3], 0);
    NowColData = saved;
}

TEST(HeavensCloudShot, TheHoldChargesInTwoStages) {
    EXPECT_FLOAT_EQ(expanded::TyphoonShotCharge(0), 0.0f);
    EXPECT_FLOAT_EQ(expanded::TyphoonShotCharge(22), 0.0f); // short of the first stage
    EXPECT_GT(expanded::TyphoonShotCharge(23), 0.0f);
    EXPECT_FLOAT_EQ(expanded::TyphoonShotCharge(177), 1.0f); // the second
    EXPECT_FLOAT_EQ(expanded::TyphoonShotCharge(400), 1.0f);
}

TEST(HeavensCloudShot, AFullChargeGrowsThePelletAndBlowsTheMonstersAway) {
    expanded::test::TestFloor floor;
    floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HEAVEN_S_CLOUD);
    steve.weapon().attack = 100;
    steve.weapon().best_elem = -1;
    floor.SetUpModel(0, expanded::Monster::MASTER_JACKET);
    floor.Spawn(0, 0);
    floor.unit().monster[0].revealed = -1;
    floor.unit().monster[0].knockback[2] = 1.0f;
    floor.unit().chara[0][0].SetPosition(30.0f, 0.0f, 0.0f);
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved = NowColData;
    NowColData = hits.get();

    BtActStatus = {};
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 12;
    for (int i = 0; i < expanded::kTyphoonShotFrames; i++) {
        expanded::TyphoonShotStep();
    }
    BtActStatus = {};
    expanded::TyphoonShotStep();
    static CSHOT shots;
    shots.size[0] = 1.0f;
    shots.damage[0] = 40;
    expanded::TyphoonShotFired(shots, 0);
    EXPECT_FLOAT_EQ(shots.size[0], expanded::kTyphoonPellet);
    EXPECT_EQ(shots.damage[0], 60);
    EXPECT_FLOAT_EQ(expanded::XiaoShotWear(1.0f), expanded::kChargedShotWear); // an empowered shot wears as a charged one

    const float landed[4] = {25.0f, 0.0f, 0.0f, 1.0f};
    expanded::TyphoonShotEnded(shots, 0, landed, true);
    const COLLISION_HIT &blast = hits->hit[0];
    ASSERT_EQ(hits->active[0], 1);
    EXPECT_EQ(blast.damage, 75); // 0.75 x 100 x (0.5 + 0.5)
    EXPECT_GT(blast.knockback_speed, 0.0f);
    NowColData = saved;
}

// A charged pellet that never ended (its floor left behind) leaves nothing for the next pellet in its slot to burst with.
TEST(HeavensCloudShot, AnUnchargedPelletDoesNotBurstWithAnEarlierOnesCharge) {
    expanded::test::TestFloor floor;
    floor.status().cur_chara = CHARA_XIAO;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HEAVEN_S_CLOUD);
    steve.weapon().attack = 100;
    steve.weapon().best_elem = -1;
    floor.SetUpModel(0, expanded::Monster::MASTER_JACKET);
    floor.Spawn(0, 0);
    floor.unit().monster[0].revealed = -1;
    floor.unit().chara[0][0].SetPosition(30.0f, 0.0f, 0.0f);
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved = NowColData;
    NowColData = hits.get();

    BtActStatus = {};
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 12;
    for (int i = 0; i < expanded::kTyphoonShotFrames; i++) {
        expanded::TyphoonShotStep();
    }
    BtActStatus = {};
    expanded::TyphoonShotStep();
    static CSHOT shots;
    shots.size[0] = 1.0f;
    shots.damage[0] = 40;
    expanded::TyphoonShotFired(shots, 0); // charged, and never ends

    shots.size[0] = 1.0f;
    shots.damage[0] = 40;
    expanded::TyphoonShotFired(shots, 0); // uncharged, into the same slot
    EXPECT_FLOAT_EQ(shots.size[0], 1.0f);
    const float landed[4] = {25.0f, 0.0f, 0.0f, 1.0f};
    expanded::TyphoonShotEnded(shots, 0, landed, true);
    EXPECT_EQ(hits->active[0], 0);
    NowColData = saved;
}

TEST(HeavensCloud, TheGrownWhirlwindHitsEveryMonsterInReachAndThrowsThemClear) {
    Toan toan(ITEM_WEAPON_HEAVEN_S_CLOUD);
    toan.wielding.weapon().attack = 100;
    toan.wielding.weapon().best_elem = -1;
    CMonstorUnit &unit = toan.floor.unit();
    toan.floor.SetUpModel(0, expanded::Monster::MASTER_JACKET);
    const float distances[] = {10.0f, 50.0f, 75.0f}; // in reach, in reach, past it (60 at 3x)
    for (int monster = 0; monster < 3; monster++) {
        toan.floor.Spawn(monster, 0);
        unit.monster[monster].revealed = -1;
        unit.monster[monster].knockback[2] = 1.0f;
        unit.chara[monster][0].SetPosition(distances[monster], 0.0f, 0.0f);
    }
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits = NowColData;
    NowColData = hits.get();
    static BT_SHOT_EFFECT entry = {};
    entry.radius[0] = 20.0f;
    static CSHOT_EFFECT tornado;
    tornado.effect_data = &entry;
    tornado.current_slot = 0;
    tornado.active[0] = 1;
    CSHOT_EFFECT *saved_effect = NowMainEffect;
    NowMainEffect = &tornado;

    toan.Charge(expanded::kTyphoonFrames);
    BtActStatus.action_no = 0x18;
    expanded::TyphoonWhirlwind();
    EXPECT_FLOAT_EQ(entry.radius[0], 60.0f);
    int planted = 0;
    for (int i = 0; i < 96; i++) {
        if (hits->active[i] != 0) {
            planted++;
            EXPECT_EQ(hits->hit[i].damage, 150);
            EXPECT_GT(hits->hit[i].knockback_speed, 0.0f);
        }
    }
    EXPECT_EQ(planted, 2);
    // Over, the tornado is its own size again.
    BtActStatus.action_no = 0;
    expanded::TyphoonStep();
    EXPECT_FLOAT_EQ(entry.radius[0], 20.0f);
    NowMainEffect = saved_effect;
    NowColData = saved_hits;
}
