#include <gtest/gtest.h>

#include "expanded/data/goro_shirt.hpp"
#include "expanded/data/sheet_look.hpp"
#include "expanded/weapons/goro/colossus_look.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"

namespace {

constexpr std::array<int, 3> kFur = {41, 33, 18}, kBelt = {115, 71, 31}, kShirt = {31, 115, 36}, kFlesh = {224, 153, 110};

} // namespace

TEST(GoroShirt, OnlyTheShirtTurnsTurquoise) {
    auto shirt = expanded::GoroShirtColour(kShirt);
    auto hsl = expanded::RgbToHsl(shirt[0], shirt[1], shirt[2]), was = expanded::RgbToHsl(kShirt[0], kShirt[1], kShirt[2]);
    EXPECT_NEAR(hsl[0], expanded::kGoroTurquoiseHue, 1.5);
    EXPECT_NEAR(hsl[2], was[2], 1.0); // its lightness its own
    for (const auto &kept : {kFur, kBelt, kFlesh}) {
        EXPECT_FALSE(expanded::GoroShirtTexel(kept));
        EXPECT_EQ(expanded::GoroShirtColour(kept), kept);
    }
}

TEST(GoroShirt, AllOfTheLookAtTwiceHisSizeNoneAtHisOwn) {
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(1.5f), 0.5f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(expanded::kColossusMost), 1.0f);
}
