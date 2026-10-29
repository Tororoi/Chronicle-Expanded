#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <iterator>
#include <memory>

#include "dngstatusdata.hpp"
#include "expanded/town/baits.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fishing.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "mainselect.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::TestSave;

constexpr int kFruitOfEden = 13, kCheese = 14; // their esa_type

// A fishing spot's six fish, out of debug mode (where every fish notices the bait from anywhere), the bait's model on
// hand for the hook and an empty bag.
class Baits : public ::testing::Test {
protected:
    void SetUp() override {
        Fish = fish_.get();
        AngleFish = nullptr;
        EsaFrame = nullptr;
        esa_type = -1;
        DebugMode = 0;
    }

    void TearDown() override {
        Fish = saved_fish_;
        AngleFish = saved_angle_;
        EsaFrame = saved_frame_;
        esa_type = saved_type_;
        DebugMode = saved_debug_;
    }

    bool InBag(int item) { return SaveData->GetDngStatus()->SearchItemIndexNo(item) >= 0; }

    // Slot's fish a kind loaded as FishingLoadFish loads it, at the hook with bait (esa_type) on it.
    CFish &AtTheHook(int slot, int kind, int bait) {
        CFish &fish = fish_[slot];
        fish.fish_kind = kind;
        fish.info = *GetFishInfo(kind);
        float at[4] = {0.0f, -12.0f, 0.0f, 1.0f};
        fish.SetPosition(at);
        fish.SetFoodPos(bait, at, 25.0f);
        return fish;
    }

    // Of 1000 rolls, how many the fish takes the bait in: notices, nibbles or bites it.
    static int Takes(CFish &fish) {
        std::srand(1);
        int takes = 0;
        for (int i = 0; i < 1000; i++) {
            takes += fish.FindFood() + fish.EatFood() + fish.BiteHook();
        }
        return takes;
    }

    TestSave                 save_;
    std::unique_ptr<CFish[]> fish_ = std::make_unique<CFish[]>(6);
    CFrameVu1                model_;
    CFish                   *saved_fish_ = Fish;
    CFish                   *saved_angle_ = AngleFish;
    CFrameVu1               *saved_frame_ = EsaFrame;
    int                      saved_type_ = esa_type;
    s32                      saved_debug_ = DebugMode;
};

} // namespace

TEST(ExtraBaits, TheyFollowTheGamesThirteen) {
    EXPECT_EQ(expanded::kFirstExtraBait, std::ssize(esa_info));
    EXPECT_EQ(expanded::ExtraBaitType(ITEM_FRUIT_OF_EDEN), kFruitOfEden);
    EXPECT_EQ(expanded::ExtraBaitType(ITEM_CHEESE), kCheese);
    EXPECT_EQ(expanded::ExtraBaitType(ITEM_EVY), -1);
    EXPECT_EQ(expanded::ExtraBaitType(ITEM_BREAD), -1);
    for (int type = 0; type < kFruitOfEden; type++) {
        EXPECT_EQ(&expanded::BaitInfo(type), &esa_info[type]) << type;
    }
    EXPECT_EQ(expanded::BaitInfo(kFruitOfEden).item_no, ITEM_FRUIT_OF_EDEN);
    EXPECT_EQ(expanded::BaitInfo(kCheese).item_no, ITEM_CHEESE);
}

TEST(ExtraBaits, ThePickerTakesThemFromTheBag) {
    EXPECT_TRUE(expanded::PickerTakesBait(ITEM_FRUIT_OF_EDEN));
    EXPECT_TRUE(expanded::PickerTakesBait(ITEM_CHEESE));
    EXPECT_FALSE(expanded::PickerTakesBait(ITEM_BREAD));
    EXPECT_FALSE(expanded::PickerTakesBait(ITEM_EVY)); // on the picker's own list
    EXPECT_FALSE(expanded::PickerTakesBait(-1));
}

// PAL retail's bait event lists eight of the game's baits after the picker's choice (fishing_bait_test.cpp).
TEST(ExtraBaits, TheTownsScriptsListThem) {
    for (int bait : {ITEM_FRUIT_OF_EDEN, ITEM_CHEESE}) {
        std::array<RS_STACKDATA, 10> variables{};
        for (RS_STACKDATA &variable : variables) {
            variable.type = RS_INT;
        }
        constexpr std::array listed = {ITEM_THROBBING_CHERRY, ITEM_GOOEY_PEACH, ITEM_BOMB_NUTS, ITEM_POISONOUS_APPLE,
                                       ITEM_MELLOW_BANANA, ITEM_EVY, ITEM_MIMI, ITEM_PRICKLY};
        for (std::size_t slot = 0; slot < listed.size(); slot++) {
            variables[1 + slot].i = listed[slot];
        }
        EXPECT_TRUE(ListBaitForScript(variables.data(), bait)) << bait;
        EXPECT_EQ(variables[1].i, bait);
    }
}

TEST_F(Baits, TheFruitOfEdenGoesOnTheHookAndBackToTheBagUnspent) {
    FishingLoadEsa(ITEM_FRUIT_OF_EDEN, &model_, 40);
    EXPECT_EQ(esa_type, kFruitOfEden);
    EXPECT_EQ(EsaFrame, &model_);
    EXPECT_EQ(FishingGetEsaItemNo(), ITEM_FRUIT_OF_EDEN);

    // Changed for Cheese: the fruit goes back to the bag.
    FishingLoadEsa(ITEM_CHEESE, &model_, 40);
    EXPECT_EQ(esa_type, kCheese);
    EXPECT_EQ(FishingGetEsaItemNo(), ITEM_CHEESE);
    EXPECT_TRUE(InBag(ITEM_FRUIT_OF_EDEN));
    EXPECT_FALSE(InBag(ITEM_CHEESE));

    // Fishing quit: the Cheese goes back too.
    FishingExit();
    EXPECT_TRUE(InBag(ITEM_CHEESE));
}

TEST_F(Baits, ABaitAFishTookIsSpent) {
    FishingLoadEsa(ITEM_FRUIT_OF_EDEN, &model_, 40);
    FishingDeleteEsa(); // as a fish takes it
    EXPECT_EQ(FishingGetEsaItemNo(), -1);
    FishingExit();
    EXPECT_FALSE(InBag(ITEM_FRUIT_OF_EDEN));
}

TEST_F(Baits, OnlyTheBaronGarayanLikesTheFruitOfEdenAndNoFishCheese) {
    for (int kind = 0; kind < expanded::fish::kKindCount; kind++) {
        const FishInfo &info = *GetFishInfo(kind);
        EXPECT_FLOAT_EQ(expanded::BaitAffinity(info, kFruitOfEden), kind == expanded::fish::kBaronGarayan ? 1.0f : 0.0f) << kind;
        EXPECT_FLOAT_EQ(expanded::BaitAffinity(info, kCheese), 0.0f) << kind;
        // The copy a loaded fish holds.
        CFish &fish = AtTheHook(kind % 6, kind, kFruitOfEden);
        EXPECT_FLOAT_EQ(expanded::BaitAffinity(fish.info, kFruitOfEden), kind == expanded::fish::kBaronGarayan ? 1.0f : 0.0f) << kind;
        // The game's baits as fish_info has them.
        for (int bait = 0; bait < kFruitOfEden; bait++) {
            EXPECT_FLOAT_EQ(expanded::BaitAffinity(info, bait), info.food_affinity[bait]) << kind << " " << bait;
        }
    }
}

TEST_F(Baits, NoBaitPastThemIsLiked) {
    const FishInfo &baron = *GetFishInfo(expanded::fish::kBaronGarayan);
    EXPECT_FLOAT_EQ(expanded::BaitAffinity(baron, -1), 0.0f);
    EXPECT_FLOAT_EQ(expanded::BaitAffinity(baron, kCheese + 1), 0.0f);
    // An info no fish holds has no kind.
    const FishInfo copy = baron;
    EXPECT_FLOAT_EQ(expanded::BaitAffinity(copy, kFruitOfEden), 0.0f);
}

TEST_F(Baits, TheBaronGarayanTakesTheFruitOfEdenAndNoFishTheCheese) {
    EXPECT_GT(Takes(AtTheHook(0, expanded::fish::kBaronGarayan, kFruitOfEden)), 0);
    for (int kind = 0; kind < expanded::fish::kKindCount; kind++) {
        if (kind != expanded::fish::kBaronGarayan) {
            EXPECT_EQ(Takes(AtTheHook(1, kind, kFruitOfEden)), 0) << kind;
        }
        EXPECT_EQ(Takes(AtTheHook(2, kind, kCheese)), 0) << kind;
    }
}

TEST_F(Baits, FishNoticeThemFrom25AsMostBaits) {
    EsaFrame = &model_;
    for (int bait : {kFruitOfEden, kCheese}) {
        esa_type = bait;
        FishingStepFish();
        EXPECT_EQ(fish_[5].food_kind, bait);
        EXPECT_FLOAT_EQ(fish_[5].food_radius, 25.0f);
    }

    // Bait Flamboyance, a Flamingo owned, as for the game's baits.
    save_.status().chara_weapons[CHARA_XIAO][3].item_no = ITEM_WEAPON_FLAMINGO;
    esa_type = kFruitOfEden;
    FishingStepFish();
    EXPECT_FLOAT_EQ(fish_[5].food_radius, 25.0f + expanded::kBaitFlamboyance);
}
