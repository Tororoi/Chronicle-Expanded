#include <string>

#include "expanded/data/fishing_spots.hpp"
#include "expanded/quests/fishing_quests.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fishing.hpp"
#include "quests/quest_test.hpp"

namespace expanded::test {

namespace {

const FishingGiver &Pike() {
    return FishingGivers()[0];
}

const FishingGiver &Pao() {
    return FishingGivers()[1];
}

const FishingGiver &Sam() {
    return FishingGivers()[2];
}

class FishingQuests : public QuestTest {
protected:
    void SetUp() override {
        QuestTest::SetUp();
        RegisterFishingQuests();
    }

    // Sets the giver's quest under way: a count quest for kind at place, or a size quest.
    static void Count(const FishingGiver &giver, int kind, FishPlace place, int count) {
        dc::quest::Act(giver.quest, [&](dc::quest::Context &ctx) {
            ctx.SetStage("active");
            ctx.SetVar("size", 0);
            ctx.SetVar("fish", kind);
            ctx.SetVar("left", count);
            ctx.SetVar("total", count);
            ctx.SetVar("place", static_cast<int>(place));
        });
    }

    static void Size(const FishingGiver &giver, int least, int most) {
        dc::quest::Act(giver.quest, [&](dc::quest::Context &ctx) {
            ctx.SetStage("active");
            ctx.SetVar("size", 1);
            ctx.SetVar("least", least);
            ctx.SetVar("most", most);
            ctx.SetVar("place", static_cast<int>(giver.waters));
        });
    }
};

TEST_F(FishingQuests, EachGiverSetsACountOrASizeQuestInTheirWaters) {
    int counts = 0;
    int sizes = 0;
    for (int i = 0; i < 40; ++i) {
        dc::quest::Act(Pike().quest, [](dc::quest::Context &ctx) { ctx.SetStage("idle"); });
        Ask(Pike().giver, "Got anything for me?");
        ASSERT_EQ(Stage(Pike().quest), "active");
        if (Var(Pike().quest, "size") != 0) {
            ++sizes;
            EXPECT_GE(Var(Pike().quest, "least"), Pike().size_least);
            EXPECT_LE(Var(Pike().quest, "least"), Pike().size_most);
            EXPECT_EQ(Var(Pike().quest, "most"), Var(Pike().quest, "least") + Pike().size_width);
        } else {
            ++counts;
            const int fish = Var(Pike().quest, "fish");
            EXPECT_TRUE(fish == fish::kNiler || fish == fish::kGummy || fish == fish::kNonky || fish == fish::kGobbler)
                << fish;
            EXPECT_EQ(Var(Pike().quest, "left"), kFishCount);
        }
        EXPECT_EQ(Var(Pike().quest, "place"), static_cast<int>(FishPlace::NORUNE));
    }
    EXPECT_GT(counts, 0);
    EXPECT_GT(sizes, 0);
}

TEST_F(FishingQuests, ThePikeIntroComesFirst) {
    const std::string said = Ask(Pike().giver, "Got anything for me?");
    EXPECT_NE(said.find("guess what I have in store"), std::string::npos) << said;
}

TEST_F(FishingQuests, ACountQuestCountsItsKindInItsWatersOnly) {
    Count(Pike(), fish::kNonky, FishPlace::NORUNE, 2);
    FishingQuestCatch(fish::kGummy, 50, kNoruneArea);
    FishingQuestCatch(fish::kNonky, 50, kPeanutPond);
    EXPECT_EQ(Var(Pike().quest, "left"), 2);
    FishingQuestCatch(fish::kNonky, 50, kNoruneArea);
    EXPECT_EQ(Var(Pike().quest, "left"), 1);
    FishingQuestCatch(fish::kNonky, 50, kNoruneArea);
    EXPECT_EQ(Stage(Pike().quest), "complete");
}

TEST_F(FishingQuests, ASizeQuestWantsOneFishInItsWindow) {
    Size(Pike(), 100, 105);
    FishingQuestCatch(fish::kNonky, 99, kNoruneArea);
    FishingQuestCatch(fish::kNonky, 106, kNoruneArea);
    EXPECT_EQ(Stage(Pike().quest), "active");
    FishingQuestCatch(fish::kGummy, 105, kNoruneArea);
    EXPECT_EQ(Stage(Pike().quest), "complete");
}

TEST_F(FishingQuests, PaosMatatakiQuestsTakeEitherWater) {
    Count(Pao(), fish::kBakuBaku, FishPlace::MATATAKI, 2);
    FishingQuestCatch(fish::kBakuBaku, 50, kPeanutPond);
    FishingQuestCatch(fish::kBakuBaku, 50, kMatatakiWaterfall);
    EXPECT_EQ(Stage(Pao().quest), "complete");

    Count(Pao(), fish::kNonky, FishPlace::WATERFALL, 2);
    FishingQuestCatch(fish::kNonky, 50, kPeanutPond);
    EXPECT_EQ(Var(Pao().quest, "left"), 2);
}

TEST_F(FishingQuests, AGarayanIsAskedForAlone) {
    for (int i = 0; i < 200; ++i) {
        dc::quest::Act(Pao().quest, [](dc::quest::Context &ctx) { ctx.SetStage("idle"); });
        Ask(Pao().giver, "Got anything for me?");
        const int fish = Var(Pao().quest, "fish");
        if (Var(Pao().quest, "size") == 0 && (fish == fish::kMardanGarayan || fish == fish::kBaronGarayan)) {
            EXPECT_EQ(Var(Pao().quest, "left"), 1);
            EXPECT_EQ(Var(Pao().quest, "place"), static_cast<int>(FishPlace::WATERFALL));
            return;
        }
    }
    ADD_FAILURE() << "no Garayan in 200 quests";
}

TEST_F(FishingQuests, TheRewardIsFishingPoints) {
    Count(Pike(), fish::kNonky, FishPlace::NORUNE, 2);
    FishingQuestCatch(fish::kNonky, 50, kNoruneArea);
    FishingQuestCatch(fish::kNonky, 50, kNoruneArea);
    const std::string said = Ask(Pike().giver, "About the quest.");
    EXPECT_GE(game.fishing_points, Pike().points_least * 2);
    EXPECT_LE(game.fishing_points, Pike().points_most * 2);
    EXPECT_NE(said.find("Nicely done"), std::string::npos) << said;
    EXPECT_EQ(Stage(Pike().quest), "idle");
}

TEST_F(FishingQuests, ASizeQuestPaysUpToItsMostAndTheWatersBonus) {
    Size(Sam(), 120, 130);
    FishingQuestCatch(fish::kBobo, 125, kEastHarbor);
    Ask(Sam().giver, "About the quest.");
    EXPECT_GE(game.fishing_points, 130 - kSizeReward + Sam().size_bonus);
    EXPECT_LT(game.fishing_points, 130 + Sam().size_bonus);
}

TEST_F(FishingQuests, SamsThirdQuestShowsEastHarborsFish) {
    EXPECT_FALSE(EastHarborFishShown());
    for (int quest = 1; quest <= kSamSpecial; ++quest) {
        Size(Sam(), 120, 130);
        FishingQuestCatch(fish::kBobo, 125, kEastHarbor);
        const std::string said = Ask(Sam().giver, "About the quest.");
        if (quest < kSamSpecial) {
            EXPECT_NE(said.find("Nicely done"), std::string::npos) << said;
            EXPECT_FALSE(EastHarborFishShown());
        } else {
            EXPECT_NE(said.find("three quests"), std::string::npos) << said;
        }
    }
    EXPECT_TRUE(EastHarborFishShown());
}

TEST_F(FishingQuests, ProgressSaysWhatIsLeft) {
    Count(Pike(), fish::kGummy, FishPlace::NORUNE, 2);
    FishingQuestCatch(fish::kGummy, 50, kNoruneArea);
    const std::string said = Ask(Pike().giver, "About the quest.");
    EXPECT_NE(said.find("Gummies"), std::string::npos) << said;
    EXPECT_NE(said.find("Just 1 left"), std::string::npos) << said;
}

} // namespace

} // namespace expanded::test
