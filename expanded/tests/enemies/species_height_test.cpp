#include <gtest/gtest.h>

#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/species.hpp"

using expanded::Monster;

TEST(SpeciesHeight, IsEachKindsOwnAuthoredHeight) {
    EXPECT_FLOAT_EQ(expanded::SpeciesHeight(expanded::No(Monster::MASTER_JACKET)), 16.0f);
    EXPECT_FLOAT_EQ(expanded::SpeciesHeight(expanded::No(Monster::STATUE)), 20.5f);
    // Past every kind, the fallback.
    EXPECT_FLOAT_EQ(expanded::SpeciesHeight(1000), expanded::kSpeciesHeightFallback);
}
