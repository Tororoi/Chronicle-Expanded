#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <memory>
#include <vector>

#include "character.hpp"
#include "editloop.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fish_steering.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/ruby/fairys_ring.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/mardan.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestSave;

constexpr int   kSlots = 6;
constexpr float kUnturned = 99.0f;

// esa_info's indexes.
constexpr int kPoisonousApple = 6;
constexpr int kCarrot = 8;
constexpr int kPotatoCake = 9;

// The fish of the tests' spot, slot by slot (-1 empty), with the game's liking for each bait: Mardan Garayan for the
// Poisonous Apple, Baron Garayan half as much for the Potato Cake, Umadakara for the Carrot, Negie for the Potato Cake
// (and the fruits a little).
constexpr int kKinds[kSlots] = {expanded::fish::kMardanGarayan, expanded::fish::kBaronGarayan, expanded::fish::kUmadakara, expanded::fish::kNegie, -1, expanded::fish::kNegie};

FishInfo Info(int kind) {
    FishInfo info = {.model_size = 21.0f, .min_size = 8.0f, .max_size = 16.0f, .min_fp = 15, .max_fp = 40, .food_affinity = {}};
    if (kind == expanded::fish::kMardanGarayan) {
        info.food_affinity[kPoisonousApple] = 1.0f;
    } else if (kind == expanded::fish::kBaronGarayan) {
        info.food_affinity[kPotatoCake] = 0.5f;
    } else if (kind == expanded::fish::kUmadakara) {
        info.food_affinity[kCarrot] = 1.0f;
    } else if (kind == expanded::fish::kNegie) {
        info.food_affinity[kPotatoCake] = 1.0f;
        info.food_affinity[3] = info.food_affinity[4] = info.food_affinity[5] = info.food_affinity[7] = 0.2f;
    }
    return info;
}

class FishSteering : public ::testing::Test {
protected:
    void SetUp() override {
        Fish = fish_.get();
        Chara = &player_;
        esa_type = -1;
        player_.SetPosition(100.0f, 0.0f, 0.0f);
        for (int i = 0; i < kSlots; i++) {
            fish_[i].fish_kind = kKinds[i];
            fish_[i].info = Info(kKinds[i]);
            float position[4] = {0.0f, -12.0f, 10.0f * static_cast<float>(i), 1.0f};
            fish_[i].SetPosition(position);
        }
        Unturn();
    }

    void TearDown() override {
        Fish = saved_fish_;
        Chara = saved_chara_;
        esa_type = saved_esa_;
    }

    void Own(int sword) { save_.status().chara_weapons[CHARA_TOAN][2].item_no = static_cast<s16>(sword); }

    void Unturn() {
        for (int i = 0; i < kSlots; i++) {
            fish_[i].target_yaw = kUnturned;
        }
    }

    void Step(int frames) {
        for (int i = 0; i < frames; i++) {
            expanded::FishStep();
        }
    }

    // Each slot, whether it has turned toward the player since Unturn; then Unturn.
    std::vector<bool> Turned() {
        std::vector<bool> turned;
        for (int i = 0; i < kSlots; i++) {
            if (fish_[i].target_yaw == kUnturned) {
                turned.push_back(false);
                continue;
            }
            EXPECT_FLOAT_EQ(fish_[i].target_yaw, atan2f(100.0f, -10.0f * static_cast<float>(i))) << i;
            turned.push_back(true);
        }
        Unturn();
        return turned;
    }

    // The frame of steps that turn a slot, over frames steps.
    std::vector<int> TurnsOf(int slot, int frames) {
        std::vector<int> turns;
        for (int frame = 0; frame < frames; frame++) {
            Step(1);
            if (Turned()[slot]) {
                turns.push_back(frame);
            }
        }
        return turns;
    }

    TestSave                 save_;
    std::unique_ptr<CFish[]> fish_ = std::make_unique<CFish[]>(kSlots);
    CCharacter               player_;
    CFish                   *saved_fish_ = Fish;
    CCharacter              *saved_chara_ = Chara;
    int                      saved_esa_ = esa_type;
};

const std::vector<bool> kNone(kSlots, false);
const std::vector<bool> kEveryFish = {true, true, true, true, false, true};

std::vector<bool> Only(int slot) {
    std::vector<bool> turned(kSlots, false);
    turned[slot] = true;
    return turned;
}

} // namespace

TEST_F(FishSteering, TheWaterfallAndEastHarborTurnEveryFishEvery600Frames) {
    for (int spot : {expanded::kMatatakiWaterfall, expanded::kEastHarbor}) {
        expanded::FishSpotLoaded(spot);
        Step(1);
        EXPECT_EQ(Turned(), kEveryFish) << spot;
        Step(expanded::kSteerFrames - 1);
        EXPECT_EQ(Turned(), kNone) << spot;
        Step(1);
        EXPECT_EQ(Turned(), kEveryFish) << spot;
        Step(expanded::kSteerFrames - 1);
        EXPECT_EQ(Turned(), kNone) << spot;
    }
}

TEST_F(FishSteering, OtherSpotsLeaveTheFishAlone) {
    esa_type = kPotatoCake;
    for (int spot : {0, 1, 4, 5}) {
        expanded::FishSpotLoaded(spot);
        Step(2 * expanded::kSteerFrames);
        EXPECT_EQ(Turned(), kNone) << spot;
    }
}

TEST_F(FishSteering, AFishAtThePlayerKeepsItsCourse) {
    expanded::FishSpotLoaded(expanded::kMatatakiWaterfall);
    float at_player[4] = {100.0f, -12.0f, 0.0f, 1.0f};
    fish_[0].SetPosition(at_player);
    Step(1);
    EXPECT_EQ(fish_[0].target_yaw, kUnturned);
}

TEST_F(FishSteering, AMardanSwordTurnsItsFishByTheirLikingForTheBait) {
    esa_type = kPoisonousApple;
    for (int i = 0; i < 3; i++) {
        const int sword = ITEM_WEAPON_MARDAN_EINS + i;
        const int every = expanded::kMardanSteerFrames[i];
        Own(sword);
        expanded::FishSpotLoaded(0);
        // The Mardan Garayan likes the apple: turned on the first frame, then every 240, 180 or 120 frames.
        EXPECT_EQ(TurnsOf(0, 2 * every + 1), (std::vector<int>{0, every, 2 * every})) << sword;
    }

    // The Baron Garayan likes the Potato Cake half as much: half as often. The Negie likes it as well, but is not a
    // Mardan sword's fish.
    Own(ITEM_WEAPON_MARDAN_EINS);
    esa_type = kPotatoCake;
    expanded::FishSpotLoaded(0);
    Step(1);
    EXPECT_EQ(Turned(), Only(1));
    Step(479);
    EXPECT_EQ(Turned(), kNone);
    Step(1);
    EXPECT_EQ(Turned(), Only(1));

    // The Umadakara likes the Carrot.
    esa_type = kCarrot;
    expanded::FishSpotLoaded(0);
    EXPECT_EQ(TurnsOf(2, 481), (std::vector<int>{0, 240, 480}));
}

TEST_F(FishSteering, TheBaitLastOnTheHookCountsAfterASwitch) {
    Own(ITEM_WEAPON_MARDAN_EINS);
    expanded::FishSpotLoaded(0);
    esa_type = kCarrot;
    Step(1);
    EXPECT_EQ(Turned(), Only(2));

    // Switched to the apple: the Mardan Garayan turns at once, the Umadakara no more.
    esa_type = kPoisonousApple;
    Step(1);
    EXPECT_EQ(Turned(), Only(0));
    Step(200);
    EXPECT_EQ(Turned(), kNone);

    // Eaten off the hook: the apple still counts.
    esa_type = -1;
    Step(39);
    EXPECT_EQ(Turned(), kNone);
    Step(1);
    EXPECT_EQ(Turned(), Only(0));
    Step(239);
    EXPECT_EQ(Turned(), kNone);
    Step(1);
    EXPECT_EQ(Turned(), Only(0));
}

TEST_F(FishSteering, NoBaitYetOrNoSwordTurnsNothing) {
    Own(ITEM_WEAPON_ARISE_MARDAN);
    expanded::FishSpotLoaded(0);
    Step(1000);
    EXPECT_EQ(Turned(), kNone);

    Own(0);
    esa_type = kPoisonousApple;
    expanded::FishSpotLoaded(0);
    Step(1000);
    EXPECT_EQ(Turned(), kNone);
}

TEST_F(FishSteering, TheSwordOwnedAsTheFishLoadCounts) {
    esa_type = kPoisonousApple;
    expanded::FishSpotLoaded(0);
    Own(ITEM_WEAPON_ARISE_MARDAN);
    Step(500);
    EXPECT_EQ(Turned(), kNone);
    expanded::FishSpotLoaded(0);
    Step(1);
    EXPECT_EQ(Turned(), Only(0));
}

TEST_F(FishSteering, WithoutWeaponAbilitiesOnlyTheSpotsTurnFish) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    Own(ITEM_WEAPON_ARISE_MARDAN);
    esa_type = kPoisonousApple;
    expanded::FishSpotLoaded(expanded::kEastHarbor);
    Step(1);
    EXPECT_EQ(Turned(), kEveryFish);
    Step(expanded::kSteerFrames - 1);
    EXPECT_EQ(Turned(), kNone);
}

TEST(MardanSteerFrames, TheSwordsPaceOverTheFishsLiking) {
    EXPECT_EQ(expanded::MardanSteerFrames(ITEM_WEAPON_MARDAN_EINS, expanded::fish::kMardanGarayan, 1.0f), 240);
    EXPECT_EQ(expanded::MardanSteerFrames(ITEM_WEAPON_MARDAN_TWEI, expanded::fish::kBaronGarayan, 0.5f), 360);
    EXPECT_EQ(expanded::MardanSteerFrames(ITEM_WEAPON_ARISE_MARDAN, expanded::fish::kUmadakara, 1.0f), 120);
    EXPECT_EQ(expanded::MardanSteerFrames(ITEM_WEAPON_ARISE_MARDAN, expanded::fish::kUmadakara, 0.2f), 600);
    EXPECT_EQ(expanded::MardanSteerFrames(ITEM_WEAPON_ARISE_MARDAN, expanded::fish::kNegie, 1.0f), 0);
    EXPECT_EQ(expanded::MardanSteerFrames(ITEM_WEAPON_MARDAN_EINS, expanded::fish::kMardanGarayan, 0.0f), 0);
    EXPECT_EQ(expanded::MardanSteerFrames(0, expanded::fish::kMardanGarayan, 1.0f), 0);
    EXPECT_EQ(expanded::MardanSteerFrames(ITEM_WEAPON_BASELARD, expanded::fish::kMardanGarayan, 1.0f), 0);
}
