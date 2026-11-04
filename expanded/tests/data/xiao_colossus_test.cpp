#include <gtest/gtest.h>

#include "expanded/data/goro_shirt.hpp"
#include "expanded/data/sheet_look.hpp"
#include "expanded/data/tall_hammer_silver.hpp"
#include "expanded/data/xiao_colossus.hpp"

namespace {

constexpr std::array<int, 3> kRed = {170, 40, 40}, kGold = {176, 135, 38}, kFur = {41, 33, 18}, kBlond = {200, 180, 60}, kWhite = {240, 240, 240};

} // namespace

TEST(XiaoColossus, HerCollarAndRingsTurnTurquoiseAsSheGrows) {
    for (const auto &tile : expanded::kXiaoTurquoise) {
        auto turned = expanded::XiaoGrowColour(tile[0], tile[1], kRed);
        auto hsl = expanded::RgbToHsl(turned[0], turned[1], turned[2]), was = expanded::RgbToHsl(kRed[0], kRed[1], kRed[2]);
        EXPECT_NEAR(hsl[0], expanded::kGoroTurquoiseHue, 1.5);
        EXPECT_NEAR(hsl[1], was[1], 1.5); // its saturation and lightness its own
        EXPECT_NEAR(hsl[2], was[2], 1.0);
        EXPECT_EQ(expanded::XiaoGrowColour(tile[0], tile[1], kFur), kFur); // only their reds
    }
    EXPECT_EQ(expanded::XiaoGrowColour(100, 0, kRed), kRed); // a red elsewhere on her sheet
}

TEST(XiaoColossus, HerBellTurnsSilverAsSheShrinks) {
    for (const auto &tile : expanded::kXiaoBell) {
        auto silver = expanded::XiaoShrinkColour(tile[0], tile[1], kGold);
        auto hsl = expanded::RgbToHsl(silver[0], silver[1], silver[2]), was = expanded::RgbToHsl(kGold[0], kGold[1], kGold[2]);
        EXPECT_LT(hsl[1], expanded::kTallHammerSilverSaturation + 2.0); // greyed
        EXPECT_NEAR(hsl[2], was[2], 1.0);
        EXPECT_EQ(expanded::XiaoShrinkColour(tile[0], tile[1], kRed), kRed); // only its golds
    }
    EXPECT_EQ(expanded::XiaoShrinkColour(200, 0, kGold), kGold); // a gold elsewhere on her sheet
}

TEST(XiaoColossus, SuperStevesHairIsOnlyTheBlondInItsTile) {
    const int(&tile)[4] = expanded::kSuperSteveHairTile;
    EXPECT_TRUE(expanded::SuperSteveHair(tile[0], tile[1], kBlond));
    EXPECT_FALSE(expanded::SuperSteveHair(tile[0], tile[1], kWhite));
    EXPECT_FALSE(expanded::SuperSteveHair(tile[0], tile[1], kRed));
    EXPECT_FALSE(expanded::SuperSteveHair(tile[2], tile[3], kBlond)); // past its tile
}
