#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <utility>

#include "expanded/core/random.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/mardan.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "savedata.hpp"
#include "stockitem.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestSave;

constexpr int kFishKinds = 18;
constexpr int kSwords[] = {ITEM_WEAPON_MARDAN_EINS, ITEM_WEAPON_MARDAN_TWEI, ITEM_WEAPON_ARISE_MARDAN};

// Mardan Garayan's fish_info: usual size 10, largest 16, 200 to 400 FP.
FishInfo MardanGarayanInfo() {
    return {.model_size = 21.0f, .min_size = 10.0f, .max_size = 16.0f, .min_fp = 200, .max_fp = 400, .food_affinity = {}};
}

class Mardan : public ::testing::Test {
protected:
    void Own(int sword) { save_.status().chara_weapons[CHARA_TOAN][3].item_no = static_cast<s16>(sword); }

    // How many of rolls slots of a kind the spots fill without Garayans become Mardan and Baron Garayans.
    std::pair<int, int> Garayans(int rolls, int rare_interval) {
        expanded::SeedModRandom(119);
        int mardan = 0;
        int baron = 0;
        for (int i = 0; i < rolls; i++) {
            int kind = expanded::FishKind(14, rare_interval);
            mardan += kind == expanded::fish::kMardanGarayan ? 1 : 0;
            baron += kind == expanded::fish::kBaronGarayan ? 1 : 0;
            EXPECT_TRUE(kind == 14 || kind == expanded::fish::kMardanGarayan || kind == expanded::fish::kBaronGarayan) << kind;
        }
        return {mardan, baron};
    }

    TestSave save_;
};

} // namespace

TEST_F(Mardan, TheBestSwordOwnedCountsFromABagOrTheStorage) {
    EXPECT_EQ(expanded::MardanSword(), 0);
    save_.stock().weapons[12].item_no = ITEM_WEAPON_MARDAN_EINS;
    EXPECT_EQ(expanded::MardanSword(), ITEM_WEAPON_MARDAN_EINS);
    save_.status().chara_weapons[CHARA_TOAN][9].item_no = ITEM_WEAPON_MARDAN_TWEI;
    EXPECT_EQ(expanded::MardanSword(), ITEM_WEAPON_MARDAN_TWEI);
    save_.stock().weapons[29].item_no = ITEM_WEAPON_ARISE_MARDAN;
    EXPECT_EQ(expanded::MardanSword(), ITEM_WEAPON_ARISE_MARDAN);

    // Any character's bag counts.
    save_.stock().weapons[12].item_no = 0;
    save_.stock().weapons[29].item_no = 0;
    save_.status().chara_weapons[CHARA_TOAN][9].item_no = 0;
    save_.status().chara_weapons[CHARA_GORO][0].item_no = ITEM_WEAPON_ARISE_MARDAN;
    EXPECT_EQ(expanded::MardanSword(), ITEM_WEAPON_ARISE_MARDAN);
}

TEST_F(Mardan, OffWithTheWeaponAbilities) {
    OptionScope options;
    Own(ITEM_WEAPON_ARISE_MARDAN);
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(expanded::MardanSword(), 0);
    EXPECT_EQ(Garayans(2000, 20), std::make_pair(0, 0));
    EXPECT_EQ(expanded::FishSize(16.0f, MardanGarayanInfo()), 16.0f);
}

TEST_F(Mardan, NoSwordPaysOtherFishingPoints) {
    auto   fish = std::make_unique<CFish>();
    CFish *saved_angle = AngleFish;
    fish->info = MardanGarayanInfo();
    AngleFish = fish.get();
    for (int sword : kSwords) {
        Own(sword);
        for (float size : {8.0f, 13.0f, 16.0f, 30.0f}) {
            fish->size = size;
            for (int kind = 0; kind < kFishKinds; kind++) {
                fish->fish_kind = kind;
                int length = 0;
                int points = 0;
                ASSERT_EQ(FishingGetAngleFishSize(&length, &points), kind);
                EXPECT_EQ(points, fish->GetFP()) << sword << " " << size << " " << kind;
            }
        }
    }
    AngleFish = saved_angle;
}

TEST_F(Mardan, TweiAndAriseGiveASecondGarayanRoll) {
    for (int sword : {0, static_cast<int>(ITEM_WEAPON_MARDAN_EINS)}) {
        Own(sword);
        EXPECT_EQ(Garayans(2000, 20), std::make_pair(0, 0)) << sword;
    }
    for (int sword : {ITEM_WEAPON_MARDAN_TWEI, ITEM_WEAPON_ARISE_MARDAN}) {
        Own(sword);
        // One in rare_interval, one in kBaronOdds of those a Baron.
        auto [mardan, baron] = Garayans(20000, 20);
        EXPECT_NEAR(mardan + baron, 1000, 100) << sword;
        EXPECT_NEAR(baron, 200, 45) << sword;
        auto [day_mardan, day_baron] = Garayans(20000, 50);
        EXPECT_NEAR(day_mardan + day_baron, 400, 60) << sword;
    }
}

TEST_F(Mardan, TheSecondRollLeavesGarayansAndEmptySlots) {
    Own(ITEM_WEAPON_ARISE_MARDAN);
    expanded::SeedModRandom(5);
    for (int i = 0; i < 500; i++) {
        EXPECT_EQ(expanded::FishKind(expanded::fish::kMardanGarayan, 1), expanded::fish::kMardanGarayan);
        EXPECT_EQ(expanded::FishKind(expanded::fish::kBaronGarayan, 1), expanded::fish::kBaronGarayan);
        EXPECT_EQ(expanded::FishKind(-1, 1), -1);
    }
    // Every slot a Garayan when the chance is one in one.
    EXPECT_TRUE(std::ranges::contains(expanded::fish::kGarayans, expanded::FishKind(3, 1)));
}

TEST_F(Mardan, AGarayanFromTheRollIsSizedAsTheGameSizesIt) {
    Own(ITEM_WEAPON_MARDAN_TWEI);
    auto fish = std::make_unique<CFish>();
    fish->info = MardanGarayanInfo();
    for (unsigned seed = 1; seed <= 200; seed++) {
        std::srand(seed);
        fish->SetScale();
        ASSERT_GE(fish->size, 5.0f);
        ASSERT_LE(fish->size, 16.0f);
        ASSERT_FLOAT_EQ(fish->angle_model_scale, fish->size / 21.0f);
        ASSERT_FLOAT_EQ(fish->model_scale, fish->size / 25.0f);
    }
}

TEST_F(Mardan, AriseScalesUpToTwiceTheLargestAndHoldsTheEnds) {
    // Baron Garayan (10 to 30), Mardan Garayan (10 to 16), Niler (6 to 10), Den (8 to 20).
    for (auto [usual, largest] : {
             std::pair{10.0f, 30.0f},
             {10.0f, 16.0f},
             {6.0f,  10.0f},
             {8.0f,  20.0f}
    }) {
        const float smallest = 0.5f * usual;
        EXPECT_FLOAT_EQ(expanded::AriseSize(largest, usual, largest), 2.0f * largest);
        EXPECT_FLOAT_EQ(expanded::AriseSize(smallest, usual, largest), smallest);
        // Hard-capped at twice the largest.
        EXPECT_FLOAT_EQ(expanded::AriseSize(3.0f * largest, usual, largest), 2.0f * largest);

        float previous = smallest;
        for (int step = 1; step <= 1000; step++) {
            float t = static_cast<float>(step) / 1000.0f;
            float size = smallest + (largest - smallest) * t;
            float grown = expanded::AriseSize(size, usual, largest);
            ASSERT_GE(grown, previous);
            ASSERT_LE(grown, 2.0f * largest);
            // Bigger rolls are grown nearer twice: at least 1 + t times, t how far up the roll is.
            ASSERT_GE(grown, size * (1.0f + t) * (1.0f - 1e-6f));
            // Only the largest roll reaches the new largest's centimetre.
            if (step < 1000) {
                ASSERT_LT(static_cast<int>(grown * 10.0f), static_cast<int>(20.0f * largest));
            }
            previous = grown;
        }
    }
}

TEST_F(Mardan, AriseSmoothsTheScaledRange) {
    // Above the scaled usual size the second smoothing lifts the scaled size; at the scaled usual size it holds.
    const float usual = 10.0f;
    const float largest = 30.0f;
    const float smallest = 5.0f;
    auto        scaled = [&](float size) { return size * (1.0f + (size - smallest) / (largest - smallest)); };
    EXPECT_FLOAT_EQ(expanded::AriseSize(usual, usual, largest), scaled(usual));
    for (float size : {12.0f, 18.0f, 24.0f, 29.0f}) {
        float grown = expanded::AriseSize(size, usual, largest);
        float lifted_by = grown - scaled(size);
        float range = 2.0f * largest - scaled(usual);
        float t = (scaled(size) - scaled(usual)) / range;
        EXPECT_GT(lifted_by, 0.0f) << size;
        EXPECT_NEAR(lifted_by, expanded::kAriseSmoothing * range * t * t * (1.0f - t), 1e-3f) << size;
    }
}

TEST_F(Mardan, AriseGrowsTheGamesOwnSmoothedRoll) {
    auto fish = std::make_unique<CFish>();
    fish->info = MardanGarayanInfo();
    for (unsigned seed = 1; seed <= 200; seed++) {
        Own(ITEM_WEAPON_MARDAN_TWEI);
        std::srand(seed);
        fish->SetScale();
        const float game = fish->size;

        Own(ITEM_WEAPON_ARISE_MARDAN);
        std::srand(seed);
        fish->SetScale();
        ASSERT_FLOAT_EQ(fish->size, expanded::AriseSize(game, 10.0f, 16.0f)) << seed;
        ASSERT_FLOAT_EQ(fish->angle_model_scale, fish->size / 21.0f);
        ASSERT_FLOAT_EQ(fish->model_scale, fish->size / 25.0f);
    }
}
