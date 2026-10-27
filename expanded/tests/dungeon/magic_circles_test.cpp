#include <gtest/gtest.h>

#include <cstdlib>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/dungeon/magic_circles.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

namespace {

// A save whose Toan wields a Baselard (a bag of eight free slots), owning the amplifying weapons asked for, and the
// Secret Armlet in storage when asked for.
struct Circle {
    expanded::test::TestSave  save;
    expanded::test::TestFloor floor;
    WEAPON_HAVE              *weapon;
    WEAPON_HAVE               battle = {}; // the record in play, which SetWeaponAttachStatus rebuilds from the bag's
    WEAPON_HAVE              *saved_weapon = NowWeaponHave;

    Circle(bool crysknife, bool hammer, bool armlet = false) {
        UserStatus = &save.status();
        CUserStatus &status = save.status();
        status.party_size = 3;
        status.cur_chara = 0;
        status.equipped_weapon_slot[0] = 0;
        weapon = &status.chara_weapons[0][0];
        weapon->item_no = ITEM_WEAPON_BASELARD; // not a default weapon, which the rolls leave alone
        weapon->attack = 20;
        weapon->durability = 40;
        weapon->durability_f = 40.0f;
        NowWeaponHave = &battle;
        status.chara_weapons[0][1].item_no = crysknife ? ITEM_WEAPON_CRYSKNIFE : 0;
        status.chara_weapons[2][0].item_no = hammer ? ITEM_WEAPON_MAGICAL_HAMMER : 0;
        status.item_pack.num = 8;
        for (s16 &item : status.item_pack.item) {
            item = -1;
        }
        save.stock().weapons[0].item_no = armlet ? ITEM_WEAPON_SECRET_ARMLET : 0;
    }

    ~Circle() { NowWeaponHave = saved_weapon; }

    bool Run(int kind) { return expanded::ExpandedCircle(kind, *weapon) >= 0; }

    // The circle as the game runs it, stepped on: the message's kind it gives.
    int Step(int kind) {
        MAP_TRAP_CIRCLE trap = {};
        trap.state = 2;
        trap.kind = kind;
        return Run_TrapCircle(&trap);
    }

    int Carried(int item_no) {
        int count = 0;
        for (s16 item : save.status().item_pack.item) {
            count += item == item_no;
        }
        return count;
    }

    CUserStatus &Status() { return save.status(); }
};

} // namespace

TEST(MagicCircles, OwningEachAmplifierAddsOne) {
    EXPECT_EQ(Circle(false, false).Run(TRAP_CIRCLE_MONEY_UP), false); // retail's
    {
        Circle circle(true, false);
        EXPECT_EQ(expanded::CircleMagnitude(), 2);
    }
    {
        Circle circle(false, true);
        EXPECT_EQ(expanded::CircleMagnitude(), 2);
    }
    {
        Circle circle(true, true);
        EXPECT_EQ(expanded::CircleMagnitude(), 3);
    }
}

TEST(MagicCircles, MoneyAndTimersScale) {
    Circle circle(true, true);
    circle.Status().money = 100;
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_MONEY_UP));
    EXPECT_EQ(circle.Status().money, 100 + 3 * (120 + 10));
    circle.Status().money = 100;
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_MONEY_DOWN));
    EXPECT_EQ(circle.Status().money, static_cast<int>(100 - 0.2f * 3 * 100)); // retail's float arithmetic: 39

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_STAMINA));
    EXPECT_EQ(circle.Status().ailment_frames[0], 1800 * 3);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ALL_MONSTER_STAMINA));
    EXPECT_EQ(circle.floor.unit().monster[7].anger_timer, 900);
}

TEST(MagicCircles, WeaponRollsScaleAndTheQuarterTakesWhpToOne) {
    Circle circle(true, false);
    std::srand(3);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_UP));
    EXPECT_GE(circle.weapon->durability, 40 + 6);
    EXPECT_LE(circle.weapon->durability, 40 + 10);
    EXPECT_EQ(circle.weapon->durability % 2, 0); // a roll of 3-5, doubled

    int before = circle.weapon->durability;
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_DOWN));
    EXPECT_GE(before - circle.weapon->durability, 6);
    EXPECT_LE(before - circle.weapon->durability, 10);

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_QUARTER));
    EXPECT_EQ(circle.weapon->durability_f, 1.0f);
}

namespace {

// Xiao's equipped slingshot and the Crysknife in Toan's bag, both empty of ABS and worn.
void Wear(Circle &circle) {
    CUserStatus &status = circle.Status();
    status.equipped_weapon_slot[1] = 0;
    for (WEAPON_HAVE *weapon : {circle.weapon, &status.chara_weapons[1][0], &status.chara_weapons[0][1]}) {
        weapon->experience = 0;
        weapon->durability = 40;
        weapon->durability_f = 5.0f;
    }
    status.chara_weapons[1][0].item_no = ITEM_WEAPON_BANDIT_SLINGSHOT;
}

} // namespace

TEST(MagicCircles, DoubleFillsReachEveryEquippedWeapon) {
    Circle circle(true, false);
    Wear(circle);
    WEAPON_HAVE &slingshot = circle.Status().chara_weapons[1][0];
    WEAPON_HAVE &crysknife = circle.Status().chara_weapons[0][1];

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
    EXPECT_GT(circle.weapon->experience, 0);
    EXPECT_GT(slingshot.experience, 0);
    EXPECT_EQ(crysknife.experience, 0);

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_CURE));
    EXPECT_EQ(circle.weapon->durability_f, 40.0f);
    EXPECT_EQ(slingshot.durability_f, 40.0f);
    EXPECT_EQ(crysknife.durability_f, 5.0f);
    EXPECT_EQ(circle.Carried(ITEM_POWERUP_POWDER), 0);
}

TEST(MagicCircles, TripleFillsReachEveryCarriedWeapon) {
    Circle circle(true, true);
    Wear(circle);
    WEAPON_HAVE &crysknife = circle.Status().chara_weapons[0][1];

    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
    EXPECT_GT(crysknife.experience, 0);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_WHP_CURE));
    EXPECT_EQ(crysknife.durability_f, 40.0f);
}

TEST(MagicCircles, AbsFullLeavesAnOvertrainedWeaponAsItIs) {
    for (bool amplified : {false, true}) {
        Circle circle(amplified, false);
        circle.weapon->experience = 0;
        ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
        s16 max = circle.weapon->experience;
        ASSERT_GT(max, 0);

        // Past its max: not lowered to it, not raised further.
        circle.weapon->experience = static_cast<s16>(max + max / 2);
        ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ABS_FULL));
        EXPECT_EQ(circle.weapon->experience, max + max / 2);
    }
}

namespace {

constexpr int kBadCircles[] = {TRAP_CIRCLE_ALL_MONSTER_STAMINA, TRAP_CIRCLE_MONEY_DOWN, TRAP_CIRCLE_STAT_DOWN, TRAP_CIRCLE_WHP_DOWN,
                               TRAP_CIRCLE_WHP_QUARTER};

// The stats, elements and antis of a weapon, summed.
int StatSum(const WEAPON_HAVE &weapon) {
    return weapon.attack + weapon.endurance + weapon.speed + weapon.magic;
}

int ByteSum(const char *values, int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum;
}

void SetStats(WEAPON_HAVE &weapon, int stat, int attack) {
    weapon.attack = static_cast<s16>(attack);
    weapon.endurance = weapon.speed = weapon.magic = static_cast<s16>(stat);
    for (s8 &elem : weapon.elem) {
        elem = static_cast<s8>(stat);
    }
    for (char &anti : weapon.vs_monster) {
        anti = static_cast<char>(stat);
    }
}

// Monsters 0-3 in play, monster 2 gooed by a hit already.
void Populate(CMonstorUnit &unit) {
    for (int i = 0; i < 4; i++) {
        unit.monster[i].state = 1;
        unit.monster[i].hp = 50;
    }
    unit.monster[2].slow_timer = 180;
}

void StepSlow(int frames) {
    for (int i = 0; i < frames; i++) {
        expanded::CircleSlowStep();
    }
}

} // namespace

TEST(FavouredCircles, OwningTheArmletAnywhereFavoursTheCircles) {
    {
        Circle circle(false, false);
        EXPECT_FALSE(expanded::CirclesFavoured());
        circle.Status().chara_weapons[3][4].item_no = ITEM_WEAPON_SECRET_ARMLET; // in Ruby's bag
        EXPECT_TRUE(expanded::CirclesFavoured());
    }
    {
        Circle circle(false, false, true); // in storage
        EXPECT_TRUE(expanded::CirclesFavoured());
        expanded::test::OptionScope options;
        ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
        EXPECT_FALSE(expanded::CirclesFavoured());
    }
}

TEST(FavouredCircles, SuperStevesSphereDoesNotPassItOn) {
    Circle       circle(false, false);
    CUserStatus &status = circle.Status();
    WEAPON_HAVE &steve = status.chara_weapons[1][0];
    steve.item_no = ITEM_WEAPON_SUPER_STEVE;
    steve.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    steve.attach[0].sphere_weapon_no = ITEM_WEAPON_SECRET_ARMLET;
    status.cur_chara = CHARA_XIAO;
    status.equipped_weapon_slot[CHARA_XIAO] = 0;
    NowWeaponHave = &steve;
    ASSERT_EQ(expanded::SuperSteveSphere(), ITEM_WEAPON_SECRET_ARMLET);
    EXPECT_FALSE(expanded::CirclesFavoured());
    EXPECT_EQ(expanded::DealtCircle(TRAP_CIRCLE_MONEY_DOWN), TRAP_CIRCLE_MONEY_DOWN);
}

TEST(FavouredCircles, BadCirclesAreDealtAsGoodOnes) {
    {
        Circle circle(false, false, true);
        EXPECT_EQ(expanded::DealtCircle(TRAP_CIRCLE_MONEY_DOWN), TRAP_CIRCLE_MONEY_UP);
        EXPECT_EQ(expanded::DealtCircle(TRAP_CIRCLE_WHP_DOWN), TRAP_CIRCLE_WHP_UP);
        EXPECT_EQ(expanded::DealtCircle(TRAP_CIRCLE_WHP_QUARTER), TRAP_CIRCLE_WHP_CURE);
        // The rage and the stat loss turn good where they run.
        for (int kind : {0, 1, 2, 3, 4, 5, 7}) {
            EXPECT_EQ(expanded::DealtCircle(kind), kind);
        }
    }
    Circle circle(false, false); // without the armlet, every circle is itself
    for (int kind = 0; kind < TRAP_CIRCLE_COUNT; kind++) {
        EXPECT_EQ(expanded::DealtCircle(kind), kind);
    }
}

TEST(FavouredCircles, TheGameRunsTheGoodCounterpart) {
    Circle circle(false, false, true);
    circle.Status().money = 100;
    EXPECT_EQ(circle.Step(TRAP_CIRCLE_MONEY_DOWN), TRAP_CIRCLE_MONEY_UP);
    EXPECT_EQ(circle.Status().money, 100 + 120 + 10);

    std::srand(4);
    EXPECT_EQ(circle.Step(TRAP_CIRCLE_WHP_DOWN), TRAP_CIRCLE_WHP_UP);
    EXPECT_GE(circle.weapon->durability, 40 + 3);
    EXPECT_LE(circle.weapon->durability, 40 + 5);

    circle.weapon->durability_f = 5.0f;
    EXPECT_EQ(circle.Step(TRAP_CIRCLE_WHP_QUARTER), TRAP_CIRCLE_WHP_CURE);
    EXPECT_EQ(circle.weapon->durability_f, static_cast<float>(circle.weapon->durability));
}

TEST(FavouredCircles, WithoutTheArmletTheBadCirclesStayBad) {
    Circle circle(false, false);
    circle.Status().money = 100;
    EXPECT_EQ(circle.Step(TRAP_CIRCLE_MONEY_DOWN), TRAP_CIRCLE_MONEY_DOWN);
    EXPECT_EQ(circle.Status().money, 80);
    circle.weapon->durability_f = 40.0f;
    EXPECT_EQ(circle.Step(TRAP_CIRCLE_WHP_QUARTER), TRAP_CIRCLE_WHP_QUARTER);
    EXPECT_EQ(circle.weapon->durability_f, 10.0f);
    for (int kind : kBadCircles) {
        EXPECT_FALSE(circle.Run(kind)) << kind; // retail's, at retail strength
    }
}

TEST(FavouredCircles, TheStatLossIsAGainOfTheSameRolls) {
    for (unsigned seed : {1u, 2u, 3u, 9u}) {
        Circle lost(false, false); // the game's own loss
        SetStats(*lost.weapon, 50, 50);
        std::srand(seed);
        ASSERT_EQ(lost.Step(TRAP_CIRCLE_STAT_DOWN), TRAP_CIRCLE_STAT_DOWN);

        Circle gained(false, false, true);
        SetStats(*gained.weapon, 50, 50);
        std::srand(seed);
        EXPECT_EQ(expanded::ExpandedCircle(TRAP_CIRCLE_STAT_DOWN, *gained.weapon), TRAP_CIRCLE_STAT_DOWN);

        int gain = StatSum(*gained.weapon) - 200;
        EXPECT_GE(gain, 2);
        EXPECT_LE(gain, 4);
        EXPECT_EQ(StatSum(*lost.weapon), 200 - gain);
        int elem_gain = ByteSum(gained.weapon->elem, 5) - 250, anti_gain = ByteSum(gained.weapon->vs_monster, 10) - 500;
        EXPECT_GE(elem_gain, 2);
        EXPECT_LE(elem_gain, 4);
        EXPECT_GE(anti_gain, 3 * 2);
        EXPECT_LE(anti_gain, 3 * 3);
        EXPECT_EQ(ByteSum(lost.weapon->elem, 5), 250 - elem_gain);
        EXPECT_EQ(ByteSum(lost.weapon->vs_monster, 10), 500 - anti_gain);
    }
}

TEST(FavouredCircles, GainsStopAt99AndLeaveHigherStatsAlone) {
    for (unsigned seed = 1; seed <= 12; seed++) {
        Circle circle(false, false, true);
        SetStats(*circle.weapon, 98, 150);
        std::srand(seed);
        ASSERT_TRUE(circle.Run(TRAP_CIRCLE_STAT_DOWN));
        const WEAPON_HAVE &weapon = *circle.weapon;
        EXPECT_EQ(weapon.attack, 150);
        int others = weapon.endurance + weapon.speed + weapon.magic;
        EXPECT_GE(others, 98 * 3);
        EXPECT_LE(others, 98 * 3 + 1);
        for (int value : {weapon.endurance, weapon.speed, weapon.magic}) {
            EXPECT_LE(value, 99);
        }
        for (char value : weapon.vs_monster) {
            EXPECT_LE(value, 99);
        }
        for (s8 value : weapon.elem) {
            EXPECT_LE(value, 99);
        }
    }
}

TEST(FavouredCircles, RageBecomesEveryMonsterSlowedForFiveSeconds) {
    Circle                        circle(false, false, true);
    expanded::test::StepsUnpaused steps;
    CMonstorUnit                 &unit = circle.floor.unit();
    Populate(unit);
    EXPECT_EQ(expanded::ExpandedCircle(TRAP_CIRCLE_ALL_MONSTER_STAMINA, *circle.weapon),
              expanded::kMonstersSlowedMessage - expanded::kCircleMessages);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(unit.monster[i].anger_timer, 0);
        EXPECT_EQ(unit.monster[i].slow_timer, i == 2 ? 180 : expanded::kCircleSlowFrames) << i;
    }

    StepSlow(expanded::kCircleSlowFrames - 1);
    EXPECT_EQ(unit.monster[0].slow_timer, expanded::kCircleSlowFrames);
    unit.monster[3].slow_timer = 180; // gooed by a hit meanwhile: its own goo from then on
    StepSlow(1);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(unit.monster[i].slow_timer, i == 2 || i == 3 ? 180 : 0) << i;
    }
}

TEST(FavouredCircles, TheSlowCountsOnlyStepsTheMonstersTake) {
    Circle        circle(false, false, true);
    CMonstorUnit &unit = circle.floor.unit();
    Populate(unit);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ALL_MONSTER_STAMINA));
    StepSlow(2 * expanded::kCircleSlowFrames); // paused: an event or a menu
    EXPECT_EQ(unit.monster[0].slow_timer, expanded::kCircleSlowFrames);
    expanded::test::StepsUnpaused steps;
    StepSlow(expanded::kCircleSlowFrames);
    EXPECT_EQ(unit.monster[0].slow_timer, 0);
}

TEST(FavouredCircles, TheSlowEndsWithItsFloor) {
    Circle                        circle(false, false, true);
    expanded::test::StepsUnpaused steps;
    CMonstorUnit                 &unit = circle.floor.unit();
    Populate(unit);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ALL_MONSTER_STAMINA));
    circle.floor.SetUpModel(0, expanded::Monster::MUMMY); // the next floor's kinds set up
    StepSlow(expanded::kCircleSlowFrames);
    EXPECT_EQ(unit.monster[0].slow_timer, expanded::kCircleSlowFrames); // nothing left to end on it
}

TEST(FavouredCircles, AmplifiedFavouredCirclesKeepTheirStrength) {
    Circle                        circle(true, false, true);
    expanded::test::StepsUnpaused steps;
    CMonstorUnit                 &unit = circle.floor.unit();
    Populate(unit);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_ALL_MONSTER_STAMINA));
    EXPECT_EQ(unit.monster[0].slow_timer, 2 * expanded::kCircleSlowFrames);
    StepSlow(2 * expanded::kCircleSlowFrames - 1);
    EXPECT_GT(unit.monster[0].slow_timer, 0);
    StepSlow(1);
    EXPECT_EQ(unit.monster[0].slow_timer, 0);

    circle.Status().money = 100;
    EXPECT_EQ(circle.Step(TRAP_CIRCLE_MONEY_DOWN), TRAP_CIRCLE_MONEY_UP);
    EXPECT_EQ(circle.Status().money, 100 + 2 * (120 + 10));

    SetStats(*circle.weapon, 50, 50);
    ASSERT_TRUE(circle.Run(TRAP_CIRCLE_STAT_DOWN));
    int gain = StatSum(*circle.weapon) - 200;
    EXPECT_GE(gain, 4);
    EXPECT_LE(gain, 8);
    EXPECT_EQ(gain % 2, 0);
}
