#include <gtest/gtest.h>

#include <vector>

#include "expanded/core/random.hpp"

namespace {

std::vector<int> Rolls() {
    std::vector<int> rolls;
    for (int i = 0; i < 16; i++) {
        rolls.push_back(expanded::ModRoll(100));
    }
    return rolls;
}

} // namespace

TEST(ModRandom, ASeedRepeatsItsRolls) {
    expanded::SeedModRandom(7);
    std::vector<int> first = Rolls();
    expanded::SeedModRandom(7);
    EXPECT_EQ(Rolls(), first);
    for (int roll : first) {
        EXPECT_GE(roll, 0);
        EXPECT_LT(roll, 100);
    }
}

TEST(ModRandom, ChanceZeroNeverAndOneAlways) {
    expanded::SeedModRandom(1);
    for (int i = 0; i < 100; i++) {
        EXPECT_FALSE(expanded::ModChance(0.0));
        EXPECT_TRUE(expanded::ModChance(1.0));
    }
}

TEST(GameFraction, IsAFraction) {
    for (int i = 0; i < 1000; i++) {
        float f = expanded::GameFraction();
        EXPECT_GE(f, 0.0f);
        EXPECT_LE(f, 1.0f);
    }
}
