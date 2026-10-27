#include <gtest/gtest.h>

#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/normalize.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using namespace expanded;

} // namespace

TEST(EnemyNormalize, RegionsAreTheDungeonsAndDemonShaftsBands) {
    EXPECT_EQ(expanded::EnemyRegion(0, 3), 0);
    EXPECT_EQ(expanded::EnemyRegion(5, 24), 5);
    EXPECT_EQ(expanded::EnemyRegion(6, 0), 6);
    EXPECT_EQ(expanded::EnemyRegion(6, 19), 6);
    EXPECT_EQ(expanded::EnemyRegion(6, 20), 7);
    EXPECT_EQ(expanded::EnemyRegion(6, 99), 10);
    EXPECT_EQ(expanded::EnemyRegion(7, 0), -1);
    EXPECT_EQ(expanded::DemonShaftBand(39), 1);
    EXPECT_EQ(expanded::DemonShaftBand(-5), 0);
    EXPECT_EQ(expanded::DemonShaftBand(120), expanded::kDemonShaftBands - 1);
}

TEST(EnemyNormalize, ARegionsOwnMonstersKeepTheirStats) {
    EXPECT_EQ(expanded::NormalizedStats(Monster::WEREWOLF, 1, false), expanded::NativeStats(No(Monster::WEREWOLF)));
    EXPECT_EQ(NormalizedStat(Monster::WEREWOLF, EnemyStat::Hp, 1, false), 180);
}

TEST(EnemyNormalize, ALaterMonsterIsCappedAtTheHighestSoFar) {
    EXPECT_EQ(NormalizedStat(Monster::WEREWOLF, EnemyStat::Hp, 0, false), 90);
    EXPECT_EQ(NormalizedStat(Monster::WEREWOLF, EnemyStat::Abs, 0, false), 5);
    expanded::EnemyStats stats = expanded::NormalizedStats(Monster::WEREWOLF, 0, false);
    EXPECT_EQ(stats.hp, 90);
    EXPECT_EQ(stats.abs, 5);
}

TEST(EnemyNormalize, AnEarlierMonsterIsLiftedToTheAverageAndKeepsItsOrder) {
    EXPECT_EQ(NormalizedStat(Monster::SKELETON_SOLDIER, EnemyStat::Hp, 4, false), 483);
    EXPECT_EQ(NormalizedStat(Monster::MASTER_JACKET, EnemyStat::Hp, 4, false), 483);
    EXPECT_EQ(NormalizedStat(Monster::SKELETON_SOLDIER, EnemyStat::Hp, 7, false), 2500);
    // A stat the monster lacks is lifted too; a shot it lacks stays missing.
    EXPECT_EQ(NormalizedStat(Monster::SKELETON_SOLDIER, EnemyStat::Defense, 5, false), 5);
    EXPECT_EQ(NormalizedStat(Monster::SKELETON_SOLDIER, EnemyStat::Hardness, 5, false), 10);
    EXPECT_EQ(NormalizedStat(Monster::SKELETON_SOLDIER, EnemyStat::Shot, 5, false), 0);
    // Cave Bat is lifted no higher than Evil Bat, a Moon Sea native.
    EXPECT_EQ(NormalizedStat(Monster::CAVE_BAT, EnemyStat::Hp, 4, false), 150);
    EXPECT_EQ(NormalizedStat(Monster::EVIL_BAT, EnemyStat::Hp, 4, false), 150);
    EXPECT_EQ(NormalizedStat(Monster::CAVE_BAT, EnemyStat::Abs, 4, false), 4);
}

TEST(EnemyNormalize, MimicsAndBossesKeepTheirStats) {
    EXPECT_EQ(expanded::NormalizedStats(Monster::MIMIC_DBC, 5, false), expanded::NativeStats(No(Monster::MIMIC_DBC)));
    EXPECT_EQ(NormalizedStat(Monster::MIMIC_DBC, EnemyStat::Hp, 5, false), 68);
    EXPECT_EQ(expanded::NormalizedStats(Monster::DRAN, 0, false), expanded::NativeStats(No(Monster::DRAN)));
}

TEST(EnemyNormalize, AKindSetUpOnAFloorIsNormalizedForItsRegion) {
    expanded::test::TestFloor floor(0, 2);
    MONSTOR                  &live = floor.unit().monster[0];
    floor.SetUpModel(1, Monster::WEREWOLF);
    floor.Spawn(0, 1);
    EXPECT_EQ(live.max_hp, 90);
    EXPECT_EQ(live.exp, 5);
    EXPECT_EQ(live.defense, 0);

    selectMapNo = 1;
    floor.SetUpModel(1, Monster::WEREWOLF);
    floor.Spawn(0, 1);
    EXPECT_EQ(live.max_hp, 180);
}
