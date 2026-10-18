#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/enemies/boss_table.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

namespace {

using expanded::Monster;
using expanded::No;

std::vector<int> g_heard; // -1 for a floor, else the model slot

void OnFloor() {
    g_heard.push_back(-1);
}

void OnModel(CMonstorUnit &, int model_slot, int) {
    g_heard.push_back(model_slot);
}

[[maybe_unused]] const bool kListening = (expanded::AddEnemyFloorListener(OnFloor), expanded::AddEnemyModelListener(OnModel), true);

} // namespace

TEST(EnemyEvents, AFloorIsHeardOnceBeforeItsFirstKind) {
    expanded::test::TestFloor floor;
    g_heard.clear();
    for (int slot = 0; slot < 3; slot++) {
        floor.SetUpModel(slot, Monster::WEREWOLF);
    }
    EXPECT_EQ(g_heard, (std::vector<int>{-1, 0, 1, 2}));
}

TEST(EnemyEvents, IsKindTakesALiveMonsterOfItsSlotsKind) {
    expanded::test::TestFloor floor;
    CMonstorUnit             &unit = floor.unit();
    floor.SetUpModel(0, Monster::CAVE_BAT);
    floor.SetUpModel(2, Monster::WEREWOLF);
    unit.monster[5].base_model = 2;
    EXPECT_FALSE(expanded::IsKind(unit, 5, Monster::WEREWOLF)); // not spawned
    unit.monster[5].state = 1;
    EXPECT_TRUE(expanded::IsKind(unit, 5, Monster::WEREWOLF));
    EXPECT_FALSE(expanded::IsKind(unit, 5, Monster::CAVE_BAT));
    EXPECT_FALSE(expanded::IsKind(unit, -1, Monster::WEREWOLF));
    EXPECT_FALSE(expanded::IsKind(unit, expanded::kFloorMonsters, Monster::WEREWOLF));
}

TEST(BossTable, EachBossIsSupportedAndItsPartnerSoundsFromItsArena) {
    for (const expanded::BossEntry &boss : expanded::kBossTable) {
        EXPECT_TRUE(expanded::BossSupported(boss.kind)) << No(boss.kind);
        EXPECT_EQ(expanded::BossEntryOf(No(boss.kind)), &boss);
        EXPECT_GE(boss.sound_set, 200);
    }
    EXPECT_EQ(expanded::BossEntryOf(No(Monster::WEREWOLF)), nullptr);
    EXPECT_FALSE(expanded::BossSupported(Monster::KINGS_CURSE)); // the coffin's, raised by it
}
