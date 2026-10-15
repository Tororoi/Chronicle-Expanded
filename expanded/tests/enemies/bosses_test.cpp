#include <gtest/gtest.h>

#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace {

constexpr int kSkeletonSoldier = 1;
constexpr int kMinotaurJoe = 83;
constexpr int kDarkGenie = 84;

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
    void LoadFloor(bool staged, std::initializer_list<int> kinds) {
        expanded::FloorRosters rosters;
        for (int kind : kinds) {
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

    std::unique_ptr<CMonstorUnit> unit_ = std::make_unique<CMonstorUnit>();
    std::unique_ptr<CUserStatus>  status_ = std::make_unique<CUserStatus>();
    CUserStatus                  *saved_status_ = nullptr;
    s32                           saved_map_ = 0;
};

} // namespace

TEST_F(OutOfPlaceBoss, KeepsItsFloorPositionAndRaisesNoEvent) {
    LoadFloor(true, {kMinotaurJoe, kSkeletonSoldier});
    Spawn(0, 0);
    Spawn(1, 1);
    EXPECT_TRUE(expanded::BossOutOfPlace(0));
    EXPECT_TRUE(expanded::BossKeepsPlace(*unit_, 0));
    EXPECT_TRUE(expanded::BossEventHeld(0));
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
    unit_->model[0] = MonstorTable[kMinotaurJoe];
    expanded::EnemyModelSetUp(*unit_, 0, kMinotaurJoe);
    Spawn(0, 0);
    EXPECT_FALSE(expanded::BossOutOfPlace(0));
    EXPECT_FALSE(expanded::BossEventHeld(0));
    unit_->monster[0].hp = 0;
    EXPECT_FALSE(expanded::BossDeathStep(*unit_, 0));
}

TEST_F(OutOfPlaceBoss, ABeatenBossFadesOutAndDies) {
    LoadFloor(true, {kDarkGenie, kSkeletonSoldier});
    Spawn(0, 0);
    unit_->alive_count = 2;
    MONSTOR &boss = unit_->monster[0];
    boss.state = 2;
    EXPECT_FALSE(expanded::BossDeathStep(*unit_, 0));

    // Its death script runs first.
    boss.hp = 0;
    unit_->script_state[0] = 1;
    EXPECT_FALSE(expanded::BossDeathStep(*unit_, 0));
    unit_->script_state[0] = 0;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_FLOAT_EQ(boss.palette_alpha_step, 128.0f / expanded::kBossFadeFrames);
    EXPECT_EQ(boss.state, 2);

    boss.palette_alpha = 0.0f;
    EXPECT_TRUE(expanded::BossDeathStep(*unit_, 0));
    EXPECT_EQ(boss.state, -1);
    EXPECT_EQ(unit_->alive_count, 1);
}

TEST(BossCollapse, MotionsOfTheBossesThatHaveOne) {
    EXPECT_EQ(expanded::BossCollapseMotion(kMinotaurJoe), 9);
    EXPECT_EQ(expanded::BossCollapseMotion(79), 13);
    EXPECT_EQ(expanded::BossCollapseMotion(kDarkGenie), -1);
}
