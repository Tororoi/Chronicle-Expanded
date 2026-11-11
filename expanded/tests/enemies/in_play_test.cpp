#include <gtest/gtest.h>

#include "character.hpp"
#include "collision.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/in_play.hpp"
#include "expanded/enemies/monsters.hpp"
#include "frame.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

using expanded::Monster;

namespace {

// Six Skeleton Soldiers 20 to 70 units from the player at the origin, all in view, on a floor short of the last.
class MonstersInPlay : public ::testing::Test {
protected:
    void SetUp() override {
        for (int i = 0; i < 3; i++) {
            saved_pos_[i] = CharaMain.pos[i];
        }
        CharaMain.SetPosition(0.0f, 0.0f, 0.0f);
        saved_map_ = selectMapNo;
        selectMapNo = 0;
        floor_.status().cur_floor = 0;
        floor_.SetUpModel(0, Monster::SKELETON_SOLDIER);
        for (int m = 0; m < kMonsters; m++) {
            floor_.Spawn(m, 0);
            MONSTOR &live = floor_.unit().monster[m];
            live.state = 2;
            live.revealed = -1;
            live.clip_distance = 200.0f;
            floor_.unit().chara[m][0].frame = nullptr;
            floor_.unit().chara[m][0].SetPosition(20.0f + 10.0f * static_cast<float>(m), 0.0f, 0.0f);
        }
    }

    void TearDown() override {
        selectMapNo = saved_map_;
        CharaMain.SetPosition(saved_pos_[0], saved_pos_[1], saved_pos_[2]);
    }

    static constexpr int kMonsters = 6;

    expanded::test::TestFloor floor_;
    float                     saved_pos_[3] = {};
    int                       saved_map_ = 0;
};

} // namespace

TEST_F(MonstersInPlay, EveryMonsterInViewTakesPartNotTheFourNearest) {
    ASSERT_GT(maxFloorTbl__3[selectMapNo], 1); // not the last floor, where the game lets them all take part already
    floor_.unit().CheckViewLevel();
    for (int m = 0; m < kMonsters; m++) {
        EXPECT_EQ(floor_.unit().monster[m].state, 2) << m;
        EXPECT_FALSE(floor_.unit().monster[m].view_held) << m;
    }
}

TEST(MonsterPolys, AnotherBodyGoesInOnlyAsFarAsThereIsRoom) {
    CFrame  body;
    CBoxVu0 box{};
    CCPoly  list[expanded::kMonsterPolys];
    EXPECT_EQ(expanded::PickUpBodyPolys(body, list, expanded::kMonsterPolys, box), 0); // full: nothing taken
    EXPECT_EQ(expanded::PickUpBodyPolys(body, list, expanded::kMonsterPolys + 5, box), 0);
    EXPECT_EQ(expanded::PickUpBodyPolys(body, list, 0, box), 0); // a frame with no collision has none
}
