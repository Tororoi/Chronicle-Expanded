#include <gtest/gtest.h>

#include "collisiondata.hpp"
#include "expanded/ungaga.hpp"

TEST(UngagaChargeShot, OnlyHisScaledHitsAreChargeShots) {
    COLLISION_HIT record = {};

    record.scale_rate = 1.5f;
    EXPECT_TRUE(expanded::ChargeShotHit(4, &record));
    EXPECT_FALSE(expanded::ChargeShotHit(0, &record));

    record.scale_rate = 0.0f;
    EXPECT_FALSE(expanded::ChargeShotHit(4, &record));
}
