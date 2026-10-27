#include <gtest/gtest.h>

#include <memory>

#include "expanded/weapons/pellets.hpp"

namespace {

using expanded::kPelletSlots;
using expanded::PelletSlots;

TEST(PelletSlots, KeepsOnePelletsValuePerSlot) {
    auto             shots = std::make_unique<CSHOT>();
    auto             other = std::make_unique<CSHOT>();
    PelletSlots<int> kept;

    kept.Keep(*shots, 3, 7);

    ASSERT_NE(kept.Of(*shots, 3), nullptr);
    EXPECT_EQ(*kept.Of(*shots, 3), 7);
    EXPECT_EQ(kept.Of(*other, 3), nullptr);
    EXPECT_EQ(kept.Of(*shots, 4), nullptr);
    EXPECT_EQ(*kept.InSlot(3), 7);
    EXPECT_TRUE(kept.Any([](int value) { return value == 7; }));

    kept.Keep(*other, 3, 9); // a pellet fired into the slot takes it over
    EXPECT_EQ(kept.Of(*shots, 3), nullptr);
    EXPECT_EQ(kept.Take(*other, 3), 9);
    EXPECT_EQ(kept.InSlot(3), nullptr);
    EXPECT_FALSE(kept.Take(*other, 3).has_value());
}

TEST(PelletSlots, ASlotOutOfRangeKeepsNothing) {
    auto             shots = std::make_unique<CSHOT>();
    PelletSlots<int> kept;

    kept.Keep(*shots, -1, 1);
    kept.Keep(*shots, kPelletSlots, 1);
    kept.Drop(kPelletSlots);

    EXPECT_EQ(kept.Of(*shots, -1), nullptr);
    EXPECT_EQ(kept.Of(*shots, kPelletSlots), nullptr);
    EXPECT_FALSE(kept.Any([](int) { return true; }));
}

TEST(Pellets, ACopyFliesAsTheOriginalFresh) {
    auto shots = std::make_unique<CSHOT>();
    *shots = {};
    shots->used[0] = 1;
    shots->used[1] = 1;
    shots->pos[0][0] = 5.0f;
    shots->vector[0][2] = 2.0f;
    shots->damage[0] = 30;
    shots->size[0] = 1.5f;
    shots->start_flag[2] = 1;

    int free = expanded::FreePelletSlot(*shots, 0);
    ASSERT_EQ(free, 2);
    expanded::CopyPellet(*shots, 0, free);

    EXPECT_EQ(shots->pos[2][0], 5.0f);
    EXPECT_EQ(shots->vector[2][2], 2.0f);
    EXPECT_EQ(shots->damage[2], 30);
    EXPECT_EQ(shots->size[2], 1.5f);
    EXPECT_EQ(shots->start_flag[2], 0);
    EXPECT_EQ(shots->used[2], 1);
}

} // namespace
