#include <gtest/gtest.h>

#include <memory>

#include "bt_shot_effect.hpp"
#include "btactstatus.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/ungaga/halberd.hpp"
#include "expanded/weapons/ungaga/scorpion.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "mainselect.hpp"
#include "menu_inventory.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::test::OptionScope;
using expanded::test::TestFloor;
using expanded::test::TestSave;
using expanded::test::Wielding;

// chara out and poisoned, weapon_no equipped (carrying sphere when given) with 10 ABS; a Gunny worth 41 ABS on the
// floor in slot 2.
class Venom : public ::testing::Test {
protected:
    explicit Venom(int chara = CHARA_UNGAGA, int weapon_no = ITEM_WEAPON_SCORPION, int sphere = 0)
        : chara_(chara), wielding_(weapon_no, sphere) {}

    void SetUp() override {
        UserStatus = &save_.status();
        status().cur_chara = static_cast<s8>(chara_);
        status().ailments[chara_] = AILMENT_POISON;
        status().equipped_weapon_slot[chara_] = 0;
        Weapon() = wielding_.weapon();
        Weapon().experience = 10;
        floor_.SetUpModel(0, Monster::GUNNY);
        floor_.Spawn(2, 0);
        Gunny().exp = 41;
    }

    CUserStatus &status() { return save_.status(); }

    WEAPON_HAVE &Weapon() { return status().chara_weapons[chara_][0]; }

    MONSTOR &Gunny() { return floor_.unit().monster[2]; }

    bool Poisoned() { return (status().ailments[chara_] & AILMENT_POISON) != 0; }

    // The Gunny's poison taking hold, as CheckDmg's roll gives it.
    void PoisonTheGunny() { expanded::Poison(&Gunny(), 2); }

    int       chara_;
    TestFloor floor_;
    TestSave  save_;
    Wielding  wielding_;
};

class VenomBySuperSteve : public Venom {
protected:
    VenomBySuperSteve() : Venom(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SCORPION) {}
};

class VenomWithAnotherWeapon : public Venom {
protected:
    VenomWithAnotherWeapon() : Venom(CHARA_UNGAGA, ITEM_WEAPON_HALBERT) {}
};

class VenomFromAnotherSphere : public Venom {
protected:
    VenomFromAnotherSphere() : Venom(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HALBERT) {}
};

// A fishing spot's six fish, none of them hooked, and a bait model on the hook, out of debug mode (where every fish
// notices the bait from anywhere); Own() puts a weapon in the storage.
class VenomLure : public ::testing::Test {
protected:
    void SetUp() override {
        Fish = fish_.get();
        EsaFrame = &bait_;
        AngleFish = nullptr;
        DebugMode = 0;
    }

    void TearDown() override {
        Fish = saved_fish_;
        EsaFrame = saved_frame_;
        esa_type = saved_type_;
        AngleFish = saved_angle_;
        DebugMode = saved_debug_;
    }

    void Own(int weapon_no) { save_.stock().weapons[0].item_no = static_cast<s16>(weapon_no); }

    // The radius each fish notices bait esa (esa_info's index) from this frame (FishingStepFish).
    float Noticed(int esa) {
        esa_type = esa;
        FishingStepFish();
        return fish_[5].food_radius;
    }

    TestSave                 save_;
    std::unique_ptr<CFish[]> fish_ = std::make_unique<CFish[]>(6);
    CFrameVu1                bait_;
    CFish                   *saved_fish_ = Fish;
    CFrameVu1               *saved_frame_ = EsaFrame;
    int                      saved_type_ = esa_type;
    CFish                   *saved_angle_ = AngleFish;
    s32                      saved_debug_ = DebugMode;
};

constexpr int kPoisonousApple = 6; // esa_info's index of the Poisonous Apple

} // namespace

TEST_F(Venom, ItsPoisonTakingHoldCuresUngagaAndFeedsTheWeaponHalfTheAbs) {
    PoisonTheGunny();

    EXPECT_TRUE(expanded::VenomActive());
    EXPECT_GT(Gunny().poison_timer, 0);
    EXPECT_FALSE(Poisoned());
    EXPECT_EQ(Weapon().experience, 10 + 41 / expanded::kVenomAbsShare); // 20, rounded down
}

TEST_F(Venom, AMonsterAlreadyPoisonedDoesNotCountAgain) {
    PoisonTheGunny();
    status().ailments[CHARA_UNGAGA] = AILMENT_POISON;
    PoisonTheGunny();

    EXPECT_TRUE(Poisoned());
    EXPECT_EQ(Weapon().experience, 30);
}

TEST_F(Venom, NeverPastTheWeaponsMaximum) {
    int max_exp = GetWeaponMaxExp(&Weapon());
    Weapon().experience = static_cast<s16>(max_exp - 5);
    PoisonTheGunny();

    EXPECT_EQ(Weapon().experience, max_exp);
}

TEST_F(Venom, NorBackFromAboveIt) {
    int max_exp = GetWeaponMaxExp(&Weapon());
    Weapon().experience = static_cast<s16>(max_exp + 30); // Overtraining's
    PoisonTheGunny();

    EXPECT_EQ(Weapon().experience, max_exp + 30);
    EXPECT_FALSE(Poisoned());
}

TEST_F(Venom, AWeaponWithoutAbsTakesNone) {
    Weapon().item_no = ITEM_WEAPON_FIGHTING_STICK_BROKEN; // the record equipped; the Scorpion still in hand
    PoisonTheGunny();

    EXPECT_FALSE(Poisoned());
    EXPECT_EQ(Weapon().experience, 10);
}

TEST_F(Venom, AWorthlessMonsterOnlyCures) {
    Gunny().exp = 1;
    PoisonTheGunny();

    EXPECT_FALSE(Poisoned());
    EXPECT_EQ(Weapon().experience, 10);
}

TEST_F(VenomBySuperSteve, CuresXiaoAndFeedsSuperSteve) {
    PoisonTheGunny();

    EXPECT_FALSE(Poisoned());
    EXPECT_EQ(Weapon().item_no, ITEM_WEAPON_SUPER_STEVE);
    EXPECT_EQ(Weapon().experience, 30);
}

TEST_F(VenomWithAnotherWeapon, DoesNothing) {
    PoisonTheGunny();

    EXPECT_FALSE(expanded::VenomActive());
    EXPECT_GT(Gunny().poison_timer, 0);
    EXPECT_TRUE(Poisoned());
    EXPECT_EQ(Weapon().experience, 10);
}

TEST_F(VenomFromAnotherSphere, DoesNothing) {
    PoisonTheGunny();

    EXPECT_TRUE(Poisoned());
    EXPECT_EQ(Weapon().experience, 10);
}

TEST_F(Venom, TheChargeShotFliesAtTheScorpionsTier) {
    BT_SHOT_EFFECT  own = {};
    BT_SHOT_EFFECT *saved_data = CharaMainShotEffect().effect_data;
    s32             saved_count = CharaMainShotEffect().slot_count;
    CSHOT_EFFECT   *saved_live = NowMainEffect;
    own.speed[0] = 0.2f;
    own.radius[0] = 8.0f;
    CharaMainShotEffect().effect_data = &own;
    CharaMainShotEffect().slot_count = 6;
    NowMainEffect = &CharaMainShotEffect();

    expanded::TornadoChargeStep();
    EXPECT_EQ(expanded::TornadoChargeTier(), &expanded::kTornadoScorpion);
    EXPECT_FLOAT_EQ(CharaMainShotEffect().effect_data->speed[0], 0.2f * 1.7f);
    EXPECT_FLOAT_EQ(CharaMainShotEffect().effect_data->radius[0], 8.0f * 1.3f);
    EXPECT_FLOAT_EQ(CharaMainShotEffect().chara[0].scale[0], 1.3f);

    status().cur_chara = CHARA_TOAN;
    expanded::TornadoChargeStep();
    EXPECT_EQ(CharaMainShotEffect().effect_data, &own);
    CharaMainShotEffect().effect_data = saved_data;
    CharaMainShotEffect().slot_count = saved_count;
    NowMainEffect = saved_live;
}

TEST_F(VenomBySuperSteve, TheChargedShotFliesAtTheScorpionsTier) {
    auto shots = std::make_unique<CSHOT>();
    shots->used[0] = 1;
    shots->size[0] = 1.0f;
    shots->vector[0][2] = 5.0f;
    shots->damage[0] = 40;
    BtActStatus = {};
    BtActStatus.action_on = 1;
    BtActStatus.action_no = 12;
    for (int frame = 0; frame < expanded::kTornadoShotFrames; frame++) {
        expanded::TornadoChargeStep();
    }
    BtActStatus.action_on = 0;
    expanded::TornadoChargeStep();
    expanded::PelletFired(*shots, 0);

    EXPECT_EQ(expanded::TornadoShotTier(), &expanded::kTornadoScorpion);
    EXPECT_FLOAT_EQ(shots->size[0], 1.3f);
    EXPECT_FLOAT_EQ(shots->vector[0][2], 5.0f * 1.7f);
    EXPECT_EQ(shots->damage[0], 60);
    EXPECT_FLOAT_EQ(expanded::XiaoShotWear(1.0f), 2.0f);
    BtActStatus = {};
}

TEST_F(VenomLure, OwnedThePoisonousAppleIsNoticedFromTwiceAsFar) {
    ASSERT_EQ(esa_info[kPoisonousApple].item_no, ITEM_POISONOUS_APPLE);
    EXPECT_FLOAT_EQ(Noticed(kPoisonousApple), 25.0f);

    Own(ITEM_WEAPON_SCORPION);
    EXPECT_FLOAT_EQ(Noticed(kPoisonousApple), 25.0f * expanded::kVenomLure);
    EXPECT_EQ(fish_[0].food_kind, kPoisonousApple);
    EXPECT_FLOAT_EQ(fish_[0].food_radius, 50.0f);
}

TEST_F(VenomLure, EveryOtherBaitAsTheGameHasIt) {
    Own(ITEM_WEAPON_SCORPION);
    for (int esa = 0; esa < std::ssize(esa_info); esa++) {
        if (esa != kPoisonousApple) {
            EXPECT_FLOAT_EQ(Noticed(esa), esa_info[esa].radius) << esa_info[esa].item_no;
        }
    }
}

TEST_F(VenomLure, DoublesBaitFlamboyanceWithIt) {
    Own(ITEM_WEAPON_SCORPION);
    save_.status().chara_weapons[CHARA_XIAO][3].item_no = ITEM_WEAPON_FLAMINGO;
    EXPECT_FLOAT_EQ(Noticed(kPoisonousApple), (25.0f + expanded::kBaitFlamboyance) * expanded::kVenomLure);
    EXPECT_FLOAT_EQ(expanded::BaitNoticeRadius(ITEM_EVY, 50.0f), 50.0f + expanded::kBaitFlamboyance);
}

TEST_F(VenomLure, AScorpionInAnyBagCounts) {
    save_.status().chara_weapons[CHARA_GORO][9].item_no = ITEM_WEAPON_SCORPION;
    EXPECT_FLOAT_EQ(Noticed(kPoisonousApple), 50.0f);
}

TEST_F(VenomLure, OffWithTheWeaponAbilities) {
    OptionScope options;
    Own(ITEM_WEAPON_SCORPION);
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_FLOAT_EQ(Noticed(kPoisonousApple), 25.0f);
}
