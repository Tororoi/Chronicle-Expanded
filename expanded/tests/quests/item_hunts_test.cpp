// gtest before the game headers, whose one-letter macros it cannot stand.
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "dungeonmap.hpp"
#include "expanded/quests/item_hunts.hpp"
#include "itemdata.hpp"
#include "quests/quest_test.hpp"

namespace expanded::test {

namespace {

const ItemHunt &Laura() {
    return ItemHunts()[0];
}

class ItemHunts : public QuestTest {
protected:
    void SetUp() override {
        QuestTest::SetUp();
        RegisterItemHunts();
    }

    // A floor with a large chest, then two small ones.
    static std::unique_ptr<CDungeonMap> Floor() {
        auto map = std::make_unique<CDungeonMap>();
        for (int i = 0; i < 3; ++i) {
            map->boxes[i].used = 1;
            map->boxes[i].kind = i == 0 ? TREASURE_BOX_LARGE : TREASURE_BOX_SMALL;
            map->boxes[i].item_no = ITEM_BREAD;
        }
        return map;
    }

    // How many of many back floors of the dungeon have the item in their first small chest.
    static int FloorsWith(const ItemHunt &hunt, int dungeon, bool back_floor) {
        int with = 0;
        for (int floor = 0; floor < 300; ++floor) {
            auto map = Floor();
            ItemHuntChests(*map, dungeon, back_floor);
            EXPECT_EQ(map->boxes[0].item_no, ITEM_BREAD);
            EXPECT_EQ(map->boxes[2].item_no, ITEM_BREAD);
            with += map->boxes[1].item_no == hunt.item ? 1 : 0;
        }
        return with;
    }
};

TEST_F(ItemHunts, TheRumourIsOnOfferThenTheHuntIsOn) {
    EXPECT_TRUE(Offered(Laura().giver, "Got anything for me?"));
    const std::string said = Ask(Laura().giver, "Got anything for me?");
    EXPECT_NE(said.find("back floors of the Divine Beast Cave"), std::string::npos) << said;
    EXPECT_EQ(Stage(Laura().quest), "heard");
    EXPECT_TRUE(ItemHuntOn(Laura()));
    EXPECT_FALSE(Offered(Laura().giver, "Got anything for me?"));
    EXPECT_TRUE(Offered(Laura().giver, "About the quest."));
}

TEST_F(ItemHunts, NoItemBeforeTheRumour) {
    EXPECT_EQ(FloorsWith(Laura(), Laura().dungeon, true), 0);
}

TEST_F(ItemHunts, ABackFloorOfItsDungeonHasItAThirdOfTheTime) {
    Ask(Laura().giver, "Got anything for me?");
    const int with = FloorsWith(Laura(), Laura().dungeon, true);
    EXPECT_GT(with, 60);
    EXPECT_LT(with, 140);
    EXPECT_EQ(FloorsWith(Laura(), Laura().dungeon, false), 0);
    EXPECT_EQ(FloorsWith(Laura(), 1, true), 0);
}

TEST_F(ItemHunts, FindingItEndsTheHuntAndTheGiverHearsOfIt) {
    Ask(Laura().giver, "Got anything for me?");
    game.stored[ITEM_MEDUSA_POWDER] = 1;
    EXPECT_FALSE(ItemHuntOn(Laura()));
    EXPECT_EQ(FloorsWith(Laura(), Laura().dungeon, true), 0);

    const std::string said = Ask(Laura().giver, "About the quest.");
    EXPECT_NE(said.find("Medusa Powder"), std::string::npos) << said;
    EXPECT_EQ(Stage(Laura().quest), "done");
    EXPECT_FALSE(Offered(Laura().giver, "About the quest."));
    EXPECT_FALSE(Offered(Laura().giver, "Got anything for me?"));
}

TEST_F(ItemHunts, UntilFoundTheGiverRemindsWhereItIs) {
    Ask(Laura().giver, "Got anything for me?");
    const std::string said = Ask(Laura().giver, "About the quest.");
    EXPECT_NE(said.find("Did you find that rare item yet?"), std::string::npos) << said;
    EXPECT_EQ(Stage(Laura().quest), "heard");
}

} // namespace

} // namespace expanded::test
