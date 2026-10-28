#include <gtest/gtest.h>

#include <memory>

#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "shop.hpp"
#include "stockitem.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestSave;

constexpr int kFishKinds = 18;

// A fish on the line (AngleFish) 15 long, between its kind's 10 and 20, so the game pays 150 of its 100 to 200 FP.
class FairyFortune : public ::testing::Test {
protected:
    void SetUp() override {
        fish_->size = 15.0f;
        fish_->info.min_size = 10.0f;
        fish_->info.max_size = 20.0f;
        fish_->info.min_fp = 100;
        fish_->info.max_fp = 200;
        AngleFish = fish_.get();
    }

    void TearDown() override { AngleFish = saved_angle_; }

    // The FP a catch of fish_kind pays, as the catch's evaluation reads it (FishingGetAngleFishSize).
    int Paid(int fish_kind) {
        fish_->fish_kind = fish_kind;
        int size = 0;
        int points = 0;
        EXPECT_EQ(FishingGetAngleFishSize(&size, &points), fish_kind);
        EXPECT_EQ(size, 150);
        return points;
    }

    TestSave               save_;
    std::unique_ptr<CFish> fish_ = std::make_unique<CFish>();
    CFish                 *saved_angle_ = AngleFish;
};

} // namespace

TEST_F(FairyFortune, WithoutTheRingEveryFishPaysItsOwn) {
    save_.status().chara_weapons[CHARA_RUBY][0].item_no = ITEM_WEAPON_CRYSTAL_RING;
    for (int kind = 0; kind < kFishKinds; kind++) {
        EXPECT_EQ(Paid(kind), 150) << kind;
    }
}

TEST_F(FairyFortune, OwnedEveryFishButTheGarayansPaysTwice) {
    save_.status().chara_weapons[CHARA_RUBY][2].item_no = ITEM_WEAPON_FAIRY_S_RING;
    for (int kind = 0; kind < kFishKinds; kind++) {
        bool garayan = kind == expanded::kMardanGarayan || kind == expanded::kBaronGarayan;
        EXPECT_EQ(Paid(kind), garayan ? 150 : 150 * expanded::kFairyFortune) << kind;
    }
    EXPECT_EQ(Paid(expanded::kMardanGarayan), 150);
    EXPECT_EQ(Paid(expanded::kBaronGarayan), 150);
}

TEST_F(FairyFortune, TheGarayansAreMardanAndBaron) {
    // GetFishMsgNo's system messages: 45 "Mardan Garayan", 37 "Baron Garayan".
    EXPECT_EQ(GetFishMsgNo(expanded::kMardanGarayan), 45);
    EXPECT_EQ(GetFishMsgNo(expanded::kBaronGarayan), 37);
    EXPECT_EQ(expanded::kGarayans.size(), 2u);
}

TEST_F(FairyFortune, ARingInStorageOrAnotherBagCounts) {
    save_.stock().weapons[20].item_no = ITEM_WEAPON_FAIRY_S_RING;
    EXPECT_EQ(Paid(0), 300);
    save_.stock().weapons[20].item_no = 0;
    save_.status().chara_weapons[CHARA_TOAN][7].item_no = ITEM_WEAPON_FAIRY_S_RING;
    EXPECT_EQ(Paid(0), 300);
}

TEST_F(FairyFortune, OffWithTheWeaponAbilities) {
    OptionScope options;
    save_.status().chara_weapons[CHARA_RUBY][0].item_no = ITEM_WEAPON_FAIRY_S_RING;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(Paid(0), 150);
}
