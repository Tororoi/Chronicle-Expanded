#include <string>

#include "expanded/quests/monster_slayer.hpp"
#include "itemdata.hpp"
#include "quests/quest_test.hpp"

namespace expanded::test {

namespace {

const SlayerGiver &Macho() {
    return SlayerGivers()[0];
}

const SlayerGiver &Gob() {
    return SlayerGivers()[1];
}

class MonsterSlayer : public QuestTest {
protected:
    void SetUp() override {
        QuestTest::SetUp();
        RegisterMonsterSlayer();
        game.floors[0] = 3;
    }

    // Kills of the quest's own target until it is done.
    void Finish(const SlayerGiver &giver) {
        const int  dungeon = Var(giver.quest, "dungeon");
        const auto monster = static_cast<Monster>(Var(giver.quest, "monster"));
        for (int left = Var(giver.quest, "left"); left > 0; --left) {
            MonsterSlayerKill(dungeon, monster);
        }
    }
};

TEST_F(MonsterSlayer, EachGiverOffersAQuestUntilOneIsUnderWay) {
    for (const SlayerGiver &giver : SlayerGivers()) {
        EXPECT_TRUE(Offered(giver.giver, "Got anything for me?")) << giver.quest;
        EXPECT_FALSE(Offered(giver.giver, "About the quest.")) << giver.quest;
    }
    Ask(Macho().giver, "Got anything for me?");
    EXPECT_FALSE(Offered(Macho().giver, "Got anything for me?"));
    EXPECT_TRUE(Offered(Macho().giver, "About the quest."));
}

TEST_F(MonsterSlayer, MachosFirstWordTellsOfTheQuestsAndSetsDashersInTheCave) {
    game.floors[1] = 5; // the Wise Owl Forest reached too: Macho's first is Dashers all the same
    const std::string said = Ask(Macho().giver, "Got anything for me?");
    EXPECT_NE(said.find("Prove your true strength"), std::string::npos) << said;
    EXPECT_EQ(Stage(Macho().quest), "active");
    EXPECT_EQ(Var(Macho().quest, "dungeon"), 0);
    EXPECT_EQ(Var(Macho().quest, "monster"), No(Monster::DASHER));
    const int left = Var(Macho().quest, "left");
    EXPECT_GE(left, kSlayerLeast);
    EXPECT_LE(left, kSlayerMost);
    EXPECT_EQ(Var(Macho().quest, "total"), left);
}

TEST_F(MonsterSlayer, OnlyTheTargetInItsDungeonCounts) {
    Ask(Macho().giver, "Got anything for me?");
    const int left = Var(Macho().quest, "left");
    MonsterSlayerKill(0, Monster::MASTER_JACKET);
    MonsterSlayerKill(1, Monster::DASHER);
    EXPECT_EQ(Var(Macho().quest, "left"), left);
    MonsterSlayerKill(0, Monster::DASHER);
    EXPECT_EQ(Var(Macho().quest, "left"), left - 1);
}

TEST_F(MonsterSlayer, AStrongerCopyCountsAsItsKind) {
    dc::quest::Act(Macho().quest, [](dc::quest::Context &ctx) {
        ctx.SetStage("active");
        ctx.SetVar("dungeon", 0);
        ctx.SetVar("monster", No(Monster::MASTER_JACKET));
        ctx.SetVar("left", 2);
    });
    MonsterSlayerKill(0, Monster::MASTER_JACKET_ENHANCED);
    EXPECT_EQ(Var(Macho().quest, "left"), 1);
}

TEST_F(MonsterSlayer, TheLastKillCompletesItAndTellsSo) {
    Ask(Macho().giver, "Got anything for me?");
    Finish(Macho());
    EXPECT_EQ(Stage(Macho().quest), "complete");
    EXPECT_EQ(QuestNoticeShown(), "Macho's quest is complete. Well done!");
    EXPECT_TRUE(Offered(Macho().giver, "About the quest."));
}

TEST_F(MonsterSlayer, TheRewardIsAPowerupPowderAndTheNextQuestFollows) {
    Ask(Macho().giver, "Got anything for me?");
    Finish(Macho());
    const std::string said = Ask(Macho().giver, "About the quest.");
    EXPECT_NE(said.find("Well done"), std::string::npos) << said;
    EXPECT_EQ(game.ItemCount(ITEM_POWERUP_POWDER), 1);
    EXPECT_EQ(game.items_shown, std::vector<int>{ITEM_POWERUP_POWDER});
    EXPECT_EQ(Stage(Macho().quest), "idle");

    // The next is any kind of a dungeon reached; with only the cave reached, one of its four.
    const std::string next = Ask(Macho().giver, "Got anything for me?");
    EXPECT_EQ(next.find("Prove your true strength"), std::string::npos) << next;
    EXPECT_EQ(Stage(Macho().quest), "active");
    EXPECT_EQ(Var(Macho().quest, "dungeon"), 0);
}

TEST_F(MonsterSlayer, AFullPackKeepsTheRewardForLater) {
    Ask(Macho().giver, "Got anything for me?");
    Finish(Macho());
    game.pack_room = 0;
    Ask(Macho().giver, "About the quest.");
    EXPECT_EQ(Stage(Macho().quest), "complete");
    EXPECT_EQ(game.ItemCount(ITEM_POWERUP_POWDER), 0);
}

TEST_F(MonsterSlayer, GiversDoNotSetAKindAlreadyWanted) {
    // Gob's quests roll among the cave's four; with Macho on Dashers, Gob never sets them.
    Ask(Macho().giver, "Got anything for me?");
    for (int i = 0; i < 40; ++i) {
        dc::quest::Act(Gob().quest, [](dc::quest::Context &ctx) { ctx.SetStage("idle"); });
        Ask(Gob().giver, "Got anything for me?");
        EXPECT_NE(Var(Gob().quest, "monster"), No(Monster::DASHER));
    }
}

TEST_F(MonsterSlayer, ProgressSaysWhatIsLeft) {
    Ask(Macho().giver, "Got anything for me?");
    MonsterSlayerKill(0, Monster::DASHER);
    const std::string said = Ask(Macho().giver, "About the quest.");
    EXPECT_NE(said.find("Dashers"), std::string::npos) << said;
    EXPECT_NE(said.find("Just " + std::to_string(Var(Macho().quest, "left")) + " left"), std::string::npos) << said;
}

TEST_F(MonsterSlayer, OtherVillagersAreLeftAlone) {
    EXPECT_FALSE(Offered({.map_no = kNorune, .npc_id = 7, .name = {}}, "Got anything for me?"));
}

} // namespace

} // namespace expanded::test
