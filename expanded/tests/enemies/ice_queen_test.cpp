#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/ice_queen.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "runscript_opcodes.hpp"
#include "shot_effect.hpp"
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

// A Divine Beast Cave floor laid out by a staged roster, or by its own, with Ruby in the party or not.
class IceQueen : public ::testing::Test {
protected:
    void SetUp() override {
        saved_status_ = UserStatus;
        saved_map_ = selectMapNo;
        UserStatus = status_.get();
        UserStatus->party_size = CHARA_RUBY + 1;
        expanded::SetShotEffectLoader(FakeLoad);
        expanded::ShotEffectsClear();
        expanded::RosterVisitBegun();
        for (MONSTOR &monster : unit_->monster) {
            monster.state = -1;
        }
    }

    void TearDown() override {
        expanded::RosterVisitBegun();
        expanded::ShotEffectsClear();
        expanded::SetShotEffectLoader(nullptr);
        UserStatus = saved_status_;
        selectMapNo = saved_map_;
    }

    // BtLoadMonstor's read of floor 2's front (as staged), then SetupBaseModel's part for each kind.
    const expanded::Roster &LoadFloor(bool staged, std::initializer_list<Monster> kinds) {
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
        const expanded::Roster &loaded = staged ? expanded::StagedFloor(2)->front : rosters.front;
        for (int slot = 0; slot < loaded.Size(); slot++) {
            unit_->model[slot] = MonstorTable[loaded.model_no[slot]];
            expanded::EnemyModelSetUp(*unit_, slot, loaded.model_no[slot]);
        }
        return loaded;
    }

    // SetupViewMonstor's part: a dormant monster of a slot.
    void Spawn(int monster, int slot) {
        MONSTOR &live = unit_->monster[monster];
        live.state = 1;
        live.base_model = slot;
        live.max_hp = live.hp = unit_->model[slot].max_hp;
        expanded::EnemySpawned(*unit_, monster);
        unit_->alive_count++;
    }

    std::unique_ptr<CMonstorUnit> unit_ = std::make_unique<CMonstorUnit>();
    std::unique_ptr<CUserStatus>  status_ = std::make_unique<CUserStatus>();
    CUserStatus                  *saved_status_ = nullptr;
    s32                           saved_map_ = 0;
};

} // namespace

TEST_F(IceQueen, ComesOnlyWithRubyInThePartyAndBringsHerBarrier) {
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

TEST_F(IceQueen, StaysAwayWhenTheRosterHasNoRoomForHerBarrier) {
    expanded::Roster full;
    for (Monster kind : {Monster::ICE_QUEEN, Monster::CAVE_BAT, Monster::WEREWOLF, Monster::HORNET, Monster::MASTER_JACKET,
                         Monster::DRAGON, Monster::GHOST, Monster::SKELETON_SOLDIER, Monster::STATUE}) {
        full.Add(kind, false);
    }
    expanded::IceQueenRoster(full);
    EXPECT_FALSE(full.Contains(Monster::ICE_QUEEN));
    EXPECT_EQ(full.Size(), expanded::kRosterSize - 1);
    EXPECT_EQ(full.model_no[0], No(Monster::CAVE_BAT));
}

TEST_F(IceQueen, HerScriptAndHerBarriersTalkThroughTheirOwnMailbox) {
    LoadFloor(true, {Monster::CAVE_BAT, Monster::ICE_QUEEN});
    Spawn(0, 0);
    Spawn(4, 1);
    Spawn(7, 2);
    EXPECT_EQ(expanded::IceQueenMonster(), 4);
    int *mailbox = expanded::IceQueenGlobals(4, GL_INT);
    EXPECT_NE(mailbox, GL_INT);
    EXPECT_EQ(expanded::IceQueenGlobals(7, GL_INT), mailbox);
    EXPECT_EQ(expanded::IceQueenGlobals(0, GL_INT), GL_INT);

    // Her barrier's monster 0 is her; anyone else's is monster 0.
    EXPECT_EQ(expanded::IceQueenMonsterRef(7, 0), 4);
    EXPECT_EQ(expanded::IceQueenMonsterRef(7, 3), 3);
    EXPECT_EQ(expanded::IceQueenMonsterRef(4, 0), 0);
    EXPECT_EQ(expanded::IceQueenMonsterRef(0, 0), 0);

    // Breaking her barrier is not a kill.
    EXPECT_FALSE(expanded::IceQueenCountsKill(7));
    EXPECT_TRUE(expanded::IceQueenCountsKill(4));
    EXPECT_TRUE(expanded::IceQueenCountsKill(0));
}

TEST_F(IceQueen, OnHerOwnFloorHerScriptUsesTheGlobals) {
    LoadFloor(false, {});
    unit_->model[0] = MonstorTable[No(Monster::ICE_QUEEN)];
    expanded::EnemyModelSetUp(*unit_, 0, No(Monster::ICE_QUEEN));
    Spawn(0, 0);
    EXPECT_EQ(expanded::IceQueenGlobals(0, GL_INT), GL_INT);
    EXPECT_TRUE(expanded::IceQueenCountsKill(0));
}

TEST_F(IceQueen, HerDefeatTakesHerBarrierAwayWithoutAKill) {
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
}
