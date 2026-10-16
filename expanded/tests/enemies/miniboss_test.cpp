#include <gtest/gtest.h>

#include <cmath>
#include <deque>
#include <memory>
#include <set>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/enemies/miniboss.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/scaling.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace {

using expanded::Monster;
using expanded::No;

std::vector<float> g_chances;
std::vector<int>   g_counts;
std::deque<int>    g_picks;
int                g_promote = -1;

// The chances asked since the last reset; the g_promote-th passes, or every one when g_promote is -1.
bool ScriptedChance(float chance) {
    int roll = static_cast<int>(g_chances.size());
    g_chances.push_back(chance);
    return g_promote == -1 || roll == g_promote;
}

// The next of g_picks, else 0: the species in the order they first appear, and the first of each.
int ScriptedPick(int count) {
    g_counts.push_back(count);
    if (g_picks.empty()) {
        return 0;
    }
    int pick = g_picks.front();
    g_picks.pop_front();
    return pick;
}

// A species of n monsters' chance at this rate.
float ChanceOf(float rate, int n) {
    return 1.0f - std::pow(1.0f - rate, static_cast<float>(n));
}

// Forgets the draws asked; the scripted picks stay.
void ResetRolls() {
    g_chances.clear();
    g_counts.clear();
}

class Miniboss : public ::testing::Test {
protected:
    void SetUp() override {
        selectMapNo = 0;
        ResetRolls();
        g_picks.clear();
        g_promote = -1;
        expanded::SetMinibossRoll(&ScriptedChance, &ScriptedPick);
        expanded::MinibossFloorEntered();
        unit = std::make_unique<CMonstorUnit>();
        for (MONSTOR &monster : unit->monster) {
            monster.state = -1;
        }
        SetUpModel(0, Monster::SKELETON_SOLDIER);
        SetUpModel(1, Monster::WEREWOLF);
        SetUpModel(2, Monster::CANNIBAL_PLANT);
        SetUpModel(3, Monster::CAVE_BAT);
        SetUpModel(4, Monster::DASHER);
        SetUpModel(5, Monster::FLI_FLI);
        SetUpModel(6, Monster::HORNET);
        SetUpModel(7, Monster::HALLOWEEN);
        SetUpModel(8, Monster::EARTH_DIGGER);
    }

    void TearDown() override {
        expanded::SetMinibossRoll(nullptr, nullptr);
        selectMapNo = -1;
    }

    // SetupBaseModel's part: the kind's record in the model slot.
    void SetUpModel(int model_slot, Monster monster) {
        unit->model[model_slot] = MonstorTable[No(monster)];
        expanded::EnemyModelSetUp(*unit, model_slot, No(monster));
    }

    // SetupViewMonstor's part: the record on the monster.
    void Spawn(int monster, int model_slot) {
        MONSTOR             &live = unit->monster[monster];
        const MONSTOR_MODEL &model = unit->model[model_slot];
        live = {};
        live.state = 2;
        live.base_model = model_slot;
        live.max_hp = live.hp = model.max_hp;
        live.defense = model.defense;
        live.hardness = model.hardness;
        live.exp = model.exp;
        live.money = model.money;
        live.money_chance = model.money_chance;
        live.kind = model.kind;
        live.name_no = model.name_no;
        live.drops_items = model.drops_items;
        live.item_damage_rate = model.item_damage_rate;
        live.drop_item = -1;
        live.lockon_scale_x = live.lockon_scale_y = 1.0f;
        live.requested_motion_speed = -1.0f;
        unit->effect[monster] = {};
        unit->effect2[monster] = {};
        unit->chara[monster][0].SetScale(1.0f, 1.0f, 1.0f);
        expanded::EnemySpawned(*unit, monster);
    }

    // CleanViewMonstor and ArrangementPos: the floor (back 0) or its back floor (1) populated with these kinds.
    void Arrange(int back, std::initializer_list<int> model_slots) {
        for (MONSTOR &monster : unit->monster) {
            monster.state = -1;
        }
        unit->back_dungeon = back;
        int monster = 0;
        for (int model_slot : model_slots) {
            Spawn(monster++, model_slot);
        }
        ResetRolls();
        expanded::MinibossesArranged(*unit);
    }

    // The death script and Step's reward.
    void Defeat(int monster) {
        unit->monster[monster].hp = 0;
        unit->monster[monster].state = -1;
        expanded::MinibossDefeated(*unit, monster);
    }

    // Whether each species has at most one miniboss.
    bool OnePerSpecies() {
        std::set<int> slots;
        for (int monster = 0; monster < 16; monster++) {
            if (unit->monster[monster].state != -1 && expanded::IsMiniboss(monster) &&
                !slots.insert(unit->monster[monster].base_model).second) {
                return false;
            }
        }
        return true;
    }

    int Minibosses() {
        int count = 0;
        for (int monster = 0; monster < 16; monster++) {
            count += unit->monster[monster].state != -1 && expanded::IsMiniboss(monster);
        }
        return count;
    }

    std::unique_ptr<CMonstorUnit> unit;
};

} // namespace

TEST_F(Miniboss, EachSpeciesRollsOnceForItsEligibleMonsters) {
    for (MONSTOR &monster : unit->monster) {
        monster.state = -1;
    }
    for (int monster = 0; monster < 15; monster++) {
        Spawn(monster, monster % 2);
    }
    unit->monster[3].drop_item = ITEM_DRAN_S_CREST;
    unit->monster[5].kind = MONSTER_KIND_NO_LOCK_ON;
    expanded::MinibossesArranged(*unit);

    ASSERT_EQ(g_chances.size(), 2u);
    EXPECT_FLOAT_EQ(g_chances[0], ChanceOf(expanded::kMinibossFirstRate, 8));
    EXPECT_FLOAT_EQ(g_chances[1], ChanceOf(expanded::kMinibossLaterRate, 5));
    EXPECT_EQ(Minibosses(), 2);
    EXPECT_TRUE(OnePerSpecies());
    EXPECT_TRUE(expanded::IsMiniboss(0));
    EXPECT_TRUE(expanded::IsMiniboss(1));
    EXPECT_FALSE(expanded::IsMiniboss(3));
    EXPECT_FALSE(expanded::IsMiniboss(5));
}

TEST_F(Miniboss, TheLaterRateFollowsTheFirstPass) {
    g_promote = 1;
    Arrange(0, {0, 0, 1, 1, 1, 2, 3, 3});
    ASSERT_EQ(g_chances.size(), 4u);
    EXPECT_FLOAT_EQ(g_chances[0], ChanceOf(expanded::kMinibossFirstRate, 2));
    EXPECT_FLOAT_EQ(g_chances[1], ChanceOf(expanded::kMinibossFirstRate, 3));
    EXPECT_FLOAT_EQ(g_chances[2], ChanceOf(expanded::kMinibossLaterRate, 1));
    EXPECT_FLOAT_EQ(g_chances[3], ChanceOf(expanded::kMinibossLaterRate, 2));
    EXPECT_EQ(Minibosses(), 1);
}

TEST_F(Miniboss, TheMinibossComesFromTheRollingSpecies) {
    g_promote = 0;
    g_picks = {2, 1};
    Arrange(0, {0, 1, 0, 2, 2, 2, 1});
    EXPECT_EQ(g_counts, (std::vector<int>{3, 3, 2, 1}));
    EXPECT_EQ(Minibosses(), 1);
    EXPECT_TRUE(expanded::IsMiniboss(4));
}

TEST_F(Miniboss, NoSpeciesGetsTwo) {
    Arrange(0, {0, 0, 0, 1, 1, 2});
    EXPECT_EQ(g_chances.size(), 3u);
    EXPECT_EQ(Minibosses(), 3);
    EXPECT_TRUE(OnePerSpecies());
}

TEST_F(Miniboss, TheBackFloorRollsTheSameWay) {
    g_promote = 99;
    Arrange(0, {0, 1});
    g_promote = 1;
    Arrange(1, {2, 2, 0, 0, 0});
    ASSERT_EQ(g_chances.size(), 2u);
    EXPECT_FLOAT_EQ(g_chances[0], ChanceOf(expanded::kMinibossFirstRate, 2));
    EXPECT_FLOAT_EQ(g_chances[1], ChanceOf(expanded::kMinibossFirstRate, 3));
    EXPECT_EQ(Minibosses(), 1);
    EXPECT_TRUE(expanded::IsMiniboss(2));
}

// The real dice over many floors of these species sizes, against the exact none / one / two-or-more rates.
TEST_F(Miniboss, FloorsGetAboutOneMiniboss) {
    expanded::SetMinibossRoll(nullptr, nullptr);

    struct Floor {
        std::vector<int> sizes;
        double           none, one, more;
    };

    const Floor floors[] = {
        {{7, 7},                      0.167, 0.785, 0.048},
        {{5, 4, 3, 2},                0.167, 0.767, 0.066},
        {{3, 3, 3, 3, 2},             0.167, 0.762, 0.071},
        {{2, 2, 2, 2, 2, 2, 2},       0.167, 0.758, 0.075},
        {{2, 2, 2, 2, 2, 1, 1, 1, 1}, 0.167, 0.756, 0.077},
    };
    constexpr int kFloors = 20000;
    for (const Floor &floor : floors) {
        int counts[3] = {};
        for (int i = 0; i < kFloors; i++) {
            expanded::MinibossFloorEntered();
            for (MONSTOR &monster : unit->monster) {
                monster.state = -1;
            }
            unit->back_dungeon = i % 2;
            int monster = 0;
            for (int slot = 0; slot < static_cast<int>(floor.sizes.size()); slot++) {
                for (int n = 0; n < floor.sizes[slot]; n++) {
                    Spawn(monster++, slot);
                }
            }
            expanded::MinibossesArranged(*unit);
            ASSERT_TRUE(OnePerSpecies());
            counts[std::min(Minibosses(), 2)]++;
        }
        EXPECT_NEAR(counts[0] / double(kFloors), floor.none, 0.015);
        EXPECT_NEAR(counts[1] / double(kFloors), floor.one, 0.015);
        EXPECT_NEAR(counts[2] / double(kFloors), floor.more, 0.01);
    }
}

TEST_F(Miniboss, NoRollOffTheDungeons) {
    selectMapNo = -1;
    Arrange(0, {0, 1});
    EXPECT_TRUE(g_chances.empty());
    EXPECT_EQ(Minibosses(), 0);
}

TEST_F(Miniboss, GrowsItsStatsBodyAndReach) {
    g_promote = 1;
    Arrange(0, {0, 1});
    ASSERT_TRUE(expanded::IsMiniboss(1));

    const MONSTOR       &werewolf = unit->monster[1];
    const MONSTOR_MODEL &model = MonstorTable[No(Monster::WEREWOLF)];
    EXPECT_EQ(werewolf.max_hp, model.max_hp * 3);
    EXPECT_EQ(werewolf.hp, model.max_hp * 3);
    EXPECT_EQ(werewolf.exp, model.exp * 3);
    EXPECT_EQ(werewolf.defense, expanded::ScaleStat(model.defense, 1.5f));
    EXPECT_EQ(werewolf.money, model.money * 4);
    EXPECT_EQ(werewolf.money_chance, 100);
    EXPECT_EQ(werewolf.drops_items, 1);
    EXPECT_EQ(werewolf.item_damage_rate, 10);
    EXPECT_FLOAT_EQ(werewolf.lockon_scale_x, 1.5f);
    EXPECT_FLOAT_EQ(unit->chara[1][0].scale[1], 1.5f);
    EXPECT_EQ(expanded::EnemyMeleeDamage(1, 100), 150);

    EXPECT_EQ(unit->monster[0].max_hp, MonstorTable[No(Monster::SKELETON_SOLDIER)].max_hp);
    EXPECT_FLOAT_EQ(unit->chara[0][0].scale[1], 1.0f);
}

TEST_F(Miniboss, GrowsTheSpheresTheScriptSetUp) {
    for (int monster = 0; monster < 2; monster++) {
        Spawn(monster, 1);
        unit->effect[monster].timer[0] = 1;
        unit->effect[monster].radius[0] = 6.0f;
        unit->effect2[monster].active[0] = 1;
        unit->effect2[monster].radius[0] = 5.0f;
    }
    expanded::MakeMiniboss(*unit, 1);

    EXPECT_FLOAT_EQ(unit->effect[1].radius[0], 9.0f);
    EXPECT_FLOAT_EQ(unit->effect2[1].radius[0], 8.0f);
    EXPECT_FLOAT_EQ(unit->effect[0].radius[0], 6.0f);

    expanded::MakeMiniboss(*unit, 1);
    EXPECT_FLOAT_EQ(unit->effect[1].radius[0], 9.0f);
}

TEST_F(Miniboss, SeesThePlayerNearer) {
    g_promote = 1;
    Arrange(0, {0, 1, 2});
    EXPECT_FLOAT_EQ(expanded::MinibossScriptDistance(1, 1, 130.0f), 100.0f);
    EXPECT_FLOAT_EQ(expanded::MinibossScriptDistance(1, 4, 130.0f), 130.0f);
    EXPECT_FLOAT_EQ(expanded::MinibossScriptDistance(0, 1, 130.0f), 130.0f);

    expanded::MakeMiniboss(*unit, 2);
    EXPECT_FLOAT_EQ(expanded::MinibossScriptDistance(2, 1, 150.0f), 100.0f);
}

TEST_F(Miniboss, WalksSlowerToKeepPace) {
    g_promote = -1;
    Arrange(0, {1, 3});
    for (int monster = 0; monster < 2; monster++) {
        unit->monster[monster].requested_motion_speed = 1.3f;
        unit->chara[monster][0].SetMotion(1, 0, 1.3f);
        expanded::MinibossMotion(*unit, monster);
    }
    EXPECT_FLOAT_EQ(unit->chara[0][0].motion_speed, 1.0f);
    EXPECT_FLOAT_EQ(unit->chara[1][0].motion_speed, 1.3f);

    unit->chara[0][0].SetMotion(5, 0, 1.3f);
    unit->monster[0].requested_motion_speed = 1.3f;
    expanded::MinibossMotion(*unit, 0);
    EXPECT_FLOAT_EQ(unit->chara[0][0].motion_speed, 1.3f);
}

TEST_F(Miniboss, BackFloorRollsItsOwnAndALivingOneComesBack) {
    g_promote = 1;
    Arrange(0, {0, 0, 1, 0});
    ASSERT_TRUE(expanded::IsMiniboss(2));

    g_promote = 99;
    Arrange(1, {0, 1, 0});
    EXPECT_EQ(g_chances.size(), 2u);
    EXPECT_EQ(Minibosses(), 0);

    Arrange(0, {0, 0, 0, 0, 0, 1});
    EXPECT_TRUE(g_chances.empty());
    EXPECT_EQ(g_counts, std::vector<int>{1});
    EXPECT_EQ(Minibosses(), 1);
    EXPECT_TRUE(expanded::IsMiniboss(5));

    Arrange(1, {1, 1});
    EXPECT_EQ(Minibosses(), 0);
}

TEST_F(Miniboss, ADefeatedOneStaysDefeated) {
    g_promote = 0;
    Arrange(0, {1, 0});
    ASSERT_TRUE(expanded::IsMiniboss(0));
    Defeat(0);

    g_promote = 0;
    Arrange(1, {0});
    EXPECT_TRUE(expanded::IsMiniboss(0));
    Arrange(0, {1, 1, 0});
    EXPECT_EQ(Minibosses(), 0);

    Arrange(1, {0, 0});
    EXPECT_EQ(Minibosses(), 1);
}

TEST_F(Miniboss, AReturningOneTakesNoKey) {
    g_promote = 0;
    Arrange(0, {1, 0});
    Arrange(1, {});

    for (MONSTOR &monster : unit->monster) {
        monster.state = -1;
    }
    unit->back_dungeon = 0;
    Spawn(0, 0);
    Spawn(1, 1);
    unit->monster[1].drop_item = ITEM_DRAN_S_CREST;
    expanded::MinibossesArranged(*unit);

    EXPECT_TRUE(expanded::IsMiniboss(1));
    EXPECT_NE(unit->monster[1].drop_item, ITEM_DRAN_S_CREST);
    EXPECT_EQ(unit->monster[0].drop_item, ITEM_DRAN_S_CREST);
}

TEST_F(Miniboss, ANewFloorRollsAgain) {
    g_promote = 0;
    Arrange(0, {1});
    Defeat(0);
    expanded::MinibossFloorEntered();
    Arrange(0, {1});
    ASSERT_EQ(g_chances.size(), 1u);
    EXPECT_FLOAT_EQ(g_chances[0], ChanceOf(expanded::kMinibossFirstRate, 1));
    EXPECT_TRUE(expanded::IsMiniboss(0));
}

TEST_F(Miniboss, ASpawnClearsTheSlot) {
    g_promote = 0;
    Arrange(0, {1});
    ASSERT_TRUE(expanded::IsMiniboss(0));
    Spawn(0, 0);
    EXPECT_FALSE(expanded::IsMiniboss(0));
}

TEST_F(Miniboss, AReturnPlacesTheLivingOnesKindsFirst) {
    g_promote = 1;
    Arrange(0, {0, 1, 0});
    ASSERT_TRUE(expanded::IsMiniboss(1));
    EXPECT_EQ(expanded::MinibossPlacedModel(*unit, 0, 0), 0);

    Arrange(1, {0});
    unit->back_dungeon = 0;
    unit->model_count = 4;
    EXPECT_EQ(expanded::MinibossPlacedModel(*unit, 0, 3), 1);
    EXPECT_EQ(expanded::MinibossPlacedModel(*unit, 1, 3), 3);
}
