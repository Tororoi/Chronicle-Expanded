#include <gtest/gtest.h>

#include "expanded/enemies/monster_queries.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

namespace {

using expanded::MonsterInPlay;
using expanded::MonsterLiving;
using expanded::MonsterShown;

MONSTOR Monster(int state, int hp, int revealed) {
    MONSTOR monster = {};
    monster.state = state;
    monster.hp = hp;
    monster.revealed = static_cast<s16>(revealed);
    return monster;
}

TEST(MonsterQueries, LivingInPlayAndShownAreTheirOwnTests) {
    EXPECT_TRUE(MonsterLiving(Monster(1, 10, 0)));    // asleep in its chest
    EXPECT_FALSE(MonsterLiving(Monster(2, 0, -1)));   // killed
    EXPECT_FALSE(MonsterLiving(Monster(-1, 10, -1))); // no monster
    EXPECT_TRUE(MonsterInPlay(Monster(2, 10, -1)));
    EXPECT_FALSE(MonsterInPlay(Monster(2, 10, 1))); // opened, its appearance still running
    EXPECT_TRUE(MonsterShown(Monster(2, 0, 1)));    // killed or not
    EXPECT_FALSE(MonsterShown(Monster(2, 10, 0)));
}

TEST(MonsterQueries, TheNearestLivingMonsterWithinReach) {
    expanded::test::TestFloor floor;
    CMonstorUnit             &unit = floor.unit();
    float                     near[4] = {3.0f, 50.0f, 0.0f, 1.0f}, far[4] = {8.0f, 0.0f, 0.0f, 1.0f}, point[4] = {};
    unit.monster[2] = Monster(2, 0, -1); // killed, nearest of all
    unit.chara[2][0].SetPosition(point);
    unit.monster[4] = Monster(2, 10, -1);
    unit.chara[4][0].SetPosition(far);
    unit.monster[6] = Monster(1, 10, -1);
    unit.chara[6][0].SetPosition(near); // across the ground: its height does not count

    EXPECT_EQ(expanded::NearestMonster(unit, point, 10.0f), 6);
    EXPECT_EQ(expanded::NearestMonster(unit, point, 2.0f), -1);
}

TEST(MonsterQueries, APointTouchingAShownMonstersSphere) {
    expanded::test::TestFloor floor;
    CMonstorUnit             &unit = floor.unit();
    unit.monster[0] = Monster(2, 10, 0); // in its chest: not tested
    unit.effect[0].timer[0] = 1;
    unit.effect[0].radius[0] = 5.0f;
    unit.monster[3] = Monster(2, 10, -1);
    unit.effect[3].timer[2] = 1;
    unit.effect[3].radius[2] = 2.0f;
    unit.effect[3].position[2][0] = 10.0f;
    float point[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    int sphere = -1;
    EXPECT_EQ(expanded::MonsterTouching(unit, point, 8.0f, &sphere), 3);
    EXPECT_EQ(sphere, 2);
    EXPECT_EQ(expanded::MonsterTouching(unit, point, 7.0f), -1);
}

} // namespace
