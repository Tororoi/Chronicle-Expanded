#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <format>
#include <fstream>
#include <string>
#include <utility>

#include "character.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "editloop3.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/stage.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "mainselect.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern s32 gameTask;
} // namespace dc::game

namespace {

using expanded::Stage;

// A stage file's text set as the run's stage for one test, and none after.
class StageScope {
public:
    explicit StageScope(const std::string &text) {
        std::filesystem::path path = folder_.path() / "test.stage";
        std::ofstream(path) << text;
        std::string why;
        EXPECT_TRUE(expanded::SetStage(path.string().c_str(), &why)) << why;
    }

    ~StageScope() { expanded::SetStage(nullptr); }

    StageScope(const StageScope &) = delete;
    StageScope &operator=(const StageScope &) = delete;

private:
    expanded::test::ScratchDir folder_{"stage"};
};

// The player at (x, 0, z) facing yaw.
struct Standing {
    float saved_pos[3];
    float saved_rot[3];

    Standing(float x, float z, float yaw) {
        for (int i = 0; i < 3; i++) {
            saved_pos[i] = CharaMain.pos[i];
        }
        CharaMain.GetRotation(saved_rot);
        CharaMain.SetPosition(x, 0.0f, z);
        CharaMain.SetRotation(0.0f, yaw, 0.0f);
    }

    ~Standing() {
        CharaMain.SetPosition(saved_pos[0], saved_pos[1], saved_pos[2]);
        CharaMain.SetRotation(saved_rot[0], saved_rot[1], saved_rot[2]);
    }
};

// A town for the stage's town step, the player at the origin: its script's label 11 calls a function that loads the
// fishing data (a fishing sign's), label 12 only goes fishing; its event points are a label-12 event, a door to label
// 11, label-11 events at (100, 5, 200) and (-50, 0, 0), and a label-11 event switched off.
class TestTown {
public:
    TestTown() {
        // Words: the header, the label table at 8, the code from 16 (records at 16, 20 and 24, cells after them).
        constexpr int kCode = 16;
        int32_t      *w = words_.data();
        w[2] = kCode * 4; // code
        w[3] = 8 * 4;     // label table
        w[4] = 2;         // labels
        w[8] = 11, w[9] = 16 * 4;
        w[10] = 12, w[11] = 24 * 4;
        auto record = [&](int at, int code_word) { w[at] = (code_word - kCode) * 4; };
        auto cell = [&](int at, int op, int arg1, int arg2) { w[at] = op, w[at + 1] = arg1, w[at + 2] = arg2; };
        record(16, 28); // label 11: calls the record at 20
        cell(28, RS_OP_CALL, 0, (20 - kCode) * 4);
        cell(31, RS_OP_RET, 0, 0);
        record(20, 37); // _LOAD_FISHING_DATA
        cell(37, RS_OP_PUSH_CONST, 1, 998);
        cell(40, RS_OP_EXT, 1, 0);
        cell(43, RS_OP_RET, 0, 0);
        record(24, 49); // label 12: _GOTO_FISHING
        cell(49, RS_OP_PUSH_CONST, 1, 997);
        cell(52, RS_OP_EXT, 1, 0);
        cell(55, RS_OP_RET, 0, 0);
        EdEventScript.prog = reinterpret_cast<RS_PROG_HEADER *>(w);
        EdEventScript.code = reinterpret_cast<char *>(w + kCode);

        auto point = [&](int i, int type, int label, float x, float y, float z) {
            points_[i].enabled = 1;
            points_[i].event_type = type;
            points_[i].side = label;
            points_[i].parts_no = -1;
            points_[i].position[0] = x, points_[i].position[1] = y, points_[i].position[2] = z;
        };
        point(0, ED_EVENT_POINT_EVENT, 12, 10.0f, 0.0f, 10.0f);
        point(1, ED_EVENT_POINT_DOOR, 11, 20.0f, 0.0f, 20.0f);
        point(2, ED_EVENT_POINT_EVENT, 11, 100.0f, 5.0f, 200.0f);
        point(3, ED_EVENT_POINT_EVENT, 11, -50.0f, 0.0f, 0.0f);
        point(4, ED_EVENT_POINT_EVENT, 11, 70.0f, 0.0f, 70.0f);
        points_[4].enabled = 0;
        EdMoveCharaInfo.points = points_.data();
        EdMoveCharaInfo.point_count = static_cast<int>(points_.size());
        EdMoveCharaInfo.fishing = 0;
        EdMoveCharaInfo.event_no = -1;
        EdMoveCharaInfo.system_event_no = -1;
        EdMoveCharaInfo.acted = 0;
        chara_fishing = ED_FISHING_INIT;
        esa_type = -1;
        MapNo = TOWN_MATATAKI;
        Chara = &chara_;
        chara_.SetPosition(0.0f, 0.0f, 0.0f);
    }

    ~TestTown() {
        EdEventScript.prog = saved_prog_;
        EdEventScript.code = saved_code_;
        EdMoveCharaInfo = saved_info_;
        chara_fishing = saved_fishing_;
        esa_type = saved_esa_;
        MapNo = saved_map_;
        Chara = saved_chara_;
    }

    TestTown(const TestTown &) = delete;
    TestTown &operator=(const TestTown &) = delete;

    CCharacter &chara() { return chara_; }

private:
    alignas(16) std::array<int32_t, 64> words_{};
    std::array<ED_EVENT_POINT, 5> points_{};
    CMainChara                    chara_;
    RS_PROG_HEADER               *saved_prog_ = EdEventScript.prog;
    char                         *saved_code_ = EdEventScript.code;
    ED_MOVE_CHARA_INFO            saved_info_ = EdMoveCharaInfo;
    int                           saved_fishing_ = chara_fishing;
    int                           saved_esa_ = esa_type;
    s32                           saved_map_ = MapNo;
    CCharacter                   *saved_chara_ = Chara;
};

} // namespace

TEST(StageFile, EveryKeyIsRead) {
    Stage       stage;
    std::string why;
    ASSERT_TRUE(expanded::ParseStage("# a comment\n"
                                     "roster = 44\n"
                                     "slot = 2\n"
                                     "dungeon = 3   # Sun & Moon Temple\n"
                                     "floor = 3\n"
                                     "\n"
                                     "character = ungaga\n"
                                     "weapon = 356\n"
                                     "attack = 25\n"
                                     "sphere = 312\n"
                                     "seed = 7\n"
                                     "place = 5\n"
                                     "distance = 100\n"
                                     "spread = 0.5\n"
                                     "angle = -0.25\n"
                                     "place_kind = 44\n"
                                     "place_after = 30\n"
                                     "big_room = yes\n"
                                     "circle = 5\n"
                                     "keep_alive = true\r\n",
                                     stage, &why))
        << why;
    EXPECT_EQ(stage.roster, "44");
    EXPECT_EQ(stage.slot, 2);
    EXPECT_EQ(stage.dungeon, 3);
    EXPECT_EQ(stage.floor, 3);
    EXPECT_EQ(stage.character, CHARA_UNGAGA);
    EXPECT_EQ(stage.weapon, 356);
    EXPECT_EQ(stage.attack, 25);
    EXPECT_EQ(stage.sphere, 312);
    EXPECT_EQ(stage.seed, 7);
    EXPECT_EQ(stage.place, 5);
    EXPECT_FLOAT_EQ(stage.distance, 100.0f);
    EXPECT_FLOAT_EQ(stage.spread, 0.5f);
    EXPECT_FLOAT_EQ(stage.angle, -0.25f);
    EXPECT_EQ(stage.place_kind, 44);
    EXPECT_EQ(stage.place_after, 30);
    EXPECT_TRUE(stage.big_room);
    EXPECT_EQ(stage.circle, 5);
    EXPECT_TRUE(stage.keep_alive);
}

TEST(StageFile, AnEmptyStageChangesNothing) {
    Stage stage;
    stage.place = 9;
    ASSERT_TRUE(expanded::ParseStage("", stage));
    EXPECT_EQ(stage.place, 0);
    EXPECT_EQ(stage.dungeon, -1);
    EXPECT_EQ(stage.character, -1);
    EXPECT_EQ(stage.circle, -1);
    EXPECT_FALSE(stage.keep_alive);
}

TEST(StageFile, CharactersByNameOrNumber) {
    Stage stage;
    ASSERT_TRUE(expanded::ParseStage("character = xiao", stage));
    EXPECT_EQ(stage.character, CHARA_XIAO);
    ASSERT_TRUE(expanded::ParseStage("character = 5", stage));
    EXPECT_EQ(stage.character, CHARA_OSMOND);
    EXPECT_FALSE(expanded::ParseStage("character = 6", stage));
    EXPECT_FALSE(expanded::ParseStage("character = steve", stage));
}

TEST(StageFile, ABadLineIsNamedAndTheStageKept) {
    Stage stage;
    stage.place = 4;
    std::string why;
    EXPECT_FALSE(expanded::ParseStage("place = 5\nflor = 3\n", stage, &why));
    EXPECT_EQ(why, "line 2: bad flor \"3\"");
    EXPECT_EQ(stage.place, 4);
    EXPECT_FALSE(expanded::ParseStage("dungeon = 7", stage, &why));
    EXPECT_FALSE(expanded::ParseStage("place = -1", stage, &why));
    EXPECT_FALSE(expanded::ParseStage("distance = far", stage, &why));
    EXPECT_FALSE(expanded::ParseStage("big_room = maybe", stage, &why));
    EXPECT_FALSE(expanded::ParseStage("circle = 10", stage, &why));
    EXPECT_FALSE(expanded::ParseStage("\n\ndungeon 3", stage, &why));
    EXPECT_EQ(why, "line 3: no '=' in \"dungeon 3\"");
}

TEST(Stage, AFileThatCannotBeReadIsRefused) {
    std::string why;
    EXPECT_FALSE(expanded::SetStage("/nonexistent/stage", &why));
    EXPECT_EQ(why, "cannot be read");
    EXPECT_EQ(expanded::ActiveStage(), nullptr);
}

TEST(Stage, ItsSeedSeedsExpandedsRandomNumbers) {
    expanded::SeedModRandom(7);
    int        first = expanded::ModRoll(1000000), second = expanded::ModRoll(1000000);
    StageScope stage("seed = 7");
    EXPECT_EQ(expanded::ModRoll(1000000), first);
    EXPECT_EQ(expanded::ModRoll(1000000), second);
}

TEST(Stage, TheEntranceTakesItsFloorOnce) {
    EXPECT_EQ(expanded::StageEntranceFloor(-1), -1);
    StageScope stage("floor = 3");
    EXPECT_EQ(expanded::StageEntranceFloor(-1), 3);
    EXPECT_EQ(expanded::StageEntranceFloor(-1), -1);
}

TEST(Stage, NoSaveIsLoadedWithoutADungeon) {
    StageScope stage("floor = 3");
    EXPECT_TRUE(expanded::StageLoadSave());
}

TEST(Stage, TheMemberHoldsItsWeaponAtFullOrGivenAttack) {
    expanded::test::TestFloor floor;
    CUserStatus              &status = floor.status();
    status.equipped_weapon_slot[CHARA_UNGAGA] = 2;
    {
        StageScope stage("character = ungaga\nweapon = 356");
        expanded::StageStatusReady();
        const WEAPON_HAVE &held = status.chara_weapons[CHARA_UNGAGA][2];
        EXPECT_EQ(held.item_no, ITEM_WEAPON_HERCULES_WRATH);
        EXPECT_EQ(held.attack, GetWeaponData(ITEM_WEAPON_HERCULES_WRATH)->attack_max);
    }
    {
        StageScope stage("character = ungaga\nweapon = 356\nattack = 25");
        expanded::StageStatusReady();
        EXPECT_EQ(status.chara_weapons[CHARA_UNGAGA][2].attack, 25);
    }
}

TEST(Stage, ASphereGoesOnXiaosSuperSteve) {
    expanded::test::TestFloor floor;
    CUserStatus              &status = floor.status();
    StageScope                stage("character = xiao\nsphere = 356");
    expanded::StageStatusReady();
    const WEAPON_HAVE &held = status.chara_weapons[CHARA_XIAO][static_cast<int>(status.equipped_weapon_slot[CHARA_XIAO])];
    EXPECT_EQ(held.item_no, ITEM_WEAPON_SUPER_STEVE);
    EXPECT_EQ(held.attach[0].item_no, ITEM_ATTACH_SYNTHESIS_SPHERE);
    EXPECT_EQ(held.attach[0].sphere_weapon_no, ITEM_WEAPON_HERCULES_WRATH);
}

TEST(Stage, MonstersAreSetOutInFrontAfterTheirSteps) {
    expanded::test::TestFloor floor;
    CMonstorUnit             &unit = floor.unit();
    for (int m : {0, 2, 5, 7}) {
        unit.monster[m].state = 2;
        unit.monster[m].revealed = -1;
        unit.chara[m][0].SetPosition(500.0f, 0.0f, 500.0f);
    }
    unit.monster[5].revealed = 0; // in a chest
    Standing   standing(100.0f, 200.0f, 0.0f);
    StageScope stage("place = 2\ndistance = 50\nspread = 0\nplace_after = 3");
    expanded::StageFrame(); // no member asked for: out at once
    expanded::StageStep();
    expanded::StageStep();
    EXPECT_FLOAT_EQ(unit.chara[0][0].pos[2], 500.0f);
    expanded::StageStep();
    // The first two in play, 50 ahead (yaw 0 faces +z); the third sent away; the one in a chest left.
    for (int m : {0, 2}) {
        EXPECT_NEAR(unit.chara[m][0].pos[0], 100.0f, 1e-3f) << m;
        EXPECT_NEAR(unit.chara[m][0].pos[2], 250.0f, 1e-3f) << m;
    }
    EXPECT_NEAR(unit.chara[7][0].pos[0], 2100.0f, 1e-3f);
    EXPECT_NEAR(unit.chara[7][0].pos[2], 2200.0f, 1e-3f);
    EXPECT_FLOAT_EQ(unit.chara[5][0].pos[0], 500.0f);
}

TEST(Stage, PlacementSpreadsThemAboutTheFacing) {
    expanded::test::TestFloor floor;
    CMonstorUnit             &unit = floor.unit();
    for (int m = 0; m < 3; m++) {
        unit.monster[m].state = 2;
        unit.monster[m].revealed = -1;
    }
    Standing   standing(0.0f, 0.0f, 0.0f);
    StageScope stage("place = 3\ndistance = 100\nspread = 0.5\nangle = 0.25\nplace_after = 1");
    expanded::StageFrame();
    expanded::StageStep();
    for (int m = 0; m < 3; m++) {
        float a = 0.25f + (static_cast<float>(m) - 1.0f) * 0.5f;
        EXPECT_NEAR(unit.chara[m][0].pos[0], 100.0f * std::sin(a), 1e-3f) << m;
        EXPECT_NEAR(unit.chara[m][0].pos[2], 100.0f * std::cos(a), 1e-3f) << m;
    }
}

TEST(Stage, AMagicCircleIsSetOutInFrontAfterTheSteps) {
    expanded::test::TestFloor      floor;
    alignas(64) static CDungeonMap map;
    CDungeonMap                   *saved_map = NowDngMap;
    NowDngMap = &map;
    map.trap_circle[0] = {};
    {
        Standing   standing(100.0f, 200.0f, 0.0f);
        StageScope stage("circle = 9\nplace_after = 2");
        expanded::StageFrame();
        expanded::StageStep();
        EXPECT_EQ(map.trap_circle[0].state, 0);
        expanded::StageStep();
        const MAP_TRAP_CIRCLE &circle = map.trap_circle[0];
        EXPECT_EQ(circle.state, 1); // waiting to be stepped on
        EXPECT_EQ(circle.kind, TRAP_CIRCLE_WHP_QUARTER);
        EXPECT_NEAR(circle.pos[0], 100.0f, 1e-3f);
        EXPECT_NEAR(circle.pos[2], 240.0f, 1e-3f); // 40 ahead: yaw 0 faces +z
    }
    NowDngMap = saved_map;
}

TEST(Stage, KeepAliveFillsThePartysLife) {
    expanded::test::TestFloor floor;
    CUserStatus              &status = floor.status();
    status.max_hp[CHARA_TOAN] = 150;
    status.hp[CHARA_TOAN] = status.next_hp[CHARA_TOAN] = 20;
    StageScope stage("keep_alive = yes");
    expanded::StageFrame();
    expanded::StageStep();
    EXPECT_EQ(status.hp[CHARA_TOAN], 150);
    EXPECT_EQ(status.next_hp[CHARA_TOAN], 150);
}

// Asked for another member, the stage waits for the floor to be in play before bringing it out.
TEST(Stage, NothingIsStagedBeforeTheMemberIsOut) {
    expanded::test::TestFloor floor;
    CUserStatus              &status = floor.status();
    status.cur_chara = CHARA_TOAN;
    status.party_size = 5;
    status.max_hp[CHARA_TOAN] = 150;
    status.hp[CHARA_TOAN] = 20;
    s32 saved_task = gameTask;
    gameTask = GAME_TASK_SCRIPT_START;
    {
        StageScope stage("character = ungaga\nkeep_alive = yes");
        expanded::StageFrame();
        expanded::StageStep();
        EXPECT_EQ(status.hp[CHARA_TOAN], 20);
        EXPECT_EQ(status.cur_chara, CHARA_TOAN);
    }
    gameTask = saved_task;
}

TEST(StageFile, TheTownKeysAreRead) {
    Stage       stage;
    std::string why;
    ASSERT_TRUE(expanded::ParseStage("town = matataki\n"
                                     "fishing_spot = 1\n"
                                     "bait = 197\n"
                                     "time = dusk\n"
                                     "give = 310, 197\n",
                                     stage, &why))
        << why;
    EXPECT_EQ(stage.town, TOWN_MATATAKI);
    EXPECT_EQ(stage.fishing_spot, 1);
    EXPECT_EQ(stage.bait, ITEM_MIMI);
    EXPECT_FLOAT_EQ(stage.time, 3.0f);
    EXPECT_EQ(stage.give, "310,197");
}

TEST(StageFile, TownsByNameOrMapNo) {
    constexpr std::pair<const char *, int> kTowns[] = {
        {"norune",       TOWN_NORUNE      },
        {"matataki",     TOWN_MATATAKI    },
        {"queens",       TOWN_QUEENS      },
        {"muska_lacka",  TOWN_MUSKA_LACKA },
        {"yellow_drops", TOWN_YELLOW_DROPS},
        {"brownboo",     14               },
        {"east_harbor",  19               },
        {"23",           23               },
        {"4",            4                },
    };
    for (const auto &[text, map_no] : kTowns) {
        Stage stage;
        ASSERT_TRUE(expanded::ParseStage(std::string("town = ") + text, stage)) << text;
        EXPECT_EQ(stage.town, map_no) << text;
    }
    Stage stage;
    for (const char *text : {"5", "10", "53", "99", "200", "-1", "pond"}) {
        EXPECT_FALSE(expanded::ParseStage(std::string("town = ") + text, stage)) << text;
    }
}

// A period by name starts in that period, as the game tells the time of day.
TEST(StageFile, ATimeIsAPeriodOrTheClock) {
    constexpr std::pair<const char *, int> kPeriods[] = {
        {"morning",   ED_TIME_MORNING},
        {"day",       ED_TIME_DAY    },
        {"afternoon", ED_TIME_DAY    },
        {"evening",   ED_TIME_EVENING},
        {"dusk",      ED_TIME_EVENING},
        {"night",     ED_TIME_NIGHT  },
    };
    for (const auto &[text, period] : kPeriods) {
        Stage stage;
        ASSERT_TRUE(expanded::ParseStage(std::string("time = ") + text, stage)) << text;
        EXPECT_EQ(EdGetTime(stage.time), period) << text;
    }
    Stage stage;
    ASSERT_TRUE(expanded::ParseStage("time = 4.5", stage));
    EXPECT_FLOAT_EQ(stage.time, 4.5f);
    for (const char *text : {"12", "-1", "noon"}) {
        EXPECT_FALSE(expanded::ParseStage(std::string("time = ") + text, stage)) << text;
    }
}

TEST(StageFile, ABaitIsOneThePickerOffers) {
    Stage stage;
    for (int bait : {ITEM_EVY, ITEM_MIMI, ITEM_THROBBING_CHERRY, ITEM_PETITE_FISH}) {
        ASSERT_TRUE(expanded::ParseStage(std::format("town = 0\nbait = {}", bait), stage)) << bait;
        EXPECT_EQ(stage.bait, bait);
    }
    EXPECT_FALSE(expanded::ParseStage(std::format("town = 0\nbait = {}", static_cast<int>(ITEM_FISHING_ROD)), stage));
    EXPECT_FALSE(expanded::ParseStage("town = 0\nbait = 0", stage));
}

TEST(StageFile, GiveTakesWhatGiveDoes) {
    Stage stage;
    ASSERT_TRUE(expanded::ParseStage("give = 268", stage));
    EXPECT_EQ(stage.give, "268");
    for (const char *text : {"0", "377", "310,", ",310", "sun", ""}) {
        EXPECT_FALSE(expanded::ParseStage(std::string("give = ") + text, stage)) << text;
    }
}

TEST(StageFile, TheTownKeysNeedATownAndNoDungeon) {
    Stage       stage;
    std::string why;
    EXPECT_FALSE(expanded::ParseStage("town = 0\ndungeon = 1", stage, &why));
    EXPECT_EQ(why, "a dungeon and a town");
    EXPECT_FALSE(expanded::ParseStage("fishing_spot = 0", stage, &why));
    EXPECT_EQ(why, "a fishing_spot with no town");
    EXPECT_FALSE(expanded::ParseStage("bait = 197", stage, &why));
    EXPECT_EQ(why, "a bait with no town");
}

// Signs count in the event points' order, among the switched-on events whose script loads the fishing data.
TEST(Stage, ThePlayerStandsAtTheFishingSignFacingTheWayTheyCame) {
    TestTown town;
    {
        StageScope stage("town = matataki\nfishing_spot = 0");
        expanded::StageTownStep();
        EXPECT_FLOAT_EQ(town.chara().pos[0], 100.0f);
        EXPECT_FLOAT_EQ(town.chara().pos[1], 5.0f);
        EXPECT_FLOAT_EQ(town.chara().pos[2], 200.0f);
        EXPECT_NEAR(town.chara().rotation.y, std::atan2(100.0f, 200.0f), 1e-5f);
        town.chara().SetPosition(1.0f, 0.0f, 1.0f);
        expanded::StageTownStep(); // once
        EXPECT_FLOAT_EQ(town.chara().pos[0], 1.0f);
    }
    town.chara().SetPosition(0.0f, 0.0f, 0.0f);
    {
        StageScope stage("town = matataki\nfishing_spot = 1");
        expanded::StageTownStep();
        EXPECT_FLOAT_EQ(town.chara().pos[0], -50.0f);
        EXPECT_FLOAT_EQ(town.chara().pos[2], 0.0f);
    }
    town.chara().SetPosition(0.0f, 0.0f, 0.0f);
    {
        StageScope stage("town = matataki\nfishing_spot = 2"); // none
        expanded::StageTownStep();
        EXPECT_FLOAT_EQ(town.chara().pos[0], 0.0f);
    }
}

TEST(Stage, NothingIsStagedInAnotherMap) {
    TestTown   town;
    StageScope stage("town = norune\nfishing_spot = 0");
    expanded::StageTownStep();
    EXPECT_FLOAT_EQ(town.chara().pos[0], 0.0f);
}

// The bait is given on arrival; once fishing, the stage asks for the bait change as the square button does and the
// picker takes the stage's bait out of the bag for the script, once.
TEST(Stage, TheBaitIsGivenAndPickedOnceFishingStarts) {
    expanded::test::TestSave save;
    TestTown                 town;
    StageScope               stage("town = matataki\nbait = 197");
    CDngStatusData          &bag = *SaveData->GetDngStatus();
    ASSERT_LT(bag.SearchItemIndexNo(ITEM_MIMI), 0);
    expanded::StageTownStep();
    int slot = bag.SearchItemIndexNo(ITEM_MIMI);
    ASSERT_GE(slot, 0);
    EXPECT_EQ(EdMoveCharaInfo.event_no, -1); // not fishing yet

    RS_STACKDATA choice[9] = {};
    EXPECT_FALSE(expanded::StagePickBait(choice)); // nothing asked for: the picker opens

    EdMoveCharaInfo.fishing = 1;
    chara_fishing = ED_FISHING_STAND;
    expanded::StageTownStep();
    EXPECT_EQ(EdMoveCharaInfo.event_no, 0x86);
    EXPECT_NE(EdMoveCharaInfo.acted, 0);

    ASSERT_TRUE(expanded::StagePickBait(choice));
    EXPECT_EQ(choice[0].i, ITEM_MIMI);
    EXPECT_EQ(bag.item_pack.item[slot], 0);
    EXPECT_FALSE(expanded::StagePickBait(choice));

    EdMoveCharaInfo.event_no = -1;
    expanded::StageTownStep();
    EXPECT_EQ(EdMoveCharaInfo.event_no, -1); // asked once
}

TEST(Stage, ATownStagesSaveIsLoaded) {
    expanded::test::OptionScope folder; // a save folder with no saves in it
    StageScope                  stage("town = norune\ntime = night");
    EXPECT_FALSE(expanded::StageLoadSave());
}
