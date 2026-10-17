#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/data/boss_model.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::No;

// A Divine Beast Cave floor laid out by a staged roster, or by its own.
class OutOfPlaceBoss : public ::testing::Test {
protected:
    void SetUp() override {
        saved_status_ = UserStatus;
        saved_map_ = selectMapNo;
        UserStatus = status_.get();
        expanded::RosterVisitBegun();
        for (MONSTOR &monster : unit_->monster) {
            monster.state = -1;
        }
    }

    void TearDown() override {
        expanded::RosterVisitBegun();
        UserStatus = saved_status_;
        selectMapNo = saved_map_;
    }

    // BtLoadMonstor's read of floor 2's front, then SetupBaseModel's part for each kind.
    void LoadFloor(bool staged, std::initializer_list<Monster> kinds) {
        expanded::FloorRosters rosters;
        for (Monster kind : kinds) {
            rosters.front.Add(kind, expanded::SpawnsOnceNatively(kind));
        }
        if (staged) {
            expanded::StageFloor(2, rosters);
        }
        selectMapNo = 0;
        UserStatus->cur_floor = 2;
        expanded::RosterFloorLayout(BtEnemyLayoutList[0] + 2, 0);
        for (int slot = 0; slot < rosters.front.Size(); slot++) {
            unit_->model[slot] = MonstorTable[rosters.front.model_no[slot]];
            expanded::EnemyModelSetUp(*unit_, slot, rosters.front.model_no[slot]);
        }
    }

    // SetupViewMonstor's part: a dormant monster of a slot.
    void Spawn(int monster, int slot) {
        MONSTOR &live = unit_->monster[monster];
        live.state = 1;
        live.base_model = slot;
        live.max_hp = live.hp = unit_->model[slot].max_hp;
        live.palette_alpha = 128.0f;
        expanded::EnemySpawned(*unit_, monster);
    }

    // A pack's KEY table of count motions: Joe's disc death, and his built fall and loop when it has them.
    void GiveMotions(int monster, int count) {
        motions_.assign(count + 1, MOTION_INFO{.start = 10, .end = 20, .speed = 0.2f});
        motions_[expanded::kJoeDiscDeath] = {.start = 300, .end = 330, .speed = 0.15f};
        if (count > expanded::kJoeDeathLoopKey) {
            motions_[expanded::kJoeFallKey] = {.start = 370, .end = 420, .speed = 0.2f};
            motions_[expanded::kJoeDeathLoopKey] = {.start = 420, .end = 420, .speed = 0.0f};
        }
        motions_[count] = {.start = -1, .end = -1, .speed = -1.0f};
        unit_->chara[monster][0].motion_type.motion_info = motions_.data();
    }

    // The fall played to its last frame.
    void FinishMotion(int monster) {
        CCharacter &body = unit_->chara[monster][0];
        body.motion_type.state.time = static_cast<float>(body.motion_type.motion_info[body.motion_no].end);
    }

    std::vector<MOTION_INFO>      motions_;
    std::unique_ptr<CMonstorUnit> unit_ = std::make_unique<CMonstorUnit>();
    std::unique_ptr<CUserStatus>  status_ = std::make_unique<CUserStatus>();
    CUserStatus                  *saved_status_ = nullptr;
    s32                           saved_map_ = 0;
};

} // namespace

TEST_F(OutOfPlaceBoss, KeepsItsFloorPositionAndRaisesNoEvent) {
    LoadFloor(true, {Monster::MINOTAUR_JOE, Monster::SKELETON_SOLDIER});
    Spawn(0, 0);
    Spawn(1, 1);
    EXPECT_TRUE(expanded::BossOutOfPlace(0));
    EXPECT_TRUE(expanded::BossKeepsPlace(*unit_, 0));
    EXPECT_TRUE(expanded::BossEventHeld(0));
    // No crowd cheers him off his arena; his own cries still sound.
    EXPECT_TRUE(expanded::BossSoundHeld(0, 1798));
    EXPECT_FALSE(expanded::BossSoundHeld(0, 1811));
    EXPECT_FALSE(expanded::BossSoundHeld(1, 1798));
    EXPECT_FALSE(expanded::BossOutOfPlace(1));
    EXPECT_FALSE(expanded::BossKeepsPlace(*unit_, 1));

    // Awake, it moves as its script says until beaten.
    unit_->monster[0].state = 2;
    EXPECT_FALSE(expanded::BossKeepsPlace(*unit_, 0));
    unit_->monster[0].hp = 0;
    EXPECT_TRUE(expanded::BossKeepsPlace(*unit_, 0));
}

TEST_F(OutOfPlaceBoss, ABossOnItsOwnFloorIsLeftAlone) {
    LoadFloor(false, {});
    unit_->model[0] = MonstorTable[No(Monster::MINOTAUR_JOE)];
    expanded::EnemyModelSetUp(*unit_, 0, No(Monster::MINOTAUR_JOE));
    Spawn(0, 0);
    EXPECT_FALSE(expanded::BossOutOfPlace(0));
    EXPECT_FALSE(expanded::BossEventHeld(0));
    EXPECT_FALSE(expanded::BossSoundHeld(0, 1798));
    unit_->monster[0].hp = 0;
    EXPECT_FALSE(expanded::BossDeathStep(*unit_, 0));
}

TEST_F(OutOfPlaceBoss, OnlyJoeUtanTheIceQueenKingsCurseAndTheBlackKnightAreSupported) {
    EXPECT_TRUE(expanded::BossSupported(Monster::MINOTAUR_JOE));
    EXPECT_TRUE(expanded::BossSupported(Monster::MASTER_UTAN));
    EXPECT_TRUE(expanded::BossSupported(Monster::ICE_QUEEN));
    EXPECT_TRUE(expanded::BossSupported(Monster::KINGS_CURSE_COFFIN));
    EXPECT_TRUE(expanded::BossSupported(Monster::BLACK_KNIGHT));
    for (Monster boss : {Monster::DRAN, Monster::DARK_GENIE}) {
        EXPECT_FALSE(expanded::BossSupported(boss)) << No(boss);
    }

    // Another boss a staged roster places plays as on its arena.
    LoadFloor(true, {Monster::DRAN, Monster::SKELETON_SOLDIER});
    Spawn(0, 0);
    EXPECT_FALSE(expanded::BossOutOfPlace(0));
    EXPECT_FALSE(expanded::BossEventHeld(0));
    unit_->monster[0].hp = 0;
    EXPECT_FALSE(expanded::BossDeathStep(*unit_, 0));
}

TEST_F(OutOfPlaceBoss, CountsAPacksMotions) {
    GiveMotions(0, 13);
    EXPECT_EQ(expanded::MotionCount(unit_->chara[0][0]), 13);
    GiveMotions(0, 11);
    EXPECT_EQ(expanded::MotionCount(unit_->chara[0][0]), 11);
    unit_->chara[0][0].motion_type.motion_info = nullptr;
    EXPECT_EQ(expanded::MotionCount(unit_->chara[0][0]), 0);
}

TEST_F(OutOfPlaceBoss, BeatenJoeFallsLiesAndFadesOut) {
    LoadFloor(true, {Monster::MINOTAUR_JOE, Monster::SKELETON_SOLDIER});
    Spawn(0, 0);
    GiveMotions(0, 13);
    unit_->alive_count = 2;
    MONSTOR &joe = unit_->monster[0];
    joe.state = 2;
    joe.movement_speed = 1.5f;
    joe.turn_speed = 0.1f;
    EXPECT_FALSE(expanded::BossDeathStep(*unit_, 0));

    // Beaten, he stops, takes no hits and falls once, his death script held.
    joe.hp = 0;
    unit_->script_state[0] = 1;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(joe.movement_speed, 0.0f);
    EXPECT_EQ(joe.turn_speed, 0.0f);
    EXPECT_EQ(joe.invincible_timer, 100);
    EXPECT_EQ(joe.requested_motion, expanded::kJoeFallKey);
    EXPECT_EQ(joe.requested_motion_flags, 2);
    EXPECT_EQ(unit_->chara[0][0].motion_no, expanded::kJoeFallKey);
    EXPECT_TRUE(expanded::BossKeepsPlace(*unit_, 0));

    // Still falling: no fade yet.
    unit_->chara[0][0].motion_type.state.time = 400.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(joe.palette_alpha_step, 0.0f);

    // On the fall's last frame he lies in the loop and fades.
    FinishMotion(0);
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(joe.requested_motion, expanded::kJoeDeathLoopKey);
    EXPECT_EQ(joe.requested_motion_flags, 0);
    EXPECT_FLOAT_EQ(joe.palette_alpha_step, 128.0f / expanded::kBossFadeFrames);
    EXPECT_EQ(joe.state, 2);

    // Faded out, he dies as _STATUS_SET_DEAD has a monster die.
    joe.palette_alpha = 0.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(joe.state, -1);
    EXPECT_EQ(unit_->alive_count, 1);
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(unit_->alive_count, 1);
}

TEST_F(OutOfPlaceBoss, WithoutTheBuiltModelJoeFallsWithHisDiscDeath) {
    LoadFloor(true, {Monster::MINOTAUR_JOE});
    Spawn(0, 0);
    GiveMotions(0, 11);
    unit_->alive_count = 1;
    MONSTOR &joe = unit_->monster[0];
    joe.state = 2;
    joe.hp = 0;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(joe.requested_motion, expanded::kJoeDiscDeath);
    EXPECT_EQ(joe.requested_motion_flags, 2);

    FinishMotion(0);
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(joe.requested_motion, expanded::kJoeDiscDeath);
    EXPECT_FLOAT_EQ(joe.palette_alpha_step, 128.0f / expanded::kBossFadeFrames);

    joe.palette_alpha = 0.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(joe.state, -1);
    EXPECT_EQ(unit_->alive_count, 0);
}

TEST_F(OutOfPlaceBoss, BeatenUtanFallsWithHisDeathAndFadesOnItsLastFrame) {
    LoadFloor(true, {Monster::MASTER_UTAN, Monster::WEREWOLF});
    Spawn(0, 0);
    GiveMotions(0, 15);
    unit_->alive_count = 2;
    MONSTOR &utan = unit_->monster[0];
    EXPECT_TRUE(expanded::BossOutOfPlace(0));
    EXPECT_TRUE(expanded::BossKeepsPlace(*unit_, 0));
    utan.state = 2;
    utan.hp = 0;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(utan.invincible_timer, 100);
    EXPECT_EQ(utan.requested_motion, 13);
    EXPECT_EQ(utan.requested_motion_flags, 2);

    // No death loop: he stays on the fall's last frame while fading.
    FinishMotion(0);
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(utan.requested_motion, 13);
    EXPECT_FLOAT_EQ(utan.palette_alpha_step, 128.0f / expanded::kBossFadeFrames);

    utan.palette_alpha = 0.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(utan.state, -1);
    EXPECT_EQ(unit_->alive_count, 1);
}

TEST_F(OutOfPlaceBoss, BeatenBlackKnightBowsWrithesAndFadesWhenTheLightComes) {
    // The light's pack, not on the test's disc, never loads.
    expanded::SetShotEffectLoader([](CSHOT_EFFECT &, CDataAlloc2<1> &, BT_SHOT_EFFECT *, int, const char *) { return false; });
    expanded::ShotEffectsClear();
    LoadFloor(true, {Monster::BLACK_KNIGHT});
    Spawn(0, 0);
    GiveMotions(0, expanded::kBlackKnightBowKey + expanded::kBlackKnightDeathKeys);
    unit_->alive_count = 1;
    MONSTOR &knight = unit_->monster[0];
    knight.state = 2;
    knight.hp = 0;

    // Head bowed, its loop once, the throes, then their loop.
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(knight.requested_motion, expanded::kBlackKnightBowKey);
    for (int key = 1; key < expanded::kBlackKnightDeathKeys; key++) {
        FinishMotion(0);
        EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
        EXPECT_EQ(knight.requested_motion, expanded::kBlackKnightBowKey + key);
        EXPECT_EQ(knight.requested_motion_flags, key == expanded::kBlackKnightDeathKeys - 1 ? 0 : 2);
    }
    EXPECT_EQ(knight.palette_alpha_step, 0.0f);

    // The light (not loaded here) is due 195 steps into the throes, counted from the step they began (two steps
    // have passed with the loop's start): without it, he fades then.
    for (int step = 2; step < 195; step++) {
        EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    }
    EXPECT_EQ(knight.palette_alpha_step, 0.0f);
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_FLOAT_EQ(knight.palette_alpha_step, 128.0f / expanded::kBossFadeFrames);
    knight.palette_alpha = 0.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(knight.state, -1);
    EXPECT_EQ(unit_->alive_count, 0);
    expanded::SetShotEffectLoader(nullptr);
}

TEST(BossPack, JoeLoadsHisBuiltModelWhenTheDataHasIt) {
    char filename[64] = "dun/monstor/c16a.chr";
    expanded::BossPackFile(filename, No(Monster::DRAN));
    EXPECT_STREQ(filename, "dun/monstor/c16a.chr");
    std::error_code error;
    bool            built = std::filesystem::is_regular_file(PathsDataRoot() / expanded::kJoeModelFile, error);
    expanded::BossPackFile(filename, No(Monster::MINOTAUR_JOE));
    EXPECT_STREQ(filename, built ? expanded::kJoeModelFile : "dun/monstor/c16a.chr");
}
