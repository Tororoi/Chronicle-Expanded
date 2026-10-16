#include <gtest/gtest.h>

#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/normalize.hpp"
#include "monstorunit.hpp"
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
}

TEST(EnemyNormalize, ARegionsOwnMonstersKeepTheirStats) {
    EXPECT_EQ(expanded::NormalizedStats(MONSTER_WEREWOLF, 1, false), expanded::NativeStats(MONSTER_WEREWOLF));
    EXPECT_EQ(NormalizedStat(MONSTER_WEREWOLF, EnemyStat::Hp, 1, false), 180);
}

TEST(EnemyNormalize, ALaterMonsterIsCappedAtTheHighestSoFar) {
    EXPECT_EQ(NormalizedStat(MONSTER_WEREWOLF, EnemyStat::Hp, 0, false), 90);
    EXPECT_EQ(NormalizedStat(MONSTER_WEREWOLF, EnemyStat::Abs, 0, false), 5);
    expanded::EnemyStats stats = expanded::NormalizedStats(MONSTER_WEREWOLF, 0, false);
    EXPECT_EQ(stats.hp, 90);
    EXPECT_EQ(stats.abs, 5);
}

TEST(EnemyNormalize, AnEarlierMonsterIsLiftedToTheAverageAndKeepsItsOrder) {
    EXPECT_EQ(NormalizedStat(MONSTER_SKELETON_SOLDIER, EnemyStat::Hp, 4, false), 483);
    EXPECT_EQ(NormalizedStat(MONSTER_MASTER_JACKET, EnemyStat::Hp, 4, false), 483);
    EXPECT_EQ(NormalizedStat(MONSTER_SKELETON_SOLDIER, EnemyStat::Hp, 7, false), 2500);
    // A stat the monster lacks is lifted too; a shot it lacks stays missing.
    EXPECT_EQ(NormalizedStat(MONSTER_SKELETON_SOLDIER, EnemyStat::Defense, 5, false), 5);
    EXPECT_EQ(NormalizedStat(MONSTER_SKELETON_SOLDIER, EnemyStat::Hardness, 5, false), 10);
    EXPECT_EQ(NormalizedStat(MONSTER_SKELETON_SOLDIER, EnemyStat::Shot, 5, false), 0);
    // Cave Bat is lifted no higher than Evil Bat, a Moon Sea native.
    EXPECT_EQ(NormalizedStat(MONSTER_CAVE_BAT, EnemyStat::Hp, 4, false), 150);
    EXPECT_EQ(NormalizedStat(MONSTER_EVIL_BAT, EnemyStat::Hp, 4, false), 150);
    EXPECT_EQ(NormalizedStat(MONSTER_CAVE_BAT, EnemyStat::Abs, 4, false), 4);
}

TEST(EnemyNormalize, MimicsAndBossesKeepTheirStats) {
    EXPECT_EQ(expanded::NormalizedStats(MONSTER_MIMIC_DBC, 5, false), expanded::NativeStats(MONSTER_MIMIC_DBC));
    EXPECT_EQ(NormalizedStat(MONSTER_MIMIC_DBC, EnemyStat::Hp, 5, false), 68);
    EXPECT_EQ(expanded::NormalizedStats(MONSTER_DRAN, 0, false), expanded::NativeStats(MONSTER_DRAN));
}

TEST(EnemyNormalize, AKindSetUpOnAFloorIsNormalizedForItsRegion) {
    auto           unit = std::make_unique<CMonstorUnit>();
    auto           status = std::make_unique<CUserStatus>();
    CUserStatus   *saved_status = UserStatus;
    s32            saved_map = selectMapNo;
    UserStatus = status.get();
    selectMapNo = 0;
    UserStatus->cur_floor = 2;
    for (MONSTOR &monster : unit->monster) {
        monster.state = -1;
    }

    unit->model[1] = MonstorTable[MONSTER_WEREWOLF];
    expanded::EnemyModelSetUp(*unit, 1, MONSTER_WEREWOLF);
    unit->monster[0].state = 1;
    unit->monster[0].base_model = 1;
    unit->monster[0].max_hp = unit->monster[0].hp = 180;
    expanded::EnemySpawned(*unit, 0);
    EXPECT_EQ(unit->monster[0].max_hp, 90);
    EXPECT_EQ(unit->monster[0].exp, 5);
    EXPECT_EQ(unit->monster[0].defense, 0);

    selectMapNo = 1;
    expanded::EnemyModelSetUp(*unit, 1, MONSTER_WEREWOLF);
    expanded::EnemySpawned(*unit, 0);
    EXPECT_EQ(unit->monster[0].max_hp, 180);

    UserStatus = saved_status;
    selectMapNo = saved_map;
}
