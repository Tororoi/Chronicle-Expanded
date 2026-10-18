#include "expanded/enemies/boss_record.hpp"

#include <algorithm>
#include <vector>

#include "expanded/core/save.hpp"
#include "expanded/enemies/scaling.hpp"
#include "monstorunit.hpp"
#include "savedata.hpp"

namespace expanded {

namespace {

const SaveValue<std::vector<int>> kDefeated{"bosses.defeated"};

// Each boss and its dungeon; past the Moon Sea none has a dungeon after it to show it beaten.
struct Boss {
    Monster kind;
    int     dungeon;
};

constexpr Boss kBosses[] = {
    {Monster::DRAN,               0},
    {Monster::MASTER_UTAN,        1},
    {Monster::ICE_QUEEN,          2},
    {Monster::KINGS_CURSE_COFFIN, 3},
    {Monster::MINOTAUR_JOE,       4},
    {Monster::BLACK_KNIGHT,       6},
};

const Boss *Find(int model_no) {
    for (const Boss &boss : kBosses) {
        if (No(boss.kind) == model_no) {
            return &boss;
        }
    }
    return nullptr;
}

} // namespace

bool BossDefeated(Monster kind) {
    const Boss *boss = Find(No(kind));
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
    if (Find(model_no) == nullptr) {
        return;
    }
    std::vector<int> defeated = kDefeated.Get();
    if (std::find(defeated.begin(), defeated.end(), model_no) == defeated.end()) {
        defeated.push_back(model_no);
        kDefeated.Set(defeated);
    }
}

} // namespace expanded
