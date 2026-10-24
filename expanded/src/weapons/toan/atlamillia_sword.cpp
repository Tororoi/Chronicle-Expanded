#include "dungeon.h"
#include "expanded/weapons/toan/atlamillia_sword.hpp"

#include <algorithm>
#include <vector>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "dungeonparts.hpp"
#include "expanded/core/random.hpp"
#include "expanded/core/save.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"
#include "menu_misc.hpp"
#include "userstatus.hpp"

// dungeonparts.cpp: whether the game lets an atla lie on a floor (1-based).
int chkAtraFloor(int dungeon, int floor);

namespace expanded {

namespace {

// The game's floors per dungeon (dungeonparts.cpp's MaxFloorTbl), and the Demon Shaft's; the last is never chosen,
// as the game never places an atla there.
constexpr int kFloors[] = {15, 17, 18, 18, 15, 25, 100};
constexpr int kTries = 64;

// One insured SynthSphere waiting in the save: its fields, kFields ints each, in SaveValue kInsured.
enum Field {
    kId,
    kDungeon,
    kFloor,
    kWeapon,
    kLevel,
    kFlags,
    kStatus,
    kElem = kStatus + 4,
    kVsMonster = kElem + 5,
    kFields = kVsMonster + 10
};

const SaveValue<std::vector<int>> kInsured{"atlamillia.insured"};

std::vector<int> placed_ids; // the insured atlas placed on the current floor
bool             due = false;

int RandomFloor(int dungeon) {
    int last = kFloors[dungeon] - 2;
    for (int i = 0; i < kTries; i++) {
        int floor = ModRoll(last + 1);
        if (dungeon == DUNGEON_DEMON_SHAFT || chkAtraFloor(dungeon, floor + 1) != 0) {
            return floor;
        }
    }
    return 0;
}

void Taken(int id) {
    std::vector<int> insured = kInsured.Get();
    for (size_t at = 0; at + kFields <= insured.size(); at += kFields) {
        if (insured[at + kId] == id) {
            insured.erase(insured.begin() + static_cast<long>(at), insured.begin() + static_cast<long>(at + kFields));
            break;
        }
    }
    if (insured.empty()) {
        kInsured.Reset();
    } else {
        kInsured.Set(insured);
    }
}

AtlaDrop Drop(const int *entry) {
    AtlaDrop drop;
    drop.weapon = entry[kWeapon];
    drop.sphere = true;
    drop.own_values = true;
    drop.level = static_cast<s8>(entry[kLevel]);
    drop.flags = static_cast<s16>(entry[kFlags]);
    for (int i = 0; i < 4; i++) {
        drop.status[i] = static_cast<s16>(entry[kStatus + i]);
    }
    for (int i = 0; i < 5; i++) {
        drop.elem[i] = static_cast<s8>(entry[kElem + i]);
    }
    for (int i = 0; i < 10; i++) {
        drop.vs_monster[i] = static_cast<s8>(entry[kVsMonster + i]);
    }
    drop.taken = Taken;
    drop.tag = entry[kId];
    return drop;
}

// A free place on the floor, as buildEventData finds one for an atla: clear of chests, atlas and magic circles.
bool FreePlace(CDungeonMap &map, float *pos) {
    for (int i = 0; i < kTries; i++) {
        if (SearchiDoPutArea(map.cells, 0, 0, 20, 20, pos) <= 0) {
            return false;
        }
        if (map.CheckTreasureBox(pos, 20.0f) != 0 && map.CheckAtra(pos, 20.0f) != 0 && map.CheckTrapCircle(pos, 20.0f) == nullptr) {
            return true;
        }
    }
    return false;
}

} // namespace

float InsuranceShare(const WEAPON_HAVE &weapon) {
    return std::min(kInsuranceShareMost, kInsuranceShare * static_cast<float>(std::max(0, static_cast<int>(weapon.level)) + 1));
}

void AtlamilliaWeaponBroken(const WEAPON_HAVE &weapon) {
    if (UserStatus == nullptr || selectMapNo < 0 || selectMapNo > DUNGEON_DEMON_SHAFT || !WeaponOwned(ITEM_WEAPON_ATLAMILLIA_SWORD)) {
        return;
    }
    std::vector<int> insured = kInsured.Get();
    int              id = 1;
    for (size_t at = 0; at + kFields <= insured.size(); at += kFields) {
        id = std::max(id, insured[at + kId] + 1);
    }
    const float share = InsuranceShare(weapon);
    int         entry[kFields] = {};
    entry[kId] = id;
    entry[kDungeon] = selectMapNo;
    entry[kFloor] = RandomFloor(selectMapNo);
    entry[kWeapon] = weapon.item_no;
    entry[kLevel] = weapon.level;
    entry[kFlags] = CheckWeaponOptionStatus(weapon.flags);
    const s16 stats[] = {weapon.attack, weapon.endurance, weapon.speed, weapon.magic};
    for (int i = 0; i < 4; i++) {
        entry[kStatus + i] = static_cast<s16>(share * static_cast<float>(stats[i]));
    }
    for (int i = 0; i < 5; i++) {
        entry[kElem + i] = static_cast<s8>(share * static_cast<float>(weapon.elem[i]));
    }
    for (int i = 0; i < 10; i++) {
        entry[kVsMonster + i] = static_cast<s8>(share * static_cast<float>(weapon.vs_monster[i]));
    }
    insured.insert(insured.end(), entry, entry + kFields);
    kInsured.Set(insured);
    // Broken on the floor it is insured to: it is placed on the next step.
    due = true;
}

void AtlamilliaFloorLoaded() {
    placed_ids.clear();
    due = true;
}

void AtlamilliaStep() {
    if (!due || UserStatus == nullptr || NowDngMap != &MainDungeonMap) {
        return;
    }
    due = false;
    std::vector<int> insured = kInsured.Get();
    for (size_t at = 0; at + kFields <= insured.size(); at += kFields) {
        const int *entry = &insured[at];
        if (entry[kDungeon] != selectMapNo || entry[kFloor] != UserStatus->cur_floor ||
            std::find(placed_ids.begin(), placed_ids.end(), entry[kId]) != placed_ids.end()) {
            continue;
        }
        float pos[4];
        if (FreePlace(*NowDngMap, pos) && PlaceAtlaDrop(pos, Drop(entry)) >= 0) {
            placed_ids.push_back(entry[kId]);
        }
    }
}

int InsuredSpheres() {
    return static_cast<int>(kInsured.Get().size() / kFields);
}

} // namespace expanded
