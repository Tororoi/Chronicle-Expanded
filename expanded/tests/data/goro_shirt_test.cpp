#include <gtest/gtest.h>

#include "expanded/data/goro_shirt.hpp"
#include "expanded/data/sheet_look.hpp"
#include "expanded/data/tall_hammer_silver.hpp"
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

TEST(GoroShirt, AsHeShrinksTheHammersGoldTurnsSilver) {
    EXPECT_FLOAT_EQ(expanded::ColossusLookShrink(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShrink(2.0f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShrink(0.75f), 0.5f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShrink(expanded::kColossusLeast), 1.0f);
    constexpr std::array<int, 3> kGold = {176, 135, 38}, kTurquoise = {18, 82, 100};
    auto                         silver = expanded::TallHammerSilverColour(kGold);
    auto                         hsl = expanded::RgbToHsl(silver[0], silver[1], silver[2]), was = expanded::RgbToHsl(kGold[0], kGold[1], kGold[2]);
    EXPECT_LT(hsl[1], 10.0); // greyed
    EXPECT_NEAR(hsl[2], was[2], 1.0);
    auto grey = expanded::TallHammerSilverColour(kTurquoise);
    auto g = expanded::RgbToHsl(grey[0], grey[1], grey[2]), t = expanded::RgbToHsl(kTurquoise[0], kTurquoise[1], kTurquoise[2]);
    EXPECT_NEAR(g[0], t[0], 3.0);                                       // its turquoise still turquoise
    EXPECT_NEAR(g[1], t[1] * expanded::kTallHammerDullSaturation, 3.0); // dulled
    EXPECT_NEAR(g[2], t[2], 1.0);                                       // its lightness its own
}
