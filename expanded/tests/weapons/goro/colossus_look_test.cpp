#include <gtest/gtest.h>

#include <algorithm>

#include "expanded/weapons/goro/colossus_look.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"

namespace {

using expanded::ColossusRegion;

constexpr float kFur[3] = {0.16f, 0.13f, 0.07f};
constexpr float kBelt[3] = {0.45f, 0.28f, 0.12f};
constexpr float kShirt[3] = {0.12f, 0.45f, 0.14f};
constexpr float kFlesh[3] = {0.88f, 0.6f, 0.43f};
constexpr float kRed[3] = {0.6f, 0.08f, 0.08f};
constexpr float kBone[3] = {0.86f, 0.83f, 0.78f};

float Hue(const float *c) {
    float mx = std::max({c[0], c[1], c[2]}), mn = std::min({c[0], c[1], c[2]}), d = mx - mn;
    float h = mx == c[0] ? (c[1] - c[2]) / d + (c[1] < c[2] ? 6.0f : 0.0f) : mx == c[1] ? (c[2] - c[0]) / d + 2.0f
                                                                                        : (c[0] - c[1]) / d + 4.0f;
    return h * 60.0f;
}

} // namespace

TEST(ColossusLook, RegionsByColourAndTheWrapsKept) {
    EXPECT_EQ(expanded::ColossusRegionOf("c06a03", 10, 10, kFur), ColossusRegion::Gold);
    EXPECT_EQ(expanded::ColossusRegionOf("c06a03", 10, 10, kBelt), ColossusRegion::Gold);
    EXPECT_EQ(expanded::ColossusRegionOf("c06a03", 10, 10, kShirt), ColossusRegion::Turquoise);
    EXPECT_EQ(expanded::ColossusRegionOf("c06a03", 10, 10, kFlesh), ColossusRegion::Kept);
    EXPECT_EQ(expanded::ColossusRegionOf("c06a03", 10, 10, kRed), ColossusRegion::Kept);
    EXPECT_EQ(expanded::ColossusRegionOf("c06a03", 10, 10, kBone), ColossusRegion::Kept);
    // The tan wraps' block keeps even its browns; the same brown beside it does not.
    EXPECT_EQ(expanded::ColossusRegionOf("c06a02", 100, 150, kBelt), ColossusRegion::Kept);
    EXPECT_EQ(expanded::ColossusRegionOf("c06a02", 90, 150, kBelt), ColossusRegion::Gold);
}

TEST(ColossusLook, AllOfItAtTwiceHisSizeNoneAtHisOwn) {
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(1.5f), 0.5f);
    EXPECT_FLOAT_EQ(expanded::ColossusLookShare(expanded::kColossusMost), 1.0f);
    float out[3];
    expanded::ColossusLookColour(ColossusRegion::Gold, 0.0f, kBelt, out);
    EXPECT_FLOAT_EQ(out[0], kBelt[0]);
    expanded::ColossusLookColour(ColossusRegion::Kept, 1.0f, kFlesh, out);
    EXPECT_FLOAT_EQ(out[1], kFlesh[1]);
}

TEST(ColossusLook, TheShirtTurnsTurquoiseAndTheGoldIsLit) {
    float out[3];
    expanded::ColossusLookColour(ColossusRegion::Turquoise, 1.0f, kShirt, out);
    EXPECT_NEAR(Hue(out), expanded::kColossusTurquoiseHue, 0.5f);
    expanded::ColossusLookColour(ColossusRegion::Turquoise, 0.5f, kShirt, out);
    EXPECT_NEAR(Hue(out), (Hue(kShirt) + expanded::kColossusTurquoiseHue) / 2.0f, 0.5f); // halfway round
    // The gold at the hammer's hue, lit by the light gold tint (brighter, warmer in red than in blue).
    float lit[3];
    expanded::ColossusLookColour(ColossusRegion::Gold, 1.0f, kBelt, lit);
    EXPECT_NEAR(Hue(lit), expanded::kColossusGoldHue, 6.0f);
    float brighter = 1.0f + expanded::kColossusTint[0] * expanded::kColossusTintStrength;
    EXPECT_GT(lit[0] + lit[1] + lit[2], (kBelt[0] + kBelt[1] + kBelt[2]) * 1.2f);
    EXPECT_LE(lit[0], kBelt[0] * brighter * 1.5f);
}
