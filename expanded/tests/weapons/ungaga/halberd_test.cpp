#include <gtest/gtest.h>

#include <memory>

#include "bt_shot_effect.hpp"
#include "btactstatus.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/ungaga/halberd.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

extern CSHOT_EFFECT CharaMainEffect;

namespace {

using expanded::test::TestFloor;
using expanded::test::Wielding;

// Ungaga's charge shot as the game enters it: speeds 0.2, 0.9, 0.2, 0.2 and radii 8, 8, 6, 6, in six slots.
BT_SHOT_EFFECT ChargeShot() {
    BT_SHOT_EFFECT shot = {};
    const float    speed[4] = {0.2f, 0.9f, 0.2f, 0.2f};
    const float    radius[4] = {8.0f, 8.0f, 6.0f, 6.0f};
    for (int phase = 0; phase < 4; phase++) {
        shot.speed[phase] = speed[phase];
        shot.radius[phase] = radius[phase];
    }
    return shot;
}

// chara out with weapon_no in hand, his main effect entered with ChargeShot.
class TornadoCharge : public ::testing::Test {
protected:
    explicit TornadoCharge(int weapon_no = ITEM_WEAPON_HALBERT, int chara = CHARA_UNGAGA) : chara_(chara), wielding_(weapon_no) {}

    void SetUp() override {
        floor_.status().cur_chara = static_cast<s8>(chara_);
        CharaMainEffect.effect_data = &own_;
        CharaMainEffect.slot_count = 6;
        NowMainEffect = &CharaMainEffect;
        Size(1.0f);
    }

    void TearDown() override {
        floor_.status().cur_chara = CHARA_TOAN;
        expanded::TornadoChargeStep();
        CharaMainEffect.effect_data = saved_data_;
        CharaMainEffect.slot_count = saved_count_;
        NowMainEffect = saved_live_;
        Size(1.0f);
    }

    // The game entering the effect again (a floor load, a menu): its own description, its slots their own size.
    void Enter() {
        CharaMainEffect.effect_data = &own_;
        Size(1.0f);
    }

    static void Size(float size) {
        for (CCharacter &slot : CharaMainEffect.chara) {
            slot.SetScale(size, size, size);
        }
    }

    // Whether the effect reads its description at tier (the slots it uses drawn at its size).
    ::testing::AssertionResult AtTier(const expanded::TornadoTier &tier) {
        const BT_SHOT_EFFECT *data = CharaMainEffect.effect_data;
        if (data == &own_) {
            return ::testing::AssertionFailure() << "it reads its own description";
        }
        for (int phase = 0; phase < 4; phase++) {
            if (data->speed[phase] != own_.speed[phase] * tier.travel || data->radius[phase] != own_.radius[phase] * tier.size) {
                return ::testing::AssertionFailure() << "phase " << phase << ": speed " << data->speed[phase] << ", radius "
                                                     << data->radius[phase];
            }
        }
        for (int slot = 0; slot < 6; slot++) {
            if (CharaMainEffect.chara[slot].scale[0] != tier.size) {
                return ::testing::AssertionFailure() << "slot " << slot << " at " << CharaMainEffect.chara[slot].scale[0];
            }
        }
        return ::testing::AssertionSuccess();
    }

    // Whether it reads its own description, its slots their own size.
    bool Own() const { return CharaMainEffect.effect_data == &own_ && CharaMainEffect.chara[0].scale[0] == 1.0f; }

    BT_SHOT_EFFECT  own_ = ChargeShot();
    int             chara_;
    TestFloor       floor_;
    Wielding        wielding_;
    BT_SHOT_EFFECT *saved_data_ = CharaMainEffect.effect_data;
    s32             saved_count_ = CharaMainEffect.slot_count;
    CSHOT_EFFECT   *saved_live_ = NowMainEffect;
};

class TornadoChargeWithAnotherWeapon : public TornadoCharge {
protected:
    TornadoChargeWithAnotherWeapon() : TornadoCharge(ITEM_WEAPON_5_FOOT_NAIL) {}
};

class TornadoChargeByAnotherCharacter : public TornadoCharge {
protected:
    TornadoChargeByAnotherCharacter() : TornadoCharge(ITEM_WEAPON_HALBERT, CHARA_TOAN) {}
};

// Xiao with weapon_no in hand (carrying sphere when given); a pellet of 41 fired into slot 3 along +x.
class TornadoShot : public ::testing::Test {
protected:
    explicit TornadoShot(int weapon_no = ITEM_WEAPON_SUPER_STEVE, int sphere = ITEM_WEAPON_HALBERT) : wielding_(weapon_no, sphere) {}

    void SetUp() override {
        BtActStatus = {};
        floor_.status().cur_chara = CHARA_XIAO;
        *shots_ = CSHOT{};
        shots_->used[3] = 1;
        shots_->size[3] = 1.0f;
        shots_->vector[3][0] = 3.5f;
        shots_->damage[3] = 41;
        floor_.SetUpModel(0, expanded::Monster::MASTER_JACKET); // a new floor: nothing held
    }

    void TearDown() override {
        BtActStatus = {};
        expanded::ChargedShotWear(1.0f);
    }

    // The slingshot drawn and held for frames, then let go into slot 3.
    void HoldAndFire(int frames) {
        BtActStatus.action_on = 1;
        BtActStatus.action_no = 12;
        for (int frame = 0; frame < frames; frame++) {
            expanded::TornadoChargeStep();
        }
        BtActStatus.action_on = 0;
        expanded::TornadoChargeStep();
        expanded::PelletFired(*shots_, 3);
    }

    TestFloor              floor_;
    Wielding               wielding_;
    std::unique_ptr<CSHOT> shots_ = std::make_unique<CSHOT>();
};

class TornadoShotFromHercules : public TornadoShot {
protected:
    TornadoShotFromHercules() : TornadoShot(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_HERCULES_WRATH) {}
};

class TornadoShotFromAnotherSphere : public TornadoShot {
protected:
    TornadoShotFromAnotherSphere() : TornadoShot(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_JAVELIN) {}
};

} // namespace

TEST(TornadoTiers, GrowDownTheLine) {
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_HALBERT), &expanded::kTornadoHalberd);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_SCORPION), &expanded::kTornadoScorpion);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_MIRAGE), &expanded::kTornadoMirage);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_CACTUS), &expanded::kTornadoMirage);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_HERCULES_WRATH), &expanded::kTornadoHercules);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_TERRA_SWORD), &expanded::kTornadoHercules);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_BABEL_S_SPEAR), &expanded::kTornadoHercules);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_JAVELIN), nullptr);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_DE_SANGA), nullptr);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_PARTISAN), nullptr);
    EXPECT_EQ(expanded::TornadoTierOf(ITEM_WEAPON_5_FOOT_NAIL), nullptr);
    EXPECT_FLOAT_EQ(expanded::kTornadoHalberd.travel, 1.4f);
    EXPECT_FLOAT_EQ(expanded::kTornadoHalberd.size, 1.1f);
    EXPECT_FLOAT_EQ(expanded::kTornadoScorpion.travel, 1.7f);
    EXPECT_FLOAT_EQ(expanded::kTornadoScorpion.size, 1.3f);
    EXPECT_FLOAT_EQ(expanded::kTornadoMirage.travel, 2.0f);
    EXPECT_FLOAT_EQ(expanded::kTornadoMirage.size, 1.5f);
    EXPECT_FLOAT_EQ(expanded::kTornadoHercules.travel, 2.5f);
    EXPECT_FLOAT_EQ(expanded::kTornadoHercules.size, 1.7f);
}

TEST_F(TornadoCharge, TheHalberdsShotFliesFasterAndLarger) {
    expanded::DungeonMonstersTick();

    EXPECT_EQ(expanded::TornadoChargeTier(), &expanded::kTornadoHalberd);
    EXPECT_TRUE(AtTier(expanded::kTornadoHalberd));
    // The game's own description is left as it is.
    EXPECT_FLOAT_EQ(own_.speed[1], 0.9f);
    EXPECT_FLOAT_EQ(own_.radius[0], 8.0f);
}

TEST_F(TornadoCharge, ItHoldsUntilTheSpearGoes) {
    expanded::TornadoChargeStep();
    expanded::TornadoChargeStep();
    EXPECT_TRUE(AtTier(expanded::kTornadoHalberd));

    wielding_.weapon().item_no = ITEM_WEAPON_JAVELIN;
    expanded::TornadoChargeStep();
    EXPECT_TRUE(Own());
}

TEST_F(TornadoCharge, AnotherTierOfTheLineScalesFromTheEffectsOwn) {
    expanded::TornadoChargeStep();
    wielding_.weapon().item_no = ITEM_WEAPON_HERCULES_WRATH;
    expanded::TornadoChargeStep();

    EXPECT_TRUE(AtTier(expanded::kTornadoHercules));
}

TEST_F(TornadoCharge, EnteredAgainItIsScaledAgain) {
    expanded::TornadoChargeStep();
    Enter();
    expanded::TornadoChargeStep();

    EXPECT_TRUE(AtTier(expanded::kTornadoHalberd));
}

TEST_F(TornadoCharge, NotWhileTheBrokenWeaponsEffectIsLive) {
    NowMainEffect = &CharaMainEffectCrash;
    expanded::TornadoChargeStep();

    EXPECT_TRUE(Own());
}

TEST_F(TornadoChargeWithAnotherWeapon, LeavesTheShotAsItIs) {
    expanded::TornadoChargeStep();

    EXPECT_EQ(expanded::TornadoChargeTier(), nullptr);
    EXPECT_TRUE(Own());
}

TEST_F(TornadoChargeByAnotherCharacter, LeavesTheShotAsItIs) {
    expanded::TornadoChargeStep();

    EXPECT_TRUE(Own());
}

TEST_F(TornadoShot, AChargedPelletFliesFasterLargerAndHarder) {
    HoldAndFire(expanded::kTornadoShotFrames);

    EXPECT_FLOAT_EQ(shots_->size[3], expanded::kTornadoHalberd.size);
    EXPECT_FLOAT_EQ(shots_->vector[3][0], 3.5f * expanded::kTornadoHalberd.travel);
    EXPECT_EQ(shots_->damage[3], 62); // 61.5, halves to even
    EXPECT_FLOAT_EQ(expanded::ChargedShotWear(1.0f), expanded::kChargedShotWear);
}

TEST_F(TornadoShot, DamageHalvesRoundToEven) {
    shots_->damage[3] = 43;
    HoldAndFire(expanded::kTornadoShotFrames);

    EXPECT_EQ(shots_->damage[3], 64); // 64.5
}

TEST_F(TornadoShot, AShortHoldFiresAnOrdinaryPellet) {
    HoldAndFire(expanded::kTornadoShotFrames - 1);

    EXPECT_FLOAT_EQ(shots_->size[3], 1.0f);
    EXPECT_FLOAT_EQ(shots_->vector[3][0], 3.5f);
    EXPECT_EQ(shots_->damage[3], 41);
    EXPECT_FLOAT_EQ(expanded::ChargedShotWear(1.0f), 1.0f);
}

TEST_F(TornadoShot, OnlyTheFirstPelletOfTheReleaseIsCharged) {
    HoldAndFire(expanded::kTornadoShotFrames);
    shots_->used[4] = 1;
    shots_->size[4] = 1.0f;
    shots_->damage[4] = 41;
    expanded::PelletFired(*shots_, 4);

    EXPECT_FLOAT_EQ(shots_->size[4], 1.0f);
    EXPECT_EQ(shots_->damage[4], 41);
}

TEST_F(TornadoShotFromHercules, FliesAtItsTier) {
    HoldAndFire(expanded::kTornadoShotFrames);

    EXPECT_FLOAT_EQ(shots_->size[3], expanded::kTornadoHercules.size);
    EXPECT_FLOAT_EQ(shots_->vector[3][0], 3.5f * expanded::kTornadoHercules.travel);
}

TEST_F(TornadoShotFromAnotherSphere, LeavesThePelletAsItIs) {
    HoldAndFire(expanded::kTornadoShotFrames);

    EXPECT_EQ(expanded::TornadoShotTier(), nullptr);
    EXPECT_FLOAT_EQ(shots_->size[3], 1.0f);
    EXPECT_EQ(shots_->damage[3], 41);
}
