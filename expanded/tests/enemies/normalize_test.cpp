#include <gtest/gtest.h>

#include <memory>

#include "dun/gameloop.hpp"
#include "expanded/enemies/normalize.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace {

using expanded::EnemyStat;
using expanded::NormalizedStat;

constexpr int kMasterJacket = 0;
constexpr int kSkeletonSoldier = 1;
constexpr int kWerewolf = 4;
constexpr int kMimicDbc = 30;
constexpr int kCaveBat = 52;
constexpr int kEvilBat = 53;
constexpr int kDran = 78;

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
    EXPECT_EQ(expanded::NormalizedStats(kWerewolf, 1, false), expanded::NativeStats(kWerewolf));
    EXPECT_EQ(NormalizedStat(kWerewolf, EnemyStat::Hp, 1, false), 180);
}

TEST(EnemyNormalize, ALaterMonsterIsCappedAtTheHighestSoFar) {
    EXPECT_EQ(NormalizedStat(kWerewolf, EnemyStat::Hp, 0, false), 90);
    EXPECT_EQ(NormalizedStat(kWerewolf, EnemyStat::Abs, 0, false), 5);
    expanded::EnemyStats stats = expanded::NormalizedStats(kWerewolf, 0, false);
    EXPECT_EQ(stats.hp, 90);
    EXPECT_EQ(stats.abs, 5);
}

TEST(EnemyNormalize, AnEarlierMonsterIsLiftedToTheAverageAndKeepsItsOrder) {
    EXPECT_EQ(NormalizedStat(kSkeletonSoldier, EnemyStat::Hp, 4, false), 483);
    EXPECT_EQ(NormalizedStat(kMasterJacket, EnemyStat::Hp, 4, false), 483);
    EXPECT_EQ(NormalizedStat(kSkeletonSoldier, EnemyStat::Hp, 7, false), 2500);
    // A stat the monster lacks is lifted too; a shot it lacks stays missing.
    EXPECT_EQ(NormalizedStat(kSkeletonSoldier, EnemyStat::Defense, 5, false), 5);
    EXPECT_EQ(NormalizedStat(kSkeletonSoldier, EnemyStat::Hardness, 5, false), 10);
    EXPECT_EQ(NormalizedStat(kSkeletonSoldier, EnemyStat::Shot, 5, false), 0);
    // Cave Bat is lifted no higher than Evil Bat, a Moon Sea native.
    EXPECT_EQ(NormalizedStat(kCaveBat, EnemyStat::Hp, 4, false), 150);
    EXPECT_EQ(NormalizedStat(kEvilBat, EnemyStat::Hp, 4, false), 150);
    EXPECT_EQ(NormalizedStat(kCaveBat, EnemyStat::Abs, 4, false), 4);
}

TEST(EnemyNormalize, MimicsAndBossesKeepTheirStats) {
    EXPECT_EQ(expanded::NormalizedStats(kMimicDbc, 5, false), expanded::NativeStats(kMimicDbc));
    EXPECT_EQ(NormalizedStat(kMimicDbc, EnemyStat::Hp, 5, false), 68);
    EXPECT_EQ(expanded::NormalizedStats(kDran, 0, false), expanded::NativeStats(kDran));
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

    unit->model[1] = MonstorTable[kWerewolf];
    expanded::EnemyModelSetUp(*unit, 1, kWerewolf);
    unit->monster[0].state = 1;
    unit->monster[0].base_model = 1;
    unit->monster[0].max_hp = unit->monster[0].hp = 180;
    expanded::EnemySpawned(*unit, 0);
    EXPECT_EQ(unit->monster[0].max_hp, 90);
    EXPECT_EQ(unit->monster[0].exp, 5);
    EXPECT_EQ(unit->monster[0].defense, 0);

    selectMapNo = 1;
    expanded::EnemyModelSetUp(*unit, 1, kWerewolf);
    expanded::EnemySpawned(*unit, 0);
    EXPECT_EQ(unit->monster[0].max_hp, 180);

    UserStatus = saved_status;
    selectMapNo = saved_map;
}
