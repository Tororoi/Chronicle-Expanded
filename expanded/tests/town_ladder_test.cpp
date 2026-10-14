#include <gtest/gtest.h>

#include <algorithm>

#include "editloop.hpp"
#include "expanded/town_ladder.hpp"

using expanded::TownLadder;
using expanded::TownLadderFrame;
using expanded::TownLadderSequence;

namespace {

constexpr int kXiao = 1;
constexpr int kGoro = 2;
constexpr int kRuby = 3;
constexpr int kUngaga = 4;
constexpr int kOsmond = 5;

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
    for (const TownLadder &ladder : {FromBottom(kUngaga), FromTop(kUngaga)}) {
        ASSERT_EQ(ladder.frames.size(), 191u);
        EXPECT_FALSE(ladder.camera);
        EXPECT_EQ(ladder.frames.front().motion, 7);
        EXPECT_EQ(ladder.frames.front().flags, 2);
        EXPECT_EQ(ladder.frames.back().motion, 0);
        EXPECT_EQ(ladder.frames.back().z, ladder.frames.front().z);
    }
}

TEST(TownLadder, XiaoLeapsUpAndLandsOnHerOwn) {
    TownLadder up = FromBottom(kXiao);
    ASSERT_FALSE(up.frames.empty());
    EXPECT_TRUE(up.camera);
    EXPECT_TRUE(Plays(up, 13));
    EXPECT_TRUE(Plays(up, 11));
    EXPECT_TRUE(Plays(up, 12));
    EXPECT_NEAR(up.frames.back().x, 0.0f, 0.01f);
    EXPECT_NEAR(up.frames.back().z, -14.0f, 0.01f);
    EXPECT_GT(up.frames.back().y, 80.0f);
    EXPECT_LT(up.frames.back().y, 84.0f);

    TownLadder down = FromTop(kXiao);
    ASSERT_FALSE(down.frames.empty());
    EXPECT_FALSE(Plays(down, 13));
    EXPECT_NEAR(down.frames.back().z, 0.0f, 0.01f);
    EXPECT_GT(down.frames.back().y, 0.0f);
}

TEST(TownLadder, GoroHopsUpFourTimes) {
    TownLadder up = FromBottom(kGoro);
    int        hops = static_cast<int>(std::count_if(up.frames.begin(), up.frames.end(), [](const TownLadderFrame &f) { return f.sound == 361; }));
    EXPECT_EQ(hops, 4);
    EXPECT_TRUE(Plays(up, 11));
    EXPECT_TRUE(Plays(up, 12));
    EXPECT_EQ(up.frames.back().y, 80.0f);
    EXPECT_EQ(up.frames.back().z, -14.0f);

    TownLadder down = FromTop(kGoro);
    EXPECT_TRUE(Plays(down, 13));
    EXPECT_EQ(down.frames.back().y, 0.0f);
}

TEST(TownLadder, RubyAndOsmondEndOnTheFarEnd) {
    for (int ally : {kRuby, kOsmond}) {
        TownLadder up = FromBottom(ally);
        ASSERT_FALSE(up.frames.empty());
        EXPECT_EQ(up.frames.back().y, 80.0f);
        EXPECT_EQ(up.frames.back().z, -14.0f);
        TownLadder down = FromTop(ally);
        ASSERT_FALSE(down.frames.empty());
        EXPECT_EQ(down.frames.back().y, 0.0f);
        EXPECT_EQ(down.frames.back().z, 0.0f);
    }
    EXPECT_TRUE(Plays(FromBottom(kOsmond), 16));
}

TEST(TownLadder, OnlyTheAlliesHaveSequences) {
    EXPECT_TRUE(FromBottom(0).frames.empty());
    ED_EVENT_PARAM door{};
    const float    position[3] = {};
    EXPECT_TRUE(TownLadderSequence(kOsmond, position, 0.0f, door).frames.empty());
    EXPECT_EQ(TownLadderSequence(kXiao, position, 0.0f, door).frames.front().motion, 7);
}
