#include <gtest/gtest.h>

#include "expanded/enemies/boss_stats.hpp"
#include "expanded/enemies/monsters.hpp"

using expanded::Monster;
using expanded::No;

TEST(BossStats, ABossInAnotherDungeonHasThatDungeonsStats) {
    std::optional<expanded::EnemyStats> utan = expanded::BossRegionStats(No(Monster::MASTER_UTAN), 0);
    ASSERT_TRUE(utan.has_value());
    EXPECT_EQ(*utan, (expanded::EnemyStats{300, 10, 12, 0, 45, 25}));
    std::optional<expanded::EnemyStats> dran = expanded::BossRegionStats(No(Monster::DRAN), 10);
    ASSERT_TRUE(dran.has_value());
    EXPECT_EQ(*dran, (expanded::EnemyStats{50000, 110, 10, 20, 155, 150}));
}

TEST(BossStats, ABossInItsOwnDungeonKeepsItsOwn) {
    EXPECT_FALSE(expanded::BossRegionStats(No(Monster::DRAN), 0).has_value());
    EXPECT_FALSE(expanded::BossRegionStats(No(Monster::MASTER_UTAN), 1).has_value());
    EXPECT_FALSE(expanded::BossRegionStats(No(Monster::ICE_QUEEN), 2).has_value());
    EXPECT_FALSE(expanded::BossRegionStats(No(Monster::KINGS_CURSE), 3).has_value());
    EXPECT_FALSE(expanded::BossRegionStats(No(Monster::MINOTAUR_JOE), 4).has_value());
    // Regular monsters and floors outside the dungeons are not the table's.
    EXPECT_FALSE(expanded::BossRegionStats(No(Monster::WEREWOLF), 3).has_value());
    EXPECT_FALSE(expanded::BossRegionStats(No(Monster::DRAN), -1).has_value());
}

TEST(BossStats, TheBlackKnightsOwnFightGivesMoreAbs) {
    std::optional<expanded::EnemyStats> knight = expanded::BossRegionStats(No(Monster::BLACK_KNIGHT), 10);
    ASSERT_TRUE(knight.has_value());
    EXPECT_EQ(*knight, (expanded::EnemyStats{40000, 110, 8, 100, 170, 150}));
}

TEST(BossStats, TheIceQueensPiecesHitForTheirRegion) {
    EXPECT_EQ(expanded::IceQueenPieceDamage(69, 0), 35);
    EXPECT_EQ(expanded::IceQueenPieceDamage(74, 0), 45);
    EXPECT_EQ(expanded::IceQueenPieceDamage(74, 10), 155);
    // Hers in Shipwreck, and the prison's none anywhere.
    EXPECT_EQ(expanded::IceQueenPieceDamage(69, 2), 69);
    EXPECT_EQ(expanded::IceQueenPieceDamage(0, 4), 0);
}
