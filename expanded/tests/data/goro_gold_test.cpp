#include <gtest/gtest.h>

#include "expanded/data/goro_gold.hpp"
#include "expanded/data/sheet_look.hpp"
#include "expanded/weapons/goro/colossus_look.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"

namespace {

using expanded::GoroRegion;

constexpr std::array<int, 3> kFur = {41, 33, 18}, kBelt = {115, 71, 31}, kShirt = {31, 115, 36}, kFlesh = {224, 153, 110},
                             kRed = {153, 20, 20}, kBone = {219, 212, 199};

double Hue(const std::array<int, 3> &c) {
    return expanded::RgbToHsl(c[0], c[1], c[2])[0];
}

} // namespace

TEST(GoroGold, RegionsByColourAndTheWrapsKept) {
    EXPECT_EQ(expanded::GoroRegionOf("c06a03", 10, 10, kFur), GoroRegion::Gold);
    EXPECT_EQ(expanded::GoroRegionOf("c06a03", 10, 10, kBelt), GoroRegion::Gold);
    EXPECT_EQ(expanded::GoroRegionOf("c06a03", 10, 10, kShirt), GoroRegion::Turquoise);
    EXPECT_EQ(expanded::GoroRegionOf("c06a03", 10, 10, kFlesh), GoroRegion::Kept);
    EXPECT_EQ(expanded::GoroRegionOf("c06a03", 10, 10, kRed), GoroRegion::Kept);
    EXPECT_EQ(expanded::GoroRegionOf("c06a03", 10, 10, kBone), GoroRegion::Kept);
    // The tan wraps' block keeps even its browns; the same brown beside it does not.
    EXPECT_EQ(expanded::GoroRegionOf("c06a02", 100, 150, kBelt), GoroRegion::Kept);
    EXPECT_EQ(expanded::GoroRegionOf("c06a02", 90, 150, kBelt), GoroRegion::Gold);
}

TEST(GoroGold, TheCopyTakesTheHammersHues) {
    EXPECT_NEAR(Hue(expanded::GoroGoldColour(GoroRegion::Gold, kBelt)), expanded::kGoroGoldHue, 1.5);
    EXPECT_NEAR(Hue(expanded::GoroGoldColour(GoroRegion::Turquoise, kShirt)), expanded::kGoroTurquoiseHue, 1.5);
    EXPECT_EQ(expanded::GoroGoldColour(GoroRegion::Kept, kFlesh), kFlesh);
    // Its lightness its own.
    auto lit = expanded::GoroGoldColour(GoroRegion::Gold, kBelt);
    EXPECT_NEAR(expanded::RgbToHsl(lit[0], lit[1], lit[2])[2], expanded::RgbToHsl(kBelt[0], kBelt[1], kBelt[2])[2], 1.0);
}

TEST(GoroGold, AllOfTheLookAtTwiceHisSizeNoneAtHisOwn) {
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(1.5f), 0.5f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(expanded::kColossusMost), 1.0f);
}
