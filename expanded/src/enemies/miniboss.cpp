#include "expanded/enemies/miniboss.hpp"

#include <algorithm>
#include <random>
#include <vector>

#include "dun/gameloop.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;

// The chase motion of every monster.
constexpr int kWalkMotion = 1;

// Monster names (MONSTOR::name_no) of the long-reach kinds: Cannibal Plant, Pirate's Chariot, Dark Flower,
// Cursed Rose, Rockanoff, Opar and King Prickly.
constexpr int kLongReach[] = {11, 25, 67, 68, 77, 304, 306};

// The bats, whose motion 1 is a flying loop with no feet to keep pace.
constexpr int kFlyers[] = {60, 61};

bool  miniboss[kMonsters];
float range[kMonsters];
bool  walk_synced[kMonsters];

// The MonstorTable kinds of the living minibosses on the floor (0) and its back floor (1), once populated.
struct Side {
    bool             populated = false;
    std::vector<int> kinds;
};

Side sides[2];

MinibossRoll roll_override = nullptr;

int Roll(int eligible) {
    if (roll_override != nullptr) {
        return roll_override(eligible);
    }
    static std::minstd_rand rng{std::random_device{}()};
    return std::uniform_int_distribution<int>(0, eligible - 1)(rng);
}

template <size_t N>
bool Contains(const int (&list)[N], int value) {
    return std::find(std::begin(list), std::end(list), value) != std::end(list);
}

Side &CurrentSide(const CMonstorUnit &unit) {
    return sides[unit.back_dungeon != 0];
}

int Kind(const CMonstorUnit &unit, int monster) {
    return ModelKind(unit.monster[monster].base_model);
}

// A monster on the floor that is no boss and no miniboss.
bool Plain(const CMonstorUnit &unit, int monster) {
    const MONSTOR &live = unit.monster[monster];
    return live.state != -1 && live.kind != MONSTER_KIND_NO_LOCK_ON && !miniboss[monster];
}

// A plain monster that carries no key.
bool Eligible(const CMonstorUnit &unit, int monster) {
    return Plain(unit, monster) && unit.monster[monster].drop_item == -1;
}

// Each eligible monster has one chance in their number: about one miniboss a floor.
void RollMinibosses(CMonstorUnit &unit) {
    std::vector<int> eligible;
    for (int monster = 0; monster < kMonsters; monster++) {
        if (Eligible(unit, monster)) {
            eligible.push_back(monster);
        }
    }
    int count = static_cast<int>(eligible.size());
    for (int monster : eligible) {
        if (Roll(count) == 0) {
            MakeMiniboss(unit, monster);
        }
    }
}

// The monsters for which pick is true.
template <class Pick>
std::vector<int> Monsters(const CMonstorUnit &unit, Pick pick) {
    std::vector<int> found;
    for (int monster = 0; monster < kMonsters; monster++) {
        if (pick(monster)) {
            found.push_back(monster);
        }
    }
    return found;
}

// Makes a monster of the kind a miniboss again, taking the key off it when every one of the kind carries it.
void RestoreMiniboss(CMonstorUnit &unit, int kind) {
    auto of_kind = Monsters(unit, [&](int m) { return Eligible(unit, m) && Kind(unit, m) == kind; });
    if (!of_kind.empty()) {
        MakeMiniboss(unit, of_kind[Roll(static_cast<int>(of_kind.size()))]);
        return;
    }
    auto holders = Monsters(unit, [&](int m) { return Plain(unit, m) && Kind(unit, m) == kind; });
    if (holders.empty()) {
        return;
    }
    int  holder = holders[0];
    auto takers = Monsters(unit, [&](int m) { return m != holder && Eligible(unit, m); });
    if (takers.empty()) {
        return;
    }
    int taker = takers[Roll(static_cast<int>(takers.size()))];
    unit.monster[taker].drop_item = unit.monster[holder].drop_item;
    unit.monster[holder].drop_item = -1;
    MakeMiniboss(unit, holder);
}

void Spawned(CMonstorUnit &, int monster) {
    miniboss[monster] = false;
}

[[maybe_unused]] const bool listening = [] {
    AddEnemySpawnListener(&Spawned);
    return true;
}();

} // namespace

bool IsMiniboss(int monster) {
    return monster >= 0 && monster < kMonsters && miniboss[monster];
}

void MakeMiniboss(CMonstorUnit &unit, int monster) {
    if (monster < 0 || monster >= kMonsters || miniboss[monster]) {
        return;
    }
    miniboss[monster] = true;
    ScaleMonster(unit, monster, kMinibossScale);

    MONSTOR &live = unit.monster[monster];
    live.money *= kMinibossMoney;
    live.money_chance = 100;
    live.drops_items = 1;
    live.item_damage_rate = 10;
    live.lockon_scale_x *= kMinibossCursor;
    live.lockon_scale_y *= kMinibossCursor;
    range[monster] = Contains(kLongReach, live.name_no) ? kMinibossRangeWide : kMinibossRange;
    walk_synced[monster] = !Contains(kFlyers, live.name_no);

    CCharacter &body = unit.chara[monster][0];
    body.SetScale(body.scale[0] * kMinibossSize, body.scale[1] * kMinibossSize, body.scale[2] * kMinibossSize);

    MONSTOR_EFFECT_STATE  &hurt = unit.effect[monster];
    MONSTOR_EFFECT_STATE2 &hits = unit.effect2[monster];
    for (int i = 0; i < 16; i++) {
        if (hurt.timer[i] != 0) {
            hurt.radius[i] *= kMinibossSize;
        }
        if (hits.active[i] != 0 && hits.radius[i] > 0.0f && hits.radius[i] < 200.0f) {
            hits.radius[i] *= kMinibossReach;
        }
    }
}

void MinibossFloorEntered() {
    sides[0] = {};
    sides[1] = {};
}

int MinibossPlacedModel(const CMonstorUnit &unit, int placement, int model_slot) {
    const Side &side = CurrentSide(unit);
    if (!side.populated || placement < 0 || placement >= static_cast<int>(side.kinds.size())) {
        return model_slot;
    }
    for (int slot = 0; slot < unit.model_count && slot < 9; slot++) {
        if (ModelKind(slot) == side.kinds[placement]) {
            return slot;
        }
    }
    return model_slot;
}

void MinibossesArranged(CMonstorUnit &unit) {
    if (selectMapNo < 0 || selectMapNo >= DUNGEON_COUNT) {
        return;
    }
    Side &side = CurrentSide(unit);
    if (!side.populated) {
        side.populated = true;
        RollMinibosses(unit);
    } else {
        for (int kind : side.kinds) {
            RestoreMiniboss(unit, kind);
        }
    }
    side.kinds.clear();
    for (int monster = 0; monster < kMonsters; monster++) {
        if (miniboss[monster] && unit.monster[monster].state != -1) {
            side.kinds.push_back(Kind(unit, monster));
        }
    }
}

void MinibossDefeated(CMonstorUnit &unit, int monster) {
    if (!IsMiniboss(monster)) {
        return;
    }
    miniboss[monster] = false;
    std::vector<int> &kinds = CurrentSide(unit).kinds;
    auto              found = std::find(kinds.begin(), kinds.end(), Kind(unit, monster));
    if (found != kinds.end()) {
        kinds.erase(found);
    }
}

float MinibossScriptDistance(int monster, int argument_count, float distance) {
    return argument_count == 1 && IsMiniboss(monster) ? distance / range[monster] : distance;
}

void MinibossMotion(CMonstorUnit &unit, int monster) {
    if (!IsMiniboss(monster) || !walk_synced[monster]) {
        return;
    }
    const MONSTOR &live = unit.monster[monster];
    for (int j = 0; j <= live.attachment_count && j < 3; j++) {
        CCharacter &character = unit.chara[monster][j];
        if ((j > 0 && character.frame == NULL) || character.motion_no != kWalkMotion) {
            continue;
        }
        float speed = live.requested_motion_speed;
        if (!(speed > 0.0f)) {
            if (character.motion_type.motion_info == NULL) {
                continue;
            }
            speed = character.motion_type.motion_info[kWalkMotion].speed;
        }
        character.SetMotionSpeed(speed / kMinibossWalkSlowdown);
    }
}

void SetMinibossRoll(MinibossRoll roll) {
    roll_override = roll;
}

} // namespace expanded
