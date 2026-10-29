#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <vector>

#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fishing.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "shop.hpp"

namespace {

constexpr int kFishKinds = 18;
constexpr int kSpots = 5;
constexpr int kEvy = 0; // esa_info's index of the Evy

// The kinds FishingLoadFish draws for each spot (Norune, Peanut Pond, the Matataki waterfall, East Harbor, Muska Lacka).
const std::array<std::vector<int>, kSpots> kRetailPools = {
    {{1, 2, 6, 7}, {1, 4, 9, 10}, {5, 17, 2, 4, 6}, {0, 3, 11, 12, 13}, {5, 17, 14, 15, 16}}
};

std::vector<int> Pool(int spot) {
    std::vector<int> pool;
    for (int kind : kRetailPools[spot]) {
        pool.push_back(expanded::PoolFish(spot, kind));
    }
    return pool;
}

// The info LoadFish gives a fish of kind.
FishInfo Loaded(int kind) {
    FishInfo info = *GetFishInfo(kind);
    expanded::FishInfoLoaded(info, kind);
    return info;
}

// The bites in tries of BiteHook's roll for a fish of kind with bait (esa_info's index) at its mouth.
int Bites(int kind, int bait, int tries) {
    CFish fish;
    fish.info = Loaded(kind);
    fish.fish_kind = kind;
    float mouth[4] = {};
    fish.SetPosition(mouth);
    fish.SetFoodPos(bait, mouth, esa_info[bait].radius);
    int bites = 0;
    for (int i = 0; i < tries; i++) {
        bites += fish.BiteHook();
    }
    return bites;
}

} // namespace

TEST(FishPools, TheKindsAreTheirFish) {
    // GetFishMsgNo's system messages: 41 "Gobbler", 46 "Gummy", 47 "Niler", 31 "Piccoly".
    EXPECT_EQ(GetFishMsgNo(expanded::fish::kGobbler), 41);
    EXPECT_EQ(GetFishMsgNo(expanded::fish::kGummy), 46);
    EXPECT_EQ(GetFishMsgNo(expanded::fish::kNiler), 47);
    EXPECT_EQ(GetFishMsgNo(expanded::fish::kPiccoly), 31);
}

TEST(FishPools, TheMatatakiWaterfallHasNilersForGummies) {
    EXPECT_EQ(Pool(expanded::kMatatakiWaterfall), (std::vector<int>{5, 17, 2, 4, 7}));
}

TEST(FishPools, EastHarborHasGobblersForPiccolies) {
    EXPECT_EQ(Pool(expanded::kEastHarbor), (std::vector<int>{0, 3, 1, 12, 13}));
}

TEST(FishPools, EveryOtherFishIsTheGames) {
    for (int spot = 0; spot < kSpots + 3; spot++) {
        for (int kind = -1; kind < kFishKinds; kind++) {
            const bool traded = (spot == expanded::kMatatakiWaterfall && kind == expanded::fish::kGummy) ||
                                (spot == expanded::kEastHarbor && kind == expanded::fish::kPiccoly);
            if (!traded) {
                EXPECT_EQ(expanded::PoolFish(spot, kind), kind) << spot << " " << kind;
            }
        }
    }
    // Norune keeps its Gummy.
    EXPECT_EQ(Pool(0), kRetailPools[0]);
}

TEST(FishPools, APiccolyLikesOnlyTheMellowBanana) {
    ASSERT_EQ(esa_info[kEvy].item_no, ITEM_EVY);
    ASSERT_EQ(esa_info[expanded::kMellowBanana].item_no, ITEM_MELLOW_BANANA);
    const FishInfo piccoly = Loaded(expanded::fish::kPiccoly);
    for (int bait = 0; bait < FISH_FOOD_KIND_COUNT; bait++) {
        EXPECT_EQ(piccoly.food_affinity[bait], bait == expanded::kMellowBanana ? 1.0f : 0.0f) << bait;
    }
    // In the game's fish_info it takes most baits, the Evy best.
    EXPECT_EQ(GetFishInfo(expanded::fish::kPiccoly)->food_affinity[kEvy], 1.0f);
    EXPECT_EQ(GetFishInfo(expanded::fish::kPiccoly)->food_affinity[expanded::kMellowBanana], 0.2f);
}

TEST(FishPools, APiccolyKeepsItsSizesAndPoints) {
    const FishInfo  piccoly = Loaded(expanded::fish::kPiccoly);
    const FishInfo &game = *GetFishInfo(expanded::fish::kPiccoly);
    EXPECT_EQ(piccoly.model_size, game.model_size);
    EXPECT_EQ(piccoly.min_size, game.min_size);
    EXPECT_EQ(piccoly.max_size, game.max_size);
    EXPECT_EQ(piccoly.min_fp, game.min_fp);
    EXPECT_EQ(piccoly.max_fp, game.max_fp);
}

TEST(FishPools, EveryOtherFishKeepsItsBaits) {
    for (int kind = 0; kind < kFishKinds; kind++) {
        if (kind == expanded::fish::kPiccoly) {
            continue;
        }
        const FishInfo  loaded = Loaded(kind);
        const FishInfo &game = *GetFishInfo(kind);
        for (int bait = 0; bait < FISH_FOOD_KIND_COUNT; bait++) {
            EXPECT_EQ(loaded.food_affinity[bait], game.food_affinity[bait]) << kind << " " << bait;
        }
    }
}

TEST(FishPools, APiccolyBitesOnlyTheMellowBanana) {
    std::srand(1);
    for (int bait = 0; bait < FISH_FOOD_KIND_COUNT; bait++) {
        int bites = Bites(expanded::fish::kPiccoly, bait, 1000);
        if (bait == expanded::kMellowBanana) {
            EXPECT_NEAR(bites, 500, 60); // BiteHook's half of its affinity
        } else {
            EXPECT_EQ(bites, 0) << bait;
        }
    }
}
