#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>
#include <vector>

#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/small_sword.hpp"
#include "expanded/weapons/toan/tsukikage.hpp"
#include "fish.hpp"
#include "itemdata.hpp"
#include "stockitem.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

using expanded::test::OptionScope;
using expanded::test::TestSave;
using expanded::test::TownClock;
using expanded::test::Wielding;

namespace {

// The game's clock: an hour into each part of the day.
constexpr float kMorning = 9.0f, kDay = 0.0f, kEvening = 3.0f, kNight = 6.0f;

// esa_info's indexes.
constexpr int kEvy = 0, kMimi = 1, kThrobbingCherry = 3, kPoisonousApple = 6;

// The Bon likes the Evy (1), the Mimi (0.5) and the Throbbing Cherry (0.2), and not the Poisonous Apple.
const FishInfo &Bon() {
    return *GetFishInfo(expanded::fish::kBon);
}

// The Bon's affinity for the Evy, the Mimi, the Throbbing Cherry and the Poisonous Apple.
std::vector<float> BonAffinities() {
    std::vector<float> affinities;
    for (int bait : {kEvy, kMimi, kThrobbingCherry, kPoisonousApple}) {
        affinities.push_back(expanded::BaitAffinity(Bon(), bait));
    }
    return affinities;
}

const std::vector<float> kRetail = {1.0f, 0.5f, 0.2f, 0.0f};
const std::vector<float> kMoonlit = {1.0f, 1.0f, 1.0f, 0.0f};

// How many of n of the game's rolls at probability (rand_check) come up from seed.
int Rolls(unsigned seed, int n, float probability) {
    std::srand(seed);
    int up = 0;
    for (int i = 0; i < n; i++) {
        up += std::rand() % 1000 < static_cast<int>(probability * 1000.0f) ? 1 : 0;
    }
    return up;
}

} // namespace

TEST(Tsukikage, ChargeBuildsTwiceAsFast) {
    for (int weapon : {ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_HEAVEN_S_CLOUD}) {
        Wielding wielding(weapon);
        EXPECT_FLOAT_EQ(expanded::MoonlitFocusCharge(1.0f / 60.0f), 2.0f / 60.0f);
        EXPECT_TRUE(expanded::QuickDrawSword());
    }

    Wielding small_sword(ITEM_WEAPON_SMALL_SWORD);
    EXPECT_FLOAT_EQ(expanded::MoonlitFocusCharge(1.0f / 60.0f), 1.0f / 60.0f);
}

TEST(Tsukikage, SpherePelletsFlyTwiceAsFast) {
    for (int sphere : {ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_HEAVEN_S_CLOUD}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        float    vector[4] = {3.0f, -1.0f, 4.0f, 1.0f};
        expanded::MoonlitFocusShot(vector);
        EXPECT_FLOAT_EQ(vector[0], 6.0f);
        EXPECT_FLOAT_EQ(vector[1], -2.0f);
        EXPECT_FLOAT_EQ(vector[2], 8.0f);
        EXPECT_FLOAT_EQ(vector[3], 1.0f);
        EXPECT_TRUE(expanded::QuickDrawSphere());
    }

    for (int sphere : {static_cast<int>(ITEM_WEAPON_SMALL_SWORD), 0}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        float    vector[4] = {3.0f, -1.0f, 4.0f, 1.0f};
        expanded::MoonlitFocusShot(vector);
        EXPECT_FLOAT_EQ(vector[0], 3.0f);
    }

    // The sword in Toan's hand leaves pellets alone.
    Wielding tsukikage(ITEM_WEAPON_TSUKIKAGE);
    float    vector[4] = {3.0f, -1.0f, 4.0f, 1.0f};
    expanded::MoonlitFocusShot(vector);
    EXPECT_FLOAT_EQ(vector[0], 3.0f);
}

TEST(MoonlitBait, AtNightOwnedEveryBaitAFishLikesIsAFavourite) {
    TownClock night(kNight);
    {
        TestSave save;
        save.status().chara_weapons[CHARA_TOAN][3].item_no = ITEM_WEAPON_TSUKIKAGE;
        EXPECT_TRUE(expanded::MoonlitBaitActive());
        EXPECT_EQ(BonAffinities(), kMoonlit);
    }
    {
        TestSave save;
        save.stock().weapons[7].item_no = ITEM_WEAPON_TSUKIKAGE;
        EXPECT_EQ(BonAffinities(), kMoonlit);
    }
}

TEST(MoonlitBait, ByDayTheGamesAffinity) {
    TestSave save;
    save.status().chara_weapons[CHARA_TOAN][3].item_no = ITEM_WEAPON_TSUKIKAGE;
    for (float time : {kMorning, kDay, kEvening, 5.49f, 8.5f}) {
        TownClock clock(time);
        EXPECT_FALSE(expanded::MoonlitBaitActive()) << time;
        EXPECT_EQ(BonAffinities(), kRetail) << time;
    }
    for (float time : {5.5f, 8.49f}) {
        TownClock clock(time);
        EXPECT_TRUE(expanded::MoonlitBaitActive()) << time;
    }
}

TEST(MoonlitBait, NotOwnedTheGamesAffinity) {
    TownClock night(kNight);
    TestSave  save;
    EXPECT_EQ(BonAffinities(), kRetail);

    // Heaven's Cloud does not carry it.
    save.status().chara_weapons[CHARA_TOAN][0].item_no = ITEM_WEAPON_HEAVEN_S_CLOUD;
    EXPECT_FALSE(expanded::MoonlitBaitActive());
    EXPECT_EQ(BonAffinities(), kRetail);
}

TEST(MoonlitBait, OffWithTheWeaponAbilities) {
    TownClock   night(kNight);
    TestSave    save;
    OptionScope options;
    save.status().chara_weapons[CHARA_TOAN][3].item_no = ITEM_WEAPON_TSUKIKAGE;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(BonAffinities(), kRetail);
}

// CFish's rolls to notice the bait (0.3 times the affinity), nibble it and bite (0.5 times) roll at the raised
// affinity; a bait the fish does not like it never takes.
TEST(MoonlitBait, TheFishsRollsTakeIt) {
    TestSave save;
    save.status().chara_weapons[CHARA_TOAN][3].item_no = ITEM_WEAPON_TSUKIKAGE;
    auto  fish = std::make_unique<CFish>();
    float at[4] = {0.0f, -12.0f, 0.0f, 1.0f};
    fish->info = Bon();
    fish->fish_kind = expanded::fish::kBon;
    fish->SetPosition(at);

    constexpr unsigned kSeed = 7;
    constexpr int      kTries = 2000;
    auto               taken = [&](int (CFish::*roll)()) {
        std::srand(kSeed);
        int up = 0;
        for (int i = 0; i < kTries; i++) {
            up += ((*fish).*roll)();
        }
        return up;
    };

    fish->SetFoodPos(kMimi, at, 25.0f);
    for (float time : {kDay, kNight}) {
        TownClock   clock(time);
        const float affinity = time == kNight ? 1.0f : 0.5f;
        EXPECT_EQ(taken(&CFish::FindFood), Rolls(kSeed, kTries, affinity * 0.3f)) << time;
        EXPECT_EQ(taken(&CFish::EatFood), Rolls(kSeed, kTries, affinity * 0.5f)) << time;
        EXPECT_EQ(taken(&CFish::BiteHook), Rolls(kSeed, kTries, affinity * 0.5f)) << time;
    }
    EXPECT_GT(Rolls(kSeed, kTries, 0.5f), Rolls(kSeed, kTries, 0.25f));

    TownClock night(kNight);
    fish->SetFoodPos(kPoisonousApple, at, 25.0f);
    EXPECT_EQ(taken(&CFish::FindFood), 0);
    EXPECT_EQ(taken(&CFish::EatFood), 0);
    EXPECT_EQ(taken(&CFish::BiteHook), 0);
}
