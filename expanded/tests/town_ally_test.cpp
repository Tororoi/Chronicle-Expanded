#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>

#include "editground.hpp"
#include "editloop.hpp"
#include "expanded/town_ally.hpp"
#include "main.hpp"
#include "platform/paths.hpp"

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

TEST(TownAlly, ToanKeepsRetailEvents) {
    expanded::TownAllyReset();
    EXPECT_FALSE(expanded::TownAllySkipsCutscene(0x83));
    EXPECT_EQ(expanded::TownAllyInteriorStartEvent(205, 3), 205);
    EXPECT_EQ(expanded::TownAllyMarkLift(), 0.0f);
}

TEST(TownAlly, ToanIsTheFirstBody) {
    EXPECT_STREQ(expanded::kTownAllyBodies[0].pack, "chara/c01d.chr");
    EXPECT_STREQ(expanded::kTownAllyBodies[0].info, "info.cfg");
    EXPECT_EQ(expanded::kTownAllyBodies[0].mark_lift, 0.0f);
    EXPECT_EQ(expanded::kTownAllyBodies[0].eye_height, 14.0f);
}

TEST(TownAlly, BuiltModelsReplaceTheDiscBodies) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / std::format("dc_town_ally_{}", std::rand());
    fs::create_directories(dir / "expanded/town");
    std::ofstream(dir / "expanded/town/ungaga.chr") << "chr";

    PathsSetDataRoot(dir);
    expanded::TownAllyReset();
    EXPECT_STREQ(expanded::TownAllyPack(0).first, "chara/c01d.chr");
    EXPECT_STREQ(expanded::TownAllyPack(4).first, "expanded/town/ungaga.chr");
    EXPECT_STREQ(expanded::TownAllyPack(4).second, "e323_2c10a.cfg");
    EXPECT_STREQ(expanded::TownAllyPack(2).first, "gedit/s01/chara/c06p.chr");
    EXPECT_STREQ(expanded::TownAllyPack(2).second, "info.cfg");

    PathsReset();
    expanded::TownAllyReset();
    fs::remove_all(dir);
}
