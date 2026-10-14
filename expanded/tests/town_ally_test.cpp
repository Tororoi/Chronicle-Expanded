#include <gtest/gtest.h>

#include "character.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "expanded/town_ally.hpp"
#include "main.hpp"

TEST(TownAlly, CommitsOnlyPartyMembersOutsideYellowDrops) {
    int saved_map = MapNo;

    MapNo = TOWN_NORUNE;
    expanded::TownAllyReset();
    EXPECT_TRUE(expanded::TownAllyCommit(0, 1));
    EXPECT_FALSE(expanded::TownAllyCommit(1, 1));
    EXPECT_TRUE(expanded::TownAllyCommit(5, 6));
    EXPECT_FALSE(expanded::TownAllyCommit(6, 6));

    MapNo = TOWN_YELLOW_DROPS;
    EXPECT_FALSE(expanded::TownAllyCommit(1, 6));

    expanded::TownAllyReset();
    EXPECT_EQ(expanded::TownAllyCurrent(), 0);
    MapNo = saved_map;
}

TEST(TownAlly, ToansBodyLoadsAsItIsWhileToanWalks) {
    char  pack[] = "chara/c01d.chr";
    char  info[] = "info.cfg";
    char *pack_ptr = pack;
    char *info_ptr = info;

    expanded::TownAllyReset();
    expanded::TownAllyMainCharaPack(&pack_ptr, &info_ptr, nullptr);
    EXPECT_STREQ(pack_ptr, "chara/c01d.chr");
    EXPECT_STREQ(info_ptr, "info.cfg");
    EXPECT_EQ(expanded::TownAllyLoaded(), 0);
    EXPECT_FALSE(expanded::TownAllyBlocksLadder());
}

TEST(TownAlly, OtherBodiesLeaveTheTownBody) {
    char  pack[] = "chara/c01d_turi.chr";
    char  info[] = "c01d_turi.cfg";
    char *pack_ptr = pack;
    char *info_ptr = info;

    expanded::TownAllyReset();
    expanded::TownAllyMainCharaPack(&pack_ptr, &info_ptr, nullptr);
    EXPECT_EQ(pack_ptr, pack);
    EXPECT_EQ(expanded::TownAllyLoaded(), -1);
    EXPECT_EQ(expanded::TownAllyMarkLift(), 0.0f);
    EXPECT_EQ(expanded::TownAllyEyeHeight(), 14.0f);
    expanded::TownAllyReset();
}

TEST(TownAlly, ToanKeepsRetailEventsAndMotions) {
    expanded::TownAllyReset();
    EXPECT_FALSE(expanded::TownAllySkipsCutscene(0x83));
    EXPECT_EQ(expanded::TownAllyInteriorStartEvent(205, 3), 205);
    EXPECT_EQ(expanded::TownAllyMotionIndex(&MainChara, MainChara.motion[0], 9), 9);
    EXPECT_EQ(expanded::TownAllyMarkLift(), 0.0f);
}

TEST(TownAlly, ToanIsTheFirstBody) {
    EXPECT_STREQ(expanded::kTownAllyBodies[0].pack, "chara/c01d.chr");
    EXPECT_STREQ(expanded::kTownAllyBodies[0].info, "info.cfg");
    EXPECT_EQ(expanded::kTownAllyBodies[0].mark_lift, 0.0f);
    EXPECT_EQ(expanded::kTownAllyBodies[0].eye_height, 14.0f);
}
