// gtest before the game headers, whose one-letter macros it cannot stand.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "expanded/quests/quests.hpp"
#include "expanded/quests/tracker.hpp"
#include "itemdata.hpp"
#include "quests/quest_test.hpp"

namespace expanded::test {

namespace {

class QuestTracker : public QuestTest {
protected:
    void SetUp() override {
        QuestTest::SetUp();
        RegisterQuests();
    }

    static void Set(const char *quest, const char *stage, std::vector<std::pair<const char *, int>> vars = {}) {
        dc::quest::Act(quest, [&](dc::quest::Context &ctx) {
            ctx.SetStage(stage);
            for (const auto &[name, value] : vars) {
                ctx.SetVar(name, value);
            }
        });
    }
};

TEST_F(QuestTracker, NothingUnderWayListsNothing) {
    EXPECT_TRUE(TrackedQuests().empty());
}

TEST_F(QuestTracker, AMonsterSlayerQuestShowsItsKillsSoFar) {
    Set("slayer_macho", "active", {
                                      {"dungeon", 0 },
                                      {"monster", 3 },
                                      {"left",    5 },
                                      {"total",   12}
    });
    const std::vector<TrackedQuest> quests = TrackedQuests();
    ASSERT_EQ(quests.size(), 1U);
    EXPECT_EQ(quests[0].giver, "Macho");
    EXPECT_EQ(quests[0].town, "Norune Village");
    EXPECT_EQ(quests[0].task, "Defeat 12 Dashers in the Divine Beast Cave.");
    EXPECT_EQ(quests[0].done, 7);
    EXPECT_EQ(quests[0].total, 12);
    EXPECT_FALSE(quests[0].complete);
    EXPECT_EQ(quests[0].item, ITEM_POWERUP_POWDER);
}

TEST_F(QuestTracker, ACompleteQuestSendsThePlayerBack) {
    Set("slayer_bonka", "complete", {
                                        {"dungeon", 2 },
                                        {"monster", 21},
                                        {"left",    0 },
                                        {"total",   9 }
    });
    const std::vector<TrackedQuest> quests = TrackedQuests();
    ASSERT_EQ(quests.size(), 1U);
    EXPECT_TRUE(quests[0].complete);
    EXPECT_EQ(TrackedQuestStatus(quests[0]), "Done! Go back to Chief Bonka.");
}

TEST_F(QuestTracker, FishingQuestsSayTheirCatch) {
    Set("fishing_sam", "active", {
                                     {"size",  1  },
                                     {"least", 120},
                                     {"most",  130},
                                     {"place", 4  }
    });
    Set("fishing_pao", "active", {
                                     {"size",  0},
                                     {"fish",  4},
                                     {"left",  1},
                                     {"total", 2},
                                     {"place", 3}
    });
    const std::vector<TrackedQuest> quests = TrackedQuests();
    ASSERT_EQ(quests.size(), 2U);
    EXPECT_EQ(quests[0].giver, "Pao");
    EXPECT_EQ(quests[0].task, "Catch 2 Baku Bakus anywhere in Matataki Village.");
    EXPECT_EQ(quests[0].done, 1);
    EXPECT_EQ(quests[1].giver, "Sam");
    EXPECT_EQ(quests[1].town, "Queens");
    EXPECT_EQ(quests[1].task, "Catch any fish 120 to 130 cm long at East Harbor.");
}

TEST_F(QuestTracker, AnItemHuntIsListedUntilTheItemIsHad) {
    Set("hunt_phil", "heard");
    ASSERT_EQ(TrackedQuests().size(), 1U);
    EXPECT_EQ(TrackedQuests()[0].task, "Find the rare item lost in the Shipwreck, on its back floors.");
    game.items[ITEM_SHELL_RING] = 1;
    EXPECT_TRUE(TrackedQuests().empty());
}

TEST_F(QuestTracker, YellowDropsQuestsAreListed) {
    Set("yd_crystal", "asked");
    Set("yd_map", "cleared");
    const std::vector<TrackedQuest> quests = TrackedQuests();
    ASSERT_EQ(quests.size(), 2U);
    EXPECT_EQ(quests[0].giver, "Ledan");
    EXPECT_EQ(quests[0].town, "Yellow Drops");
    EXPECT_EQ(quests[1].giver, "Samba");
    EXPECT_TRUE(quests[1].complete);
    EXPECT_EQ(quests[1].item, ITEM_MAP);
}

} // namespace

} // namespace expanded::test
