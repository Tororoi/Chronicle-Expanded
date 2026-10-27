#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <string>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/stage.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

extern s32 gameTask;

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
    EXPECT_TRUE(stage.keep_alive);
}

TEST(StageFile, AnEmptyStageChangesNothing) {
    Stage stage;
    stage.place = 9;
    ASSERT_TRUE(expanded::ParseStage("", stage));
    EXPECT_EQ(stage.place, 0);
    EXPECT_EQ(stage.dungeon, -1);
    EXPECT_EQ(stage.character, -1);
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
    EXPECT_EQ(expanded::StageEntranceFloor(), -1);
    StageScope stage("floor = 3");
    EXPECT_EQ(expanded::StageEntranceFloor(), 3);
    EXPECT_EQ(expanded::StageEntranceFloor(), -1);
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
