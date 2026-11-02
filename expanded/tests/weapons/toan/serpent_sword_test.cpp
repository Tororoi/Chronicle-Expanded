#include <gtest/gtest.h>

#include <array>
#include <initializer_list>
#include <string>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/serpent_sword.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

using expanded::Monster;

namespace {

// The head's clips as e117e54a.cfg gives them.
std::vector<MOTION_INFO> g_clips = {
    {.start = 10,  .end = 40,  .speed = 0.2f },
    {.start = 41,  .end = 44,  .speed = 0.03f},
    {.start = 110, .end = 115, .speed = 0.05f},
    {.start = 46,  .end = 70,  .speed = 0.2f },
    {.start = 71,  .end = 85,  .speed = 0.2f },
    {.start = 86,  .end = 92,  .speed = 0.1f },
    {.start = 93,  .end = 100, .speed = 0.1f },
    {.start = 125, .end = 133, .speed = 0.1f },
    {.start = 135, .end = 155, .speed = 0.2f },
    {.start = 270, .end = 290, .speed = 0.3f },
    {.start = 176, .end = 260, .speed = 0.2f },
};
constexpr int kAppearSteps = 100; // (155 - 135) / 0.2
constexpr int kHissSteps = 60;    // (92 - 86) / 0.1

CFrameVu1                g_snake_frame;
std::vector<std::string> g_loaded;

bool FakeSnake(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    g_snake_frame.Initialize();
    chara.frame = &g_snake_frame;
    chara.motion_type.motion_info = g_clips.data();
    g_loaded.emplace_back(path);
    return true;
}

// One monster of each kind in its own slot, in play at the origin; Toan out with weapon (carrying sphere), the player
// at the origin facing +z, the guard down.
class SerpentSword : public ::testing::Test {
protected:
    explicit SerpentSword(int weapon = ITEM_WEAPON_SERPENT_SWORD, int sphere = 0) : wielding_(weapon, sphere) {
        for (int i = 0; i < 4; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_guard_ = BtActStatus.guard_mode;
        BtActStatus.guard_mode = 0;
        expanded::SetFloorCharacterLoader(FakeSnake);
        expanded::FloorCharactersClear();
        expanded::SnakeCharmerReset();
        g_loaded.clear();
    }

    ~SerpentSword() override {
        expanded::SnakeCharmerReset();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(nullptr);
        BtActStatus.guard_mode = saved_guard_;
        for (int i = 0; i < 4; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
    }

    void Put(std::initializer_list<Monster> kinds) {
        int slot = 0;
        for (Monster kind : kinds) {
            floor_.SetUpModel(slot, kind);
            floor_.Spawn(slot, slot);
            floor_.unit().monster[slot].state = 2;
            floor_.unit().monster[slot].revealed = -1;
            slot++;
        }
    }

    void At(int monster, float x, float z) { floor_.unit().chara[monster][0].SetPosition(x, 0.0f, z); }

    void Guard(bool on) { BtActStatus.guard_mode = on ? expanded::kGuardUp : 0; }

    // A hit of the active character's weapon (or not) on monster, amount the game's.
    int Hit(int monster, int amount, bool weapon = true) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = floor_.status().cur_chara;
        hit.vs_monster = weapon ? table : nullptr;
        return expanded::MonsterHitAmount(floor_.unit(), monster, hit, amount);
    }

    // Where monster's script finds the player.
    std::array<float, 4> Target(int monster) {
        std::array<float, 4> found{};
        floor_.unit().current_monster = monster;
        expanded::ScriptPlayerPosition(found.data());
        return found;
    }

    expanded::test::Wielding  wielding_;
    expanded::test::TestFloor floor_;
    float                     saved_pos_[4] = {};
    float                     saved_rot_[4] = {};
    int                       saved_guard_ = 0;
};

class SuperSteveSerpent : public SerpentSword {
protected:
    SuperSteveSerpent() : SerpentSword(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SERPENT_SWORD) {}
};

class NoSerpent : public SerpentSword {
protected:
    NoSerpent() : SerpentSword(ITEM_WEAPON_DAGGER) {}
};

} // namespace

TEST_F(SerpentSword, SnakeSlayerKillsAuntieMeduInOneHit) {
    Put({Monster::AUNTIE_MEDU, Monster::AUNTIE_MEDU_ENHANCED, Monster::WEREWOLF});
    EXPECT_EQ(Hit(0, 12), floor_.unit().monster[0].hp);
    EXPECT_EQ(Hit(1, 0), floor_.unit().monster[1].hp); // even a hit that would do nothing
    EXPECT_EQ(Hit(2, 12), 12);
    EXPECT_EQ(Hit(0, 12, false), 12); // not a weapon hit (an item, a monster)
}

TEST_F(NoSerpent, SnakeSlayerNeedsTheSerpentSword) {
    Put({Monster::AUNTIE_MEDU});
    EXPECT_EQ(Hit(0, 12), 12);
}

TEST_F(SuperSteveSerpent, SuperSteveCarriesSnakeSlayer) {
    Put({Monster::AUNTIE_MEDU});
    floor_.status().cur_chara = CHARA_XIAO;
    EXPECT_EQ(Hit(0, 12), floor_.unit().monster[0].hp);
}

TEST_F(SerpentSword, TheSnakeComesWithTheGuard) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::SnakeCharmerUp());
    EXPECT_TRUE(g_loaded.empty());
    Guard(true);
    expanded::SnakeCharmerStep();
    ASSERT_TRUE(expanded::SnakeCharmerUp());
    ASSERT_EQ(g_loaded.size(), 1u);
    EXPECT_EQ(g_loaded[0], expanded::kKillerSnakeFile);
    Guard(false);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::SnakeCharmerUp());
}

TEST_F(SerpentSword, MonstersBackAwayUntilPastTheCalmRadius) {
    Put({Monster::WEREWOLF, Monster::DASHER});
    At(0, 0.0f, 60.0f);   // within the radius
    At(1, 0.0f, -100.0f); // outside it
    Guard(true);
    expanded::SnakeCharmerStep();
    auto away = Target(0);
    EXPECT_GT(away[2], 60.0f); // a point behind it, away from the player
    auto player = Target(1);
    EXPECT_FLOAT_EQ(player[2], 0.0f); // the other finds the player

    // A step toward the player is turned straight away; one already away is kept.
    MONSTOR &wolf = floor_.unit().monster[0];
    wolf.movement_speed = 1.0f;
    wolf.movement[0] = 0.6f, wolf.movement[2] = -0.8f;
    expanded::MonsterMoveChecked(floor_.unit(), 0);
    EXPECT_FLOAT_EQ(wolf.movement[0], 0.0f);
    EXPECT_FLOAT_EQ(wolf.movement[2], 1.0f);
    wolf.movement[0] = 0.8f, wolf.movement[2] = 0.6f;
    expanded::MonsterMoveChecked(floor_.unit(), 0);
    EXPECT_FLOAT_EQ(wolf.movement[0], 0.8f);

    // Out past the radius, it still backs off; past the calm radius it may come back.
    At(0, 0.0f, 100.0f);
    expanded::SnakeCharmerStep();
    EXPECT_GT(Target(0)[2], 100.0f);
    At(0, 0.0f, expanded::kSnakeCalmRadius + 5.0f);
    expanded::SnakeCharmerStep();
    EXPECT_FLOAT_EQ(Target(0)[2], 0.0f);

    // The guard down, none backs off.
    At(0, 0.0f, 60.0f);
    expanded::SnakeCharmerStep();
    Guard(false);
    expanded::SnakeCharmerStep();
    EXPECT_FLOAT_EQ(Target(0)[2], 0.0f);
}

TEST_F(SerpentSword, TheHeadAppearsIdlesAndHisses) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    CCharacter *head = expanded::EnterFloorCharacter(expanded::kKillerSnakeFile, expanded::kKillerSnakeCfg);
    ASSERT_NE(head, nullptr);
    // Its appearance played through, then the idle.
    for (int step = 1; step < kAppearSteps; step++) {
        ASSERT_EQ(head->motion_no, expanded::kSnakeAppear) << step;
        expanded::SnakeCharmerStep();
    }
    EXPECT_EQ(head->motion_no, expanded::kSnakeAppear);
    expanded::SnakeCharmerStep();
    EXPECT_EQ(head->motion_no, expanded::kSnakeIdle);
    // A hiss when a monster comes near, played through.
    At(0, 0.0f, 50.0f);
    expanded::SnakeCharmerStep();
    EXPECT_EQ(head->motion_no, expanded::kSnakeHiss);
    for (int step = 1; step < kHissSteps; step++) {
        expanded::SnakeCharmerStep();
        ASSERT_EQ(head->motion_no, expanded::kSnakeHiss) << step;
    }
    expanded::SnakeCharmerStep();
    EXPECT_EQ(head->motion_no, expanded::kSnakeIdle);
    // And now and then: after kSnakeHissPeriod steps of the idle (the one it came back on the first).
    for (int step = 2; step < expanded::kSnakeHissPeriod; step++) {
        expanded::SnakeCharmerStep();
        ASSERT_EQ(head->motion_no, expanded::kSnakeIdle) << step;
    }
    expanded::SnakeCharmerStep();
    EXPECT_EQ(head->motion_no, expanded::kSnakeHiss);
    // Down with the guard, up again with its appearance.
    Guard(false);
    expanded::SnakeCharmerStep();
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_EQ(head->motion_no, expanded::kSnakeAppear);
}

TEST_F(SuperSteveSerpent, SuperSteveCarriesSnakeCharmer) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 60.0f);
    floor_.status().cur_chara = CHARA_XIAO;
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_TRUE(expanded::SnakeCharmerUp());
    EXPECT_GT(Target(0)[2], 60.0f);
}

TEST_F(SerpentSword, OffWithTheWeaponAbilities) {
    Put({Monster::AUNTIE_MEDU});
    At(0, 0.0f, 60.0f);
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(Hit(0, 12), 12);
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::SnakeCharmerUp());
}
