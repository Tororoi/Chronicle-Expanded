#include "expanded/weapons/goro/midas_touch.hpp"

#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/data/gold_statue.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "randomitem.hpp"
#include "render/draw3d.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kGoroOwner = 2;    // a hit's owner: Goro's
constexpr int kLethalTimes = 10; // the hit's damage once it turns the monster to gold: its life times this,
constexpr int kLethalMore = 100; // and this more
constexpr int kStatues = 16;     // as many as the floor's monsters

struct Statue {
    bool  on = false;
    bool  done = false;   // the game is done with it (its slot let go): drawn by us
    bool  rolled = false; // its gold bar rolled
    int   base_model = -1;
    float at[3] = {};
};

Statue         statues[kStatues];
COLLISION_HIT *turning = nullptr; // the hit made killing, until it has struck
int            turning_damage = 0;
bool           loaded = false; // the gold's pack entered this floor
int            gold = -1;      // its picture's texture handle

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(MidasReset), true);

// The gold picture, its pack entered once a floor.
// The hit made killing given back its own damage.
void PutBack() {
    if (turning != nullptr) {
        turning->damage = turning_damage;
        turning = nullptr;
    }
}

int Gold() {
    if (!loaded) {
        loaded = true;
        if (EnterFloorCharacter(kGoldStatueFile, kGoldStatueCfg) != nullptr) {
            gold = TexManager.GetTextureHandle(const_cast<char *>(kGoldStatuePicture), kFloorCharacterBlock);
        }
    }
    return gold;
}

bool InPlay(const CMonstorUnit &unit, int monster) {
    return &unit == NowMonstorUnit && monster >= 0 && monster < kStatues;
}

} // namespace

bool MidasTouchWielded() {
    return Playing(CHARA_GORO) && NowWeapon == &MainWeapon && WeaponWielded(ITEM_WEAPON_BIG_BUCKS_HAMMER);
}

bool MidasStatue(const CMonstorUnit &unit, int monster) {
    return InPlay(unit, monster) && statues[monster].on;
}

void MidasHitTaken(CMonstorUnit &unit, int monster, COLLISION_HIT &hit) {
    PutBack();
    if (!InPlay(unit, monster) || hit.owner != kGoroOwner || !MidasTouchWielded() || statues[monster].on) {
        return;
    }
    const MONSTOR &live = unit.monster[monster];
    if (live.hp <= 0 || live.kind == MONSTER_KIND_NO_LOCK_ON || ModRoll(100) >= kMidasChance) {
        return;
    }
    Statue &statue = statues[monster];
    statue = Statue{.on = true, .base_model = live.base_model};
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    for (int c = 0; c < 3; c++) {
        statue.at[c] = at[c];
    }
    turning = &hit;
    turning_damage = hit.damage;
    hit.damage = live.hp * kLethalTimes + kLethalMore; // this monster's alone: put back once it has struck (MidasDamaged)
}

void MidasDamaged(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit) {
    if (&hit == turning) {
        PutBack();
    }
    // One it could not kill (its element's no effect on it) is not a statue.
    if (MidasStatue(unit, monster) && unit.monster[monster].hp > 0 && !statues[monster].done) {
        statues[monster] = Statue{};
    }
}

void MidasDrawing(CMonstorUnit &unit, int monster) {
    if (!MidasStatue(unit, monster)) {
        return;
    }
    unit.chara[monster][0].fade[0] = 1.0f; // its fade held
    dc::render::Draw3DSetTextureOverride(Gold());
}

void MidasDrawn(CMonstorUnit &unit, int monster) {
    if (MidasStatue(unit, monster)) {
        dc::render::Draw3DSetTextureOverride(-1);
    }
}

void MidasStep() {
    PutBack();
    if (NowMonstorUnit == nullptr) {
        return;
    }
    CMonstorUnit &unit = *NowMonstorUnit;
    for (int monster = 0; monster < kStatues; monster++) {
        Statue &statue = statues[monster];
        if (!statue.on) {
            continue;
        }
        const MONSTOR &live = unit.monster[monster];
        if (live.state == -1) {
            statue.done = true;
        } else if (statue.done || live.base_model != statue.base_model) {
            statue = Statue{}; // its slot taken by another monster
            continue;
        }
        if (statue.done && !statue.rolled && std::hypot(CharaMain.pos[0] - statue.at[0], CharaMain.pos[2] - statue.at[2]) <= kMidasNear) {
            statue.rolled = true;
            if (ModRoll(100) < kMidasBarChance && RandomItem != nullptr) {
                float drop[4] = {statue.at[0], statue.at[1], statue.at[2], 1.0f};
                RandomItem->Set(drop, 1, -1, ITEM_GOLD_BULLION);
            }
        }
    }
}

void MidasDraw() {
    if (NowMonstorUnit == nullptr || BtEventMode != 0) {
        return;
    }
    CMonstorUnit &unit = *NowMonstorUnit;
    for (int monster = 0; monster < kStatues; monster++) {
        const Statue &statue = statues[monster];
        if (!statue.on || !statue.done || unit.chara[monster][0].frame == nullptr) {
            continue;
        }
        CCharacter &body = unit.chara[monster][0];
        body.fade[0] = 1.0f;
        dc::render::Draw3DSetTextureOverride(Gold());
        body.Draw();
        dc::render::Draw3DSetTextureOverride(-1);
    }
}

MonsterDamageResult MidasCheckDmg(CMonstorUnit &unit, MonsterDamageResult result) {
    int monster = unit.current_monster;
    if (result != MonsterDamageResult::Killed || !MidasStatue(unit, monster)) {
        return result;
    }
    unit.monster[monster].state = -1;
    unit.alive_count--;
    return MonsterDamageResult::None;
}

void MidasLight(const CCharacter &character, float *ambient) {
    if (NowMonstorUnit == nullptr) {
        return;
    }
    for (int monster = 0; monster < kStatues; monster++) {
        if (statues[monster].on && &character == &NowMonstorUnit->chara[monster][0]) {
            for (int c = 0; c < 3; c++) {
                ambient[c] += kMidasShine[c];
            }
            return;
        }
    }
}

void MidasReset() {
    turning = nullptr; // the floor's records gone with it
    for (Statue &statue : statues) {
        statue = Statue{};
    }
    loaded = false;
    gold = -1;
}

} // namespace expanded
