#include <gtest/gtest.h>

#include "expanded/core/frame_tint.hpp"
#include "test_support.hpp"

namespace {

const CFrame *Frame(int n) {
    static const char frames[3] = {};
    return reinterpret_cast<const CFrame *>(&frames[n]);
}

} // namespace

TEST(FrameTint, AddsToTheAmbientForItsFrameAlone) {
    expanded::test::TestFloor floor;
    floor.SetUpModel(0, expanded::Monster::WEREWOLF); // a new floor: no tints
    float ambient[4] = {10, 20, 30, 128}, saved[4];
    EXPECT_FALSE(expanded::FrameTintBegin(Frame(0), ambient, saved));

    expanded::SetFrameTint(Frame(0), 5, 6, 7);
    EXPECT_FALSE(expanded::FrameTintBegin(Frame(1), ambient, saved));
    ASSERT_TRUE(expanded::FrameTintBegin(Frame(0), ambient, saved));
    EXPECT_EQ(ambient[0], 15);
    EXPECT_EQ(ambient[1], 26);
    EXPECT_EQ(ambient[2], 37);
    EXPECT_EQ(ambient[3], 128);
    expanded::FrameTintEnd(ambient, saved);
    EXPECT_EQ(ambient[0], 10);
    EXPECT_EQ(ambient[2], 30);

    // A new tint replaces the frame's; none stops it.
    expanded::SetFrameTint(Frame(0), 1, 1, 1);
    ASSERT_TRUE(expanded::FrameTintBegin(Frame(0), ambient, saved));
    EXPECT_EQ(ambient[0], 11);
    expanded::FrameTintEnd(ambient, saved);
    expanded::SetFrameTint(Frame(0), 0, 0, 0);
    EXPECT_FALSE(expanded::FrameTintBegin(Frame(0), ambient, saved));
}

TEST(FrameTint, ANewFloorForgetsEveryTint) {
    expanded::test::TestFloor floor;
    expanded::SetFrameTint(Frame(1), 5, 5, 5);
    expanded::SetFrameTint(Frame(2), 5, 5, 5);
    floor.SetUpModel(3, expanded::Monster::WEREWOLF); // a kind set up mid-floor keeps them
    float ambient[4] = {}, saved[4];
    EXPECT_TRUE(expanded::FrameTintBegin(Frame(1), ambient, saved));
    expanded::FrameTintEnd(ambient, saved);

    floor.SetUpModel(0, expanded::Monster::WEREWOLF);
    EXPECT_FALSE(expanded::FrameTintBegin(Frame(1), ambient, saved));
    EXPECT_FALSE(expanded::FrameTintBegin(Frame(2), ambient, saved));
}
