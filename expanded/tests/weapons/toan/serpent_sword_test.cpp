#include <gtest/gtest.h>

#include <array>
#include <initializer_list>
#include <string>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/character_light.hpp"
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
            floor_.unit().chara[slot][0].motion_type.motion_info = monster_clips_;
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

    MOTION_INFO               monster_clips_[20] = {};
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

TEST_F(SerpentSword, MostKindsBackOffOnTheirBackstep) {
    Put({Monster::WEREWOLF, Monster::AUNTIE_MEDU, Monster::HALLOWEEN, Monster::CAVE_BAT, Monster::MIMIC_DBC});
    EXPECT_EQ(expanded::SnakeBackOffMotion(floor_.unit(), 0), 2);
    EXPECT_EQ(expanded::SnakeBackOffMotion(floor_.unit(), 1), 4);
    EXPECT_EQ(expanded::SnakeBackOffMotion(floor_.unit(), 2), 13);
    EXPECT_EQ(expanded::SnakeBackOffMotion(floor_.unit(), 3), -1); // none
    EXPECT_EQ(expanded::SnakeBackOffMotion(floor_.unit(), 4), -1); // a mimic's script is its own
}

TEST_F(SerpentSword, MonstersBackOffUntilPastTheCalmRadius) {
    Put({Monster::WEREWOLF, Monster::DASHER});
    CMonstorUnit &unit = floor_.unit();
    At(0, 0.0f, 60.0f);   // within the radius
    At(1, 0.0f, -100.0f); // outside it
    unit.script_state[0] = 1;
    Guard(true);
    expanded::SnakeCharmerStep();

    // Taken from its script, it plays its backstep stepping away, facing the player; the other is left to its own.
    ASSERT_TRUE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 0);
    EXPECT_EQ(unit.monster[0].requested_motion, 2);
    EXPECT_FLOAT_EQ(unit.monster[0].movement[2], 1.0f);
    EXPECT_FLOAT_EQ(unit.monster[0].movement_speed, expanded::kSnakeBackSpeed);
    EXPECT_FLOAT_EQ(unit.monster[0].turn_target[2], 0.0f);
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 1));

    // A hit's reaction runs on its script; then it backs off again.
    unit.script_state[0] = 1;
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    unit.script_state[0] = 0;
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));

    // Out past the radius it still backs off; past the calm radius its script takes it back from the top.
    At(0, 0.0f, 100.0f);
    expanded::SnakeCharmerStep();
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));
    At(0, 0.0f, expanded::kSnakeCalmRadius + 5.0f);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 0);

    // The guard down, none backs off.
    At(0, 0.0f, 60.0f);
    expanded::SnakeCharmerStep();
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));
    Guard(false);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
}

TEST_F(SerpentSword, KindsWithoutABackstepStopComingOn) {
    Put({Monster::CAVE_BAT});
    CMonstorUnit &unit = floor_.unit();
    At(0, 0.0f, 60.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    MONSTOR &bat = unit.monster[0];
    // Straight at the player: it stops.
    bat.movement_speed = 1.0f;
    bat.movement[0] = 0.0f, bat.movement[2] = -1.0f;
    expanded::MonsterMoveChecked(unit, 0);
    EXPECT_FLOAT_EQ(bat.movement_speed, 0.0f);
    // Slanting in: only the sideways part is left.
    bat.movement_speed = 1.0f;
    bat.movement[0] = 0.6f, bat.movement[2] = -0.8f;
    expanded::MonsterMoveChecked(unit, 0);
    EXPECT_FLOAT_EQ(bat.movement[0], 1.0f);
    EXPECT_NEAR(bat.movement[2], 0.0f, 1e-6f);
    EXPECT_FLOAT_EQ(bat.movement_speed, 0.6f);
    // Away, as its script would have it: kept.
    bat.movement_speed = 1.0f;
    bat.movement[0] = 0.0f, bat.movement[2] = 1.0f;
    expanded::MonsterMoveChecked(unit, 0);
    EXPECT_FLOAT_EQ(bat.movement[2], 1.0f);
    EXPECT_FLOAT_EQ(bat.movement_speed, 1.0f);
}

TEST_F(SerpentSword, TheHeadFadesInAndOutWithTheGuard) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    Guard(true);
    for (int step = 1; step <= expanded::kSnakeFadeSteps; step++) {
        expanded::SnakeCharmerStep();
        EXPECT_NEAR(expanded::SnakeCharmerShown(), static_cast<float>(step) / expanded::kSnakeFadeSteps, 1e-5f) << step;
    }
    // It glows white as it shows, and others do not.
    CCharacter *head = expanded::EnterFloorCharacter(expanded::kKillerSnakeFile, expanded::kKillerSnakeCfg);
    ASSERT_NE(head, nullptr);
    float ambient[4] = {};
    expanded::CharacterLight(*head, ambient);
    EXPECT_FLOAT_EQ(ambient[0], expanded::kSnakeGlow);
    CCharacter other;
    float      none[4] = {};
    expanded::CharacterLight(other, none);
    EXPECT_FLOAT_EQ(none[0], 0.0f);
    // Down with the guard it fades out as long, the monsters free of it at once.
    Guard(false);
    for (int step = expanded::kSnakeFadeSteps - 1; step >= 0; step--) {
        expanded::SnakeCharmerStep();
        EXPECT_FALSE(expanded::SnakeCharmerUp());
        EXPECT_NEAR(expanded::SnakeCharmerShown(), static_cast<float>(step) / expanded::kSnakeFadeSteps, 1e-5f) << step;
    }
    float after[4] = {};
    expanded::CharacterLight(*head, after);
    EXPECT_FLOAT_EQ(after[0], 0.0f);
}

TEST_F(SerpentSword, TheHeadAppearsThenOpensItsMouthNowAndThen) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    CCharacter *head = expanded::EnterFloorCharacter(expanded::kKillerSnakeFile, expanded::kKillerSnakeCfg);
    ASSERT_NE(head, nullptr);
    // The appearance, held on its last frame from then on.
    EXPECT_EQ(head->motion_no, expanded::kSnakeAppear);
    EXPECT_NE(head->motion_flags & 2, 0);
    for (int step = 1; step < kAppearSteps + 2 * expanded::kSnakeMouthPeriod; step++) {
        expanded::SnakeCharmerStep();
        ASSERT_EQ(head->motion_no, expanded::kSnakeAppear) << step;
    }
    // The mouth shut after the appearance until its cycle comes round, then open, held and shut again.
    for (int step = 0; step < expanded::kSnakeMouthPeriod; step++) {
        expanded::SnakeCharmerStep();
        if (expanded::SnakeCharmerMouth() > 0.99f) {
            break;
        }
    }
    ASSERT_GT(expanded::SnakeCharmerMouth(), 0.99f);
    for (int step = 0; step < expanded::kSnakeMouthHold + expanded::kSnakeMouthEase; step++) {
        expanded::SnakeCharmerStep();
    }
    EXPECT_LT(expanded::SnakeCharmerMouth(), 0.01f);
    // A monster coming near opens it at once.
    expanded::SnakeCharmerStep();
    At(0, 0.0f, 50.0f);
    for (int step = 0; step < expanded::kSnakeMouthEase; step++) {
        expanded::SnakeCharmerStep();
    }
    EXPECT_GT(expanded::SnakeCharmerMouth(), 0.99f);
    // Gone with the guard, up again with its appearance.
    Guard(false);
    for (int step = 0; step < expanded::kSnakeFadeSteps; step++) {
        expanded::SnakeCharmerStep();
    }
    Guard(true);
    head->motion_no = 0;
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
    EXPECT_TRUE(expanded::MonsterStepTaken(floor_.unit(), 0));
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
