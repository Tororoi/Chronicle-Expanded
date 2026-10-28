#include <gtest/gtest.h>

#include <limits>

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

TEST(MonsterQueries, ABodysUndersideAndHowNearAPointIs) {
    expanded::test::TestFloor floor;
    CMonstorUnit             &unit = floor.unit();
    unit.monster[1] = Monster(2, 10, -1);
    float root[4] = {100.0f, 2.0f, 0.0f, 1.0f};
    unit.chara[1][0].SetPosition(root);
    // No sphere posed: its root's height, and nothing near.
    EXPECT_FLOAT_EQ(expanded::MonsterBodyUnderside(unit, 1), 2.0f);
    float point[3] = {100.0f, 10.0f, 0.0f};
    EXPECT_EQ(expanded::MonsterBodyDistance(unit, 1, point), std::numeric_limits<float>::max());
    // Two spheres: radius 3 at height 8, radius 1 at height 5; and one left from another frame's pose, far off.
    MONSTOR_EFFECT_STATE &body = unit.effect[1];
    body.timer[0] = body.timer[1] = body.timer[2] = 1;
    body.position[0][0] = 100.0f, body.position[0][1] = 8.0f, body.radius[0] = 3.0f;
    body.position[1][0] = 100.0f, body.position[1][1] = 5.0f, body.position[1][2] = 4.0f, body.radius[1] = 1.0f;
    body.position[2][0] = 0.0f, body.position[2][1] = -50.0f, body.radius[2] = 1.0f;
    EXPECT_FLOAT_EQ(expanded::MonsterBodyUnderside(unit, 1), 4.0f);
    EXPECT_FLOAT_EQ(expanded::MonsterBodyDistance(unit, 1, point), -1.0f); // inside the first, 2 from its centre
    float beside[3] = {100.0f, 5.0f, 10.0f};
    EXPECT_FLOAT_EQ(expanded::MonsterBodyDistance(unit, 1, beside), 5.0f);
}

TEST(MonsterQueries, WhereAnAttackerGoesForABody) {
    expanded::test::TestFloor floor;
    CMonstorUnit             &unit = floor.unit();
    unit.monster[2] = Monster(2, 10, -1);
    float root[4] = {100.0f, 3.0f, 0.0f, 1.0f};
    unit.chara[2][0].SetPosition(root);
    float point[4];
    // No sphere posed: its root.
    expanded::MonsterAimPoint(unit, 2, 0.0f, 0.0f, point);
    EXPECT_FLOAT_EQ(point[0], 100.0f);
    EXPECT_FLOAT_EQ(point[1], 3.0f);
    EXPECT_FLOAT_EQ(point[3], 1.0f);
    // A small sphere (radius 5, no deeper than kAimDepth): its centre, at the root's height.
    MONSTOR_EFFECT_STATE &body = unit.effect[2];
    body.timer[0] = 1;
    body.position[0][0] = 104.0f, body.position[0][1] = 9.0f, body.position[0][2] = 2.0f, body.radius[0] = 5.0f;
    expanded::MonsterAimPoint(unit, 2, 0.0f, 2.0f, point);
    EXPECT_FLOAT_EQ(point[0], 104.0f);
    EXPECT_FLOAT_EQ(point[1], 3.0f);
    EXPECT_FLOAT_EQ(point[2], 2.0f);
    // A large one (radius 20) whose edge is nearer: kAimDepth inside it toward the attacker, 12 from its centre.
    body.timer[1] = 1;
    body.position[1][0] = 120.0f, body.position[1][2] = 0.0f, body.radius[1] = 20.0f;
    expanded::MonsterAimPoint(unit, 2, 120.0f, -50.0f, point);
    EXPECT_FLOAT_EQ(point[0], 120.0f);
    EXPECT_FLOAT_EQ(point[2], -12.0f);
    // An attacker already that deep goes for the centre.
    expanded::MonsterAimPoint(unit, 2, 120.0f, -5.0f, point);
    EXPECT_FLOAT_EQ(point[2], 0.0f);
}

} // namespace
