#include <gtest/gtest.h>

#include "expanded/status/poison.hpp"
#include "monstorunit.hpp"

TEST(PoisonTick, ScalesWithStatusChanceAndTakesAtLeastOne) {
    MONSTOR monster = {};

    monster.max_hp = 600;
    monster.status_chance = 70;
    EXPECT_EQ(expanded::PoisonTickDamage(&monster), 42);

    monster.max_hp = 10;
    EXPECT_EQ(expanded::PoisonTickDamage(&monster), 1);
}

TEST(PoisonTick, RepoisoningKeepsTheRunningPoison) {
    MONSTOR monster = {};

    expanded::Poison(&monster, 0);
    EXPECT_EQ(monster.poison_timer, 180);

    monster.poison_timer = 37;
    monster.slow_timer = 5;
    expanded::Poison(&monster, 0);
    EXPECT_EQ(monster.poison_timer, 37);
    EXPECT_EQ(monster.slow_timer, 0);
}
