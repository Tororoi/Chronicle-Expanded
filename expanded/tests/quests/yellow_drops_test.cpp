// gtest before the game headers, whose one-letter macros it cannot stand.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "expanded/quests/yellow_drops.hpp"
#include "itemdata.hpp"
#include "quests/quest_test.hpp"

namespace expanded::test {

namespace {

const Giver kLedan = {.map_no = kYellowDrops, .npc_id = 0, .name = {}};
const Giver kSamba = {.map_no = kYellowDrops, .npc_id = 1, .name = {}};

class YellowDropsQuests : public QuestTest {
protected:
    void SetUp() override {
        QuestTest::SetUp();
        RegisterYellowDropsQuests();
        game.party = 6;
    }
};

TEST_F(YellowDropsQuests, NothingUntilTheBossHasJoined) {
    game.party = 5;
    EXPECT_FALSE(Offered(kLedan, "Got anything for me?"));
    EXPECT_FALSE(Offered(kSamba, "Got anything for me?"));
    game.party = 6;
    EXPECT_TRUE(Offered(kLedan, "Got anything for me?"));
    EXPECT_TRUE(Offered(kSamba, "Got anything for me?"));
}

TEST_F(YellowDropsQuests, ThePuzzleWantsTheFourInOrder) {
    const std::string said = Ask(kLedan, "Got anything for me?");
    EXPECT_NE(said.find("valuable crystal"), std::string::npos) << said;
    EXPECT_TRUE(PuzzleOn());

    game.pack = {ITEM_ESCAPE_POWDER, ITEM_GOOEY_PEACH, ITEM_STAMINA_DRINK, ITEM_BOMB};
    Ask(kLedan, "About the quest.");
    EXPECT_TRUE(PuzzleOn());
    EXPECT_EQ(game.ItemCount(ITEM_MAGICAL_CRYSTAL), 0);

    game.pack = {ITEM_GOOEY_PEACH, ITEM_ESCAPE_POWDER, ITEM_STAMINA_DRINK, ITEM_BOMB, ITEM_BREAD};
    const std::string solved = Ask(kLedan, "About the quest.");
    EXPECT_NE(solved.find("You solved the puzzle"), std::string::npos) << solved;
    EXPECT_EQ(game.ItemCount(ITEM_MAGICAL_CRYSTAL), 1);
    EXPECT_FALSE(PuzzleOn());
    EXPECT_FALSE(Offered(kLedan, "About the quest."));
    EXPECT_FALSE(Offered(kLedan, "Got anything for me?"));
}

TEST_F(YellowDropsQuests, ThePuzzlesItemsAreTheOnesBlueTerraNeeded) {
    EXPECT_EQ(kPuzzleItems, (std::array<int, 4>{ITEM_GOOEY_PEACH, ITEM_ESCAPE_POWDER, ITEM_STAMINA_DRINK, ITEM_BOMB}));
}

TEST_F(YellowDropsQuests, TheChallengeIsTakenThenRewardedOnceCleared) {
    const std::string said = Ask(kSamba, "Got anything for me?");
    EXPECT_NE(said.find("true strength"), std::string::npos) << said;
    EXPECT_TRUE(ChallengeOn());

    Ask(kSamba, "About the quest.");
    EXPECT_EQ(game.ItemCount(ITEM_MAP), 0);

    dc::quest::Act("yd_map", [](dc::quest::Context &ctx) { ctx.SetStage("cleared"); });
    EXPECT_TRUE(ChallengeCleared());
    const std::string reward = Ask(kSamba, "About the quest.");
    EXPECT_NE(reward.find("Extraordinary"), std::string::npos) << reward;
    EXPECT_EQ(game.ItemCount(ITEM_MAP), 1);
    EXPECT_FALSE(Offered(kSamba, "About the quest."));
}

TEST_F(YellowDropsQuests, AFullPackKeepsTheRewardForLater) {
    Ask(kLedan, "Got anything for me?");
    game.pack = {ITEM_GOOEY_PEACH, ITEM_ESCAPE_POWDER, ITEM_STAMINA_DRINK, ITEM_BOMB};
    game.pack_room = 0;
    Ask(kLedan, "About the quest.");
    EXPECT_TRUE(PuzzleOn());
    EXPECT_EQ(game.ItemCount(ITEM_MAGICAL_CRYSTAL), 0);
}

} // namespace

} // namespace expanded::test
