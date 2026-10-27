#include <gtest/gtest.h>

#include <algorithm>

#include "editloop.hpp"
#include "expanded/town/ladder.hpp"
#include "itemdata.hpp"

using expanded::TownLadder;
using expanded::TownLadderFrame;
using expanded::TownLadderSequence;

namespace {

// The bottom of an 80-high ladder at the origin, facing back along z.
ED_EVENT_PARAM Bottom() {
    ED_EVENT_PARAM param{};
    param.kind = ED_EVENT_POINT_LADDER_BOTTOM;
    param.position[3] = 1.0f;
    param.camera_pos[1] = 80.0f;
    param.camera_pos[2] = -14.0f;
    param.camera_pos[3] = 1.0f;
    param.rotation[1] = 3.14159265f;
    return param;
}

TownLadder FromBottom(int ally) {
    const float position[3] = {0.0f, 0.0f, 5.0f};
    return TownLadderSequence(ally, position, 0.0f, Bottom());
}

TownLadder FromTop(int ally) {
    const float position[3] = {0.0f, 80.0f, -14.0f};
    return TownLadderSequence(ally, position, 0.0f, Bottom());
}

bool Plays(const TownLadder &ladder, int motion) {
    return std::any_of(ladder.frames.begin(), ladder.frames.end(), [&](const TownLadderFrame &f) { return f.motion == motion; });
}

} // namespace

TEST(TownLadder, UngagaRefusesAtBothEnds) {
    for (const TownLadder &ladder : {FromBottom(CHARA_UNGAGA), FromTop(CHARA_UNGAGA)}) {
        ASSERT_EQ(ladder.frames.size(), 191u);
        EXPECT_FALSE(ladder.camera);
        EXPECT_EQ(ladder.frames.front().motion, 7);
        EXPECT_EQ(ladder.frames.front().flags, 2);
        EXPECT_EQ(ladder.frames.back().motion, 0);
        EXPECT_EQ(ladder.frames.back().z, ladder.frames.front().z);
    }
}

TEST(TownLadder, XiaoLeapsUpAndLandsOnHerOwn) {
    TownLadder up = FromBottom(CHARA_XIAO);
    ASSERT_FALSE(up.frames.empty());
    EXPECT_TRUE(up.camera);
    EXPECT_TRUE(Plays(up, 13));
    EXPECT_TRUE(Plays(up, 11));
    EXPECT_TRUE(Plays(up, 12));
    EXPECT_NEAR(up.frames.back().x, 0.0f, 0.01f);
    EXPECT_NEAR(up.frames.back().z, -14.0f, 0.01f);
    EXPECT_GT(up.frames.back().y, 80.0f);
    EXPECT_LT(up.frames.back().y, 84.0f);

    TownLadder down = FromTop(CHARA_XIAO);
    ASSERT_FALSE(down.frames.empty());
    EXPECT_FALSE(Plays(down, 13));
    EXPECT_NEAR(down.frames.back().z, 0.0f, 0.01f);
    EXPECT_GT(down.frames.back().y, 0.0f);
}

TEST(TownLadder, GoroHopsUpFourTimes) {
    TownLadder up = FromBottom(CHARA_GORO);
    int        hops = static_cast<int>(std::count_if(up.frames.begin(), up.frames.end(), [](const TownLadderFrame &f) { return f.sound == 361; }));
    EXPECT_EQ(hops, 4);
    EXPECT_TRUE(Plays(up, 11));
    EXPECT_TRUE(Plays(up, 12));
    EXPECT_EQ(up.frames.back().y, 80.0f);
    EXPECT_EQ(up.frames.back().z, -14.0f);

    TownLadder down = FromTop(CHARA_GORO);
    EXPECT_TRUE(Plays(down, 13));
    EXPECT_EQ(down.frames.back().y, 0.0f);
}

TEST(TownLadder, RubyAndOsmondEndOnTheFarEnd) {
    for (int ally : {CHARA_RUBY, CHARA_OSMOND}) {
        TownLadder up = FromBottom(ally);
        ASSERT_FALSE(up.frames.empty());
        EXPECT_EQ(up.frames.back().y, 80.0f);
        EXPECT_EQ(up.frames.back().z, -14.0f);
        TownLadder down = FromTop(ally);
        ASSERT_FALSE(down.frames.empty());
        EXPECT_EQ(down.frames.back().y, 0.0f);
        EXPECT_EQ(down.frames.back().z, 0.0f);
    }
    EXPECT_TRUE(Plays(FromBottom(CHARA_OSMOND), 16));
}

TEST(TownLadder, OnlyTheAlliesHaveSequences) {
    EXPECT_TRUE(FromBottom(0).frames.empty());
    ED_EVENT_PARAM door{};
    const float    position[3] = {};
    EXPECT_TRUE(TownLadderSequence(CHARA_OSMOND, position, 0.0f, door).frames.empty());
    EXPECT_EQ(TownLadderSequence(CHARA_XIAO, position, 0.0f, door).frames.front().motion, 7);
}
