#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "runscript_opcodes.hpp"
#include "shot_effect.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::Monster;
using expanded::No;

bool FakeLoad(CSHOT_EFFECT &effect, CDataAlloc2<1> &, BT_SHOT_EFFECT *description, int shots, const char *) {
    effect.Initialize();
    effect.effect_data = description;
    effect.slot_count = shots;
    return true;
}

// A Divine Beast Cave floor laid out by a staged roster, or by its own, with Ruby and Ungaga in the party or not.
class BossCasts : public ::testing::Test {
protected:
    void SetUp() override {
        UserStatus->party_size = CHARA_UNGAGA + 1;
        expanded::SetShotEffectLoader(FakeLoad);
        expanded::ShotEffectsClear();
        expanded::RosterVisitBegun();
    }

    void TearDown() override {
        expanded::RosterVisitBegun();
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
    }

    const expanded::Roster &LoadFloor(bool staged, std::initializer_list<Monster> kinds) { return floor_.LoadRoster(staged, kinds); }

    void Spawn(int monster, int slot) { floor_.Spawn(monster, slot); }

    expanded::test::TestFloor floor_;
    CMonstorUnit             *unit_ = &floor_.unit();
};

} // namespace

// The ints every monster script shares, as the game keeps them.
int shared_ints_[10];

TEST_F(BossCasts, ComesOnlyWithRubyInThePartyAndBringsHerBarrier) {
    UserStatus->party_size = CHARA_RUBY;
    const expanded::Roster &without = LoadFloor(true, {Monster::ICE_QUEEN, Monster::CAVE_BAT});
    EXPECT_FALSE(without.Contains(Monster::ICE_QUEEN));
    EXPECT_FALSE(without.Contains(Monster::ICE_BARRIER));
    EXPECT_TRUE(without.Contains(Monster::CAVE_BAT));

    expanded::RosterVisitBegun();
    UserStatus->party_size = CHARA_RUBY + 1;
    const expanded::Roster &with = LoadFloor(true, {Monster::ICE_QUEEN, Monster::CAVE_BAT});
    EXPECT_EQ(with.model_no[0], No(Monster::ICE_QUEEN));
    EXPECT_EQ(with.model_no[1], No(Monster::CAVE_BAT));
    EXPECT_EQ(with.model_no[2], No(Monster::ICE_BARRIER));
    EXPECT_TRUE(with.once[2]);
}

TEST_F(BossCasts, StaysAwayWhenTheRosterHasNoRoomForHerBarrier) {
    expanded::Roster full;
    for (Monster kind : {Monster::ICE_QUEEN, Monster::CAVE_BAT, Monster::WEREWOLF, Monster::HORNET, Monster::MASTER_JACKET,
                         Monster::DRAGON, Monster::GHOST, Monster::SKELETON_SOLDIER, Monster::STATUE}) {
        full.Add(kind, false);
    }
    expanded::BossCastRoster(full);
    EXPECT_FALSE(full.Contains(Monster::ICE_QUEEN));
    EXPECT_EQ(full.Size(), expanded::kRosterSize - 1);
    EXPECT_EQ(full.model_no[0], No(Monster::CAVE_BAT));
}

TEST_F(BossCasts, HerScriptAndHerBarriersTalkThroughTheirOwnMailbox) {
    LoadFloor(true, {Monster::CAVE_BAT, Monster::ICE_QUEEN});
    Spawn(0, 0);
    Spawn(4, 1);
    Spawn(7, 2);
    EXPECT_EQ(expanded::BossCastMonster(Monster::ICE_QUEEN), 4);
    EXPECT_EQ(expanded::BossCastMonster(Monster::ICE_BARRIER), 7);
    int *mailbox = expanded::BossCastGlobals(4, shared_ints_);
    EXPECT_NE(mailbox, shared_ints_);
    EXPECT_EQ(expanded::BossCastGlobals(7, shared_ints_), mailbox);
    EXPECT_EQ(expanded::BossCastGlobals(0, shared_ints_), shared_ints_);

    // In her cast's scripts monster 0 is her and 1 her barrier; anyone else's are the floor's.
    EXPECT_EQ(expanded::BossCastMonsterRef(7, 0), 4);
    EXPECT_EQ(expanded::BossCastMonsterRef(4, 1), 7);
    EXPECT_EQ(expanded::BossCastMonsterRef(7, 3), 3);
    EXPECT_EQ(expanded::BossCastMonsterRef(0, 0), 0);

    // Breaking her barrier is not a kill.
    EXPECT_FALSE(expanded::BossCastCountsKill(7));
    EXPECT_TRUE(expanded::BossCastCountsKill(4));
    EXPECT_TRUE(expanded::BossCastCountsKill(0));
}

TEST_F(BossCasts, OnHerOwnFloorHerScriptUsesTheGlobals) {
    LoadFloor(false, {});
    unit_->model[0] = MonstorTable[No(Monster::ICE_QUEEN)];
    expanded::EnemyModelSetUp(*unit_, 0, No(Monster::ICE_QUEEN));
    Spawn(0, 0);
    EXPECT_EQ(expanded::BossCastGlobals(0, shared_ints_), shared_ints_);
    EXPECT_TRUE(expanded::BossCastCountsKill(0));
}

TEST_F(BossCasts, HerDefeatTakesHerBarrierAwayWithoutAKill) {
    LoadFloor(true, {Monster::ICE_QUEEN, Monster::CAVE_BAT});
    Spawn(0, 0);
    Spawn(1, 1);
    Spawn(2, 2);
    ASSERT_EQ(unit_->alive_count, 3);
    std::vector<MOTION_INFO> motions(14, MOTION_INFO{.start = 10, .end = 20, .speed = 0.2f});
    motions[13] = {.start = -1, .end = -1, .speed = -1.0f};
    unit_->chara[0][0].motion_type.motion_info = motions.data();
    unit_->monster[0].state = 2;
    unit_->monster[0].hp = 0;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(unit_->monster[2].state, -1);
    EXPECT_EQ(unit_->monster[1].state, 1);
    EXPECT_EQ(unit_->alive_count, 2);
    EXPECT_EQ(unit_->monster[0].requested_motion, 11);

    // On her death's last frame she lies in its loop and fades.
    unit_->chara[0][0].motion_type.state.time = 20.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(unit_->monster[0].requested_motion, 12);
    EXPECT_EQ(unit_->monster[0].requested_motion_flags, 0);
    EXPECT_FLOAT_EQ(unit_->monster[0].palette_alpha_step, 128.0f / expanded::kBossFadeFrames);
}

TEST_F(BossCasts, KingsCurseComesWholeOnlyWithUngagaInTheParty) {
    UserStatus->party_size = CHARA_UNGAGA;
    const expanded::Roster &without = LoadFloor(true, {Monster::KINGS_CURSE, Monster::CAVE_BAT});
    EXPECT_FALSE(without.Contains(Monster::KINGS_CURSE));
    EXPECT_FALSE(without.Contains(Monster::KINGS_CURSE_COFFIN));

    expanded::RosterVisitBegun();
    UserStatus->party_size = CHARA_UNGAGA + 1;
    const expanded::Roster &with = LoadFloor(true, {Monster::KINGS_CURSE, Monster::CAVE_BAT});
    EXPECT_EQ(with.model_no[2], No(Monster::KINGS_CURSE_COFFIN));
    EXPECT_TRUE(with.once[0]);
    EXPECT_TRUE(with.once[2]);
}

TEST_F(BossCasts, TheCoffinAndTheKingFindEachOtherAndTheCoffinHidesUnderHim) {
    LoadFloor(true, {Monster::CAVE_BAT, Monster::KINGS_CURSE_COFFIN});
    Spawn(0, 0);
    Spawn(5, 1); // the coffin, its scripts' monster 0
    Spawn(9, 2); // the king, monster 1
    EXPECT_EQ(expanded::BossCastMonsterRef(5, 1), 9);
    EXPECT_EQ(expanded::BossCastMonsterRef(9, 0), 5);
    EXPECT_EQ(expanded::BossCastMonsterRef(0, 1), 1);
    EXPECT_EQ(expanded::BossCastGlobals(5, shared_ints_), expanded::BossCastGlobals(9, shared_ints_));
    EXPECT_FALSE(expanded::BossCastCountsKill(9));
    EXPECT_TRUE(expanded::BossCastCountsKill(5));

    unit_->chara[9][0].SetPosition(300.0f, 4.0f, 700.0f);
    float hide[3] = {0.0f, 0.0f, -500.0f};
    EXPECT_FALSE(expanded::BossPlace(*unit_, 5, hide));
    EXPECT_FLOAT_EQ(hide[0], 300.0f);
    EXPECT_FLOAT_EQ(hide[1], -496.0f);
    EXPECT_FLOAT_EQ(hide[2], 700.0f);

    // Setting up, the king keeps the floor's spot; awake, he goes where his script says.
    float spot[3] = {0.0f, 0.0f, -237.0f};
    EXPECT_TRUE(expanded::BossPlace(*unit_, 9, spot));
    unit_->monster[9].state = 2;
    EXPECT_FALSE(expanded::BossPlace(*unit_, 9, spot));
    EXPECT_FLOAT_EQ(spot[2], -237.0f);

    // Dormant, the coffin waits under the king, to wake with him.
    expanded::BossCastStep(*unit_);
    float coffin[4];
    unit_->chara[5][0].GetPosition(coffin);
    EXPECT_FLOAT_EQ(coffin[1], -496.0f);
}

TEST_F(BossCasts, TheCoffinsDefeatTakesTheKingAway) {
    LoadFloor(true, {Monster::KINGS_CURSE_COFFIN});
    Spawn(0, 0);
    Spawn(1, 1);
    std::vector<MOTION_INFO> motions(8, MOTION_INFO{.start = 10, .end = 20, .speed = 0.2f});
    motions[7] = {.start = -1, .end = -1, .speed = -1.0f};
    unit_->chara[0][0].motion_type.motion_info = motions.data();
    unit_->monster[0].state = 2;
    unit_->monster[0].hp = 0;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(unit_->monster[1].state, -1);
    EXPECT_EQ(unit_->alive_count, 1);
    EXPECT_EQ(unit_->monster[0].requested_motion, 5);

    unit_->chara[0][0].motion_type.state.time = 20.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(unit_->monster[0].requested_motion, 6);
    EXPECT_EQ(unit_->monster[0].requested_motion_flags, 0);
}

TEST_F(BossCasts, TheMountBringsTheBlackKnightWhoMayAlsoComeAlone) {
    const expanded::Roster &mounted = LoadFloor(true, {Monster::BLACK_KNIGHT_MOUNT, Monster::CAVE_BAT});
    EXPECT_EQ(mounted.model_no[2], No(Monster::BLACK_KNIGHT));

    expanded::RosterVisitBegun();
    const expanded::Roster &alone = LoadFloor(true, {Monster::BLACK_KNIGHT, Monster::CAVE_BAT});
    EXPECT_FALSE(alone.Contains(Monster::BLACK_KNIGHT_MOUNT));
    EXPECT_TRUE(alone.once[0]);

    // Alone, he starts as the mount leaves him: dismounted, and his monster 0 is himself.
    Spawn(3, 0);
    EXPECT_EQ(expanded::BossCastMailbox(Monster::BLACK_KNIGHT)[0], 1);
    EXPECT_EQ(expanded::BossCastMonsterRef(3, 0), 3);
    EXPECT_EQ(expanded::BossCastMonsterRef(3, 1), 3);
}

TEST_F(BossCasts, TheRidingKnightSitsOnTheMountAndWakesByIt) {
    LoadFloor(true, {Monster::BLACK_KNIGHT_MOUNT});
    Spawn(2, 0);
    Spawn(6, 1);
    EXPECT_EQ(expanded::BossCastMailbox(Monster::BLACK_KNIGHT)[0], 0);
    EXPECT_EQ(expanded::BossCastMonsterRef(6, 0), 2);
    unit_->chara[2][0].SetPosition(500.0f, 0.0f, 900.0f);
    unit_->chara[6][0].SetPosition(40.0f, 0.0f, 80.0f);

    expanded::BossCastAttached(*unit_, 6, 2);
    float seat[4];
    unit_->chara[6][0].GetPosition(seat);
    EXPECT_FLOAT_EQ(seat[0], 0.0f);
    float wake[4] = {};
    expanded::BossCastWakePosition(*unit_, 6, wake);
    EXPECT_FLOAT_EQ(wake[0], 500.0f);
    EXPECT_FLOAT_EQ(wake[2], 900.0f);

    // Let go, he wakes by where he stands.
    expanded::BossCastAttached(*unit_, 6, -1);
    float own[4] = {7.0f, 0.0f, 7.0f, 1.0f};
    expanded::BossCastWakePosition(*unit_, 6, own);
    EXPECT_FLOAT_EQ(own[0], 7.0f);
    EXPECT_FALSE(expanded::BossCastCountsKill(2));
}
