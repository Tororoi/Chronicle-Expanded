#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "expanded/town/fishing.hpp"
#include "shop.hpp"

namespace {

constexpr int kFishKinds = 18;
constexpr int kSpots = 5;

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

} // namespace

TEST(FishPools, TheKindsAreTheirFish) {
    // GetFishMsgNo's system messages: 41 "Gobbler", 46 "Gummy", 47 "Niler", 31 "Piccoly".
    EXPECT_EQ(GetFishMsgNo(expanded::kGobbler), 41);
    EXPECT_EQ(GetFishMsgNo(expanded::kGummy), 46);
    EXPECT_EQ(GetFishMsgNo(expanded::kNiler), 47);
    EXPECT_EQ(GetFishMsgNo(expanded::kPiccoly), 31);
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
            const bool traded = (spot == expanded::kMatatakiWaterfall && kind == expanded::kGummy) ||
                                (spot == expanded::kEastHarbor && kind == expanded::kPiccoly);
            if (!traded) {
                EXPECT_EQ(expanded::PoolFish(spot, kind), kind) << spot << " " << kind;
            }
        }
    }
    // Norune keeps its Gummy.
    EXPECT_EQ(Pool(0), kRetailPools[0]);
}
