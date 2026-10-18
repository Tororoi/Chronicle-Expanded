#include "expanded/enemies/boss_record.hpp"

#include <algorithm>
#include <vector>

#include "expanded/core/save.hpp"
#include "expanded/enemies/boss_table.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"
#include "savedata.hpp"

namespace expanded {

namespace {

const SaveValue<std::vector<int>> kDefeated{"bosses.defeated"};

} // namespace

// Past the Moon Sea no boss has a dungeon after it to show it beaten.
bool BossDefeated(Monster kind) {
    const BossEntry *boss = BossEntryOf(No(kind));
    if (boss == nullptr) {
        return false;
    }
    std::vector<int> defeated = kDefeated.Get();
    if (std::find(defeated.begin(), defeated.end(), No(kind)) != defeated.end()) {
        return true;
    }
    return SaveData != nullptr && boss->dungeon < 5 && SaveData->QuestDungeon(boss->dungeon + 1, 0) > 0;
}

void BossRecordKill(const CMonstorUnit &unit, int monster) {
    int model_no = ModelKind(unit.monster[monster].base_model);
    if (BossEntryOf(model_no) == nullptr) {
        return;
    }
    std::vector<int> defeated = kDefeated.Get();
    if (std::find(defeated.begin(), defeated.end(), model_no) == defeated.end()) {
        defeated.push_back(model_no);
        kDefeated.Set(defeated);
    }
}

} // namespace expanded
