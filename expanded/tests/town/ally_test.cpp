#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "battlemenu.hpp"
#include "character.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "expanded/core/save.hpp"
#include "expanded/data/town_model.hpp"
#include "expanded/town/ally.hpp"
#include "main.hpp"
#include "platform/paths.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

TEST(TownAlly, CommitsOnlyPartyMembersEverywhere) {
    int saved_map = MapNo;

    MapNo = TOWN_NORUNE;
    expanded::TownAllyReset();
    EXPECT_TRUE(expanded::TownAllyCommit(0, 1));
    EXPECT_FALSE(expanded::TownAllyCommit(1, 1));
    EXPECT_TRUE(expanded::TownAllyCommit(5, 6));
    EXPECT_FALSE(expanded::TownAllyCommit(6, 6));

    MapNo = TOWN_YELLOW_DROPS;
    EXPECT_TRUE(expanded::TownAllyCommit(1, 6));

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
    expanded::test::ScratchDir scratch("town_ally");
    fs::path                   dir = scratch.path();
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
}

TEST(TownAllyMenu, ASwapPlaysTheDungeonChangeAndARefusalLeavesThePage) {
    int saved_map = MapNo;

    MapNo = TOWN_NORUNE;
    expanded::TownAllyReset();
    MenuChara.state = MENU_CHARA_SELECT;
    expanded::TownAllyMenuCommit(2, 1);
    EXPECT_EQ(MenuChara.state, MENU_CHARA_SELECT);

    expanded::TownAllyMenuCommit(0, 6);
    EXPECT_EQ(MenuChara.state, MENU_CHARA_SELECT);

    MenuChara.change_counter = 9;
    expanded::TownAllyMenuCommit(2, 6);
    EXPECT_EQ(MenuChara.state, MENU_CHARA_CHANGE_LOAD);
    EXPECT_EQ(MenuChara.change_counter, 0);

    MenuChara.state = MENU_CHARA_SELECT;
    expanded::TownAllyReset();
    MapNo = saved_map;
}

TEST(TownAlly, ToanStandsAsTheGameHasHim) {
    // Only Xiao's cat sits: Toan standing however long keeps the game's idle, and moving its motion.
    expanded::TownAllyReset();
    for (int frame = 0; frame < 2 * expanded::kTownAllySitFrames; frame++) {
        EXPECT_EQ(expanded::TownAllyWalkMotion(0), 0) << frame;
    }
    EXPECT_EQ(expanded::TownAllyWalkMotion(2), 2);
}

// Each ally's boarding motions are slots of its built model.
TEST(TownAlly, BoardsInMotionsOfItsOwnModel) {
    const auto &allies = expanded::TownAllies();
    for (int ally = 1; ally < 6; ally++) {
        const expanded::TownAlly &model = allies[std::size_t(ally - 1)];
        for (const expanded::BoardingMotion &own : expanded::kBoardingMotions[ally]) {
            bool found = false;
            for (const expanded::TownSlot &slot : model.slots) {
                found = found || slot.idx == own.motion;
            }
            EXPECT_TRUE(found) << model.name << " motion " << own.motion;
        }
    }
}

// Toan boards and leaves the submarine as the game has him: his jump and climb load and play as they are.
TEST(TownAlly, ToanBoardsAsTheGameHasHim) {
    expanded::TownAllyReset();
    for (const char *cfg : expanded::kBoardingCfgs) {
        EXPECT_FALSE(expanded::TownAllyKeepsBody(cfg)) << cfg;
    }
    int   motion = expanded::kBoardingFirstMotion;
    float speed = 0.8f;
    expanded::TownAllyEventMotion(motion, speed);
    EXPECT_EQ(motion, expanded::kBoardingFirstMotion);
    EXPECT_EQ(speed, 0.8f);
}

namespace {

// A save of a party this size for a test, Expanded's state cleared after.
struct PartyOf {
    explicit PartyOf(int size) {
        expanded::SaveClear();
        SaveData->GetDngStatus()->party_size = size;
    }

    ~PartyOf() { expanded::SaveClear(); }

    expanded::test::TestSave save;
};

} // namespace

// The member chosen is kept with the save; Toan is kept as none, so a Toan-only save writes nothing.
TEST(PartyCharacter, IsTheMemberChosenKeptWithTheSave) {
    PartyOf party(6);
    expanded::SaveClear();
    EXPECT_EQ(expanded::PartyCharacter(), 0);
    expanded::PartyChoose(3);
    EXPECT_EQ(expanded::PartyCharacter(), 3);
    expanded::PartyChoose(0);
    EXPECT_EQ(expanded::PartyCharacter(), 0);
    EXPECT_TRUE(expanded::SaveEmpty());
}

// A member the party does not have (yet) is Toan.
TEST(PartyCharacter, IsToanForOneThePartyLacks) {
    PartyOf party(3);
    expanded::PartyChoose(4);
    EXPECT_EQ(expanded::PartyCharacter(), 0);
}

// Leaving a dungeon chooses the member out, unless a floor's limit zone or script put it out.
TEST(PartyCharacter, LeavingADungeonChoosesTheMemberOutUnlessAFloorPutItOut) {
    PartyOf party(6);
    expanded::PartyChoose(2);
    expanded::PartyLeavesDungeon(5, RES_LIMIT_ZONE_OSMOND, -1);
    EXPECT_EQ(expanded::PartyCharacter(), 2);
    expanded::PartyLeavesDungeon(5, RES_LIMIT_ZONE_NONE, 5);
    EXPECT_EQ(expanded::PartyCharacter(), 2);
    expanded::PartyLeavesDungeon(5, RES_LIMIT_ZONE_THIRST, -1);
    EXPECT_EQ(expanded::PartyCharacter(), 5);
    expanded::PartyLeavesDungeon(0, RES_LIMIT_ZONE_NONE, -1);
    EXPECT_EQ(expanded::PartyCharacter(), 0);
}

// A cutscene, an event posing the player as Toan, switches to Toan, who stays the one chosen.
TEST(PartyCharacter, ACutsceneSwitchesToToan) {
    PartyOf party(6);
    expanded::TownAllyReset();
    expanded::PartyChoose(4);
    expanded::TownAllyEventBody();
    EXPECT_EQ(expanded::PartyCharacter(), 0);
    EXPECT_EQ(expanded::TownAllyCurrent(), 0);
}
