#include <gtest/gtest.h>

#include "expanded/data/queens_canal.hpp"
#include "expanded/town/sea.hpp"

namespace expanded::test {

namespace {

TEST(TownSea, EastHarborsSeaIsDrawnWithTheWater) {
    EXPECT_EQ(TownPartPass(kEastHarborMapNo, "gedit/s09/mds/s0903", 1), 0x15);
}

TEST(TownSea, EveryOtherPartKeepsItsPass) {
    EXPECT_EQ(TownPartPass(kEastHarborMapNo, "gedit/s09/mds/s0901", 1), 1);
    EXPECT_EQ(TownPartPass(kEastHarborMapNo, "gedit/s09/mds/s0902", 1), 1);
    EXPECT_EQ(TownPartPass(0, "gedit/s09/mds/s0903", 1), 1);
}

} // namespace

} // namespace expanded::test
