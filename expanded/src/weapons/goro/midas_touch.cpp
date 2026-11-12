#include "expanded/weapons/goro/midas_touch.hpp"

#include <algorithm>
#include <cmath>

#include "battlemenu.hpp"
#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/data/gold_statue.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/reflection_cubes.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/goro/midas_curse.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "hitmark.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "randomitem.hpp"
#include "render/draw3d.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int   kGoroOwner = 2;      // a hit's owner: Goro's
constexpr int   kLethalTimes = 10;   // the hit's damage once it turns the monster to gold: its life times this,
constexpr int   kLethalMore = 100;   // and this more
constexpr int   kStatues = 16;       // as many as the floor's monsters
constexpr int   kParts = 3;          // a monster's body and its attachments (CMonstorChara)
constexpr float kPlayerBlock = 6.0f; // how far beyond a monster's sphere the player is stopped (MoveCheck)

struct Statue {
    int   clinked = 0; // frames until Goro's hit on it clinks again
    bool  on = false;
    bool  done = false; // the game is done with it (its slot let go): drawn by us
    int   flash = 0;    // frames of its turning flash left
    int   base_model = -1;
    float at[3] = {};
    int   motion[kParts] = {}; // each part's motion as it was struck: its pose is that motion's, at the time it stopped
};

Statue statues[kStatues];
Statue candidate;         // the monster a hit of Goro's is striking, as it stood: a statue if the hit kills it
int    candidate_of = -1; // whose (-1: none)

// The player thrown back off a statue: the way (x, z) and the speed left, added to his step (MidasPlayerMove).
struct Rebound {
    float x = 0.0f;
    float z = 0.0f;
    float speed = 0.0f;
};

Rebound        rebound;
COLLISION_HIT *turning = nullptr; // the hit made killing, until it has struck
int            turning_damage = 0;
bool           loaded = false; // the gold's pack entered this floor
int            gold = -1;      // its picture's texture handle

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(MidasReset), RegisterOwnSound(kMidasClink), RegisterOwnSound(kMidasTurnSound), true);

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

// One part's step as the game steps a monster its stop timer holds (CMonstorUnit::DrawMonstor): the motion it was
// struck in (not a hurt or death motion asked for since) set held (SetMotion mode 1), so the step poses the model where
// that motion stopped and no time passes. The model's frames are its kind's, posed by every monster of that kind.
void Hold(CCharacter &character, int motion) {
    character.SetMotion(motion, 1);
    character.Step();
}

// Every part held, for a statue the game no longer steps.
void HoldAll(CMonstorUnit &unit, int monster) {
    for (int part = 0; part < kParts; part++) {
        if (unit.chara[monster][part].frame != nullptr) {
            Hold(unit.chara[monster][part], statues[monster].motion[part]);
        }
    }
}

bool InPlay(const CMonstorUnit &unit, int monster) {
    return &unit == NowMonstorUnit && monster >= 0 && monster < kStatues;
}

// Each sphere a monster stops the player with (CMonstorUnit::MoveCheck): the ones its script set, else its body about
// where it stands. f(x, z, radius).
template <typename F>
void EachBlock(const CMonstorUnit &unit, int monster, F f) {
    const MONSTOR_EFFECT_STATE3 &blocks = unit.effect3[monster];
    if (blocks.count == 0) {
        const float *at = unit.chara[monster][0].pos;
        f(at[0], at[2], unit.monster[monster].body_radius);
        return;
    }
    for (int i = 0; i < blocks.count && i < 12; i++) {
        f(blocks.position[i][0], blocks.position[i][2], blocks.radius[i]);
    }
}

void Clink() {
    PlayOwnSound(kMidasClink, false);
}

// Goro's hits reaching a statue's spheres (the ones its hits were taken on, still where it stood): each clinks off it as
// off a guard, a guard's spark where it struck, and throws him back.
void StatueHits(CMonstorUnit &unit, int monster, Statue &statue) {
    if (statue.clinked > 0) {
        statue.clinked--;
        return;
    }
    if (NowColData == nullptr) {
        return;
    }
    MONSTOR_EFFECT_STATE &spheres = unit.effect[monster];
    for (int i = 0; i < 16; i++) {
        if (spheres.timer[i] == 0) {
            continue;
        }
        int hit = NowColData->FindMonsterHit(spheres.position[i], spheres.radius[i]);
        if (hit == -1 || NowColData->GetMonsterOwner(hit) != -1) {
            continue;
        }
        statue.clinked = kMidasClinkFrames;
        Clink();
        float up[4] = {0.0f, 2.5f, 0.0f, 1.0f};
        HitMark[hitCnt].Set(spheres.position[i], up, HIT_MARK_GUARD, 0.8f, 0.005f, 0.02f, 1.3f, 32, unit.monster[monster].ground_y);
        HitPointMark[hitCnt].Set(spheres.position[i]);
        hitCnt = hitCnt == 15 ? 0 : hitCnt + 1;
        // A melee swing (Toan's, Goro's, Ungaga's weapon, as the guard's wear counts them) throws its wielder back.
        const COLLISION_HIT &record = NowColData->hit[hit];
        bool                 melee = record.attack_no == 0 && (record.owner == 0 || record.owner == 2 || record.owner == 4);
        float                away[2] = {CharaMain.pos[0] - statue.at[0], CharaMain.pos[2] - statue.at[2]};
        float                apart = std::hypot(away[0], away[1]);
        if (melee && apart > 0.0f) {
            rebound = {away[0] / apart, away[1] / apart, kMidasRebound};
        }
        return;
    }
}

} // namespace

void MidasPolish(const float *at) {
    static dc::render::Draw3DReflectionCube cube;
    const dc::render::Draw3DReflectionCube *tile = ReflectionCubeAt(at[0], at[2]);
    if (tile != nullptr) {
        cube = *tile;
        cube.tint = kMidasGoldTint;
        cube.lift = kMidasGoldLift;
        dc::render::Draw3DSetReflectionCube(&cube);
        return;
    }
    MidasPolishBall();
}

void MidasPolishBall() {
    dc::render::Draw3DSetTextureOverride(Gold());
    dc::render::Draw3DSetReflecting(true);
}

void MidasUnpolish() {
    dc::render::Draw3DSetReflectionCube(nullptr);
    dc::render::Draw3DSetTextureOverride(-1);
    dc::render::Draw3DSetReflecting(false);
}

bool MidasTouchWielded() {
    return Playing(CHARA_GORO) && NowWeapon == &MainWeapon && WeaponWielded(ITEM_WEAPON_BIG_BUCKS_HAMMER) && MidasCursed();
}

bool MidasGuardSound() {
    if (!(Playing(CHARA_GORO) && GoroGold())) {
        return false;
    }
    Clink();
    return true;
}

bool MidasStatue(const CMonstorUnit &unit, int monster) {
    return InPlay(unit, monster) && statues[monster].on;
}

bool MidasHold(CMonstorUnit &unit, int monster, CCharacter &character) {
    if (!MidasStatue(unit, monster)) {
        return false;
    }
    for (int part = 0; part < kParts; part++) {
        if (&character == &unit.chara[monster][part]) {
            Hold(character, statues[monster].motion[part]);
        }
    }
    return true;
}

void MidasHitTaken(CMonstorUnit &unit, int monster, COLLISION_HIT &hit) {
    PutBack();
    if (!InPlay(unit, monster) || hit.owner != kGoroOwner || !MidasTouchWielded() || statues[monster].on) {
        return;
    }
    const MONSTOR &live = unit.monster[monster];
    if (live.hp <= 0 || live.kind == MONSTER_KIND_NO_LOCK_ON) {
        return;
    }
    candidate = Statue{.on = true, .base_model = live.base_model};
    for (int part = 0; part < kParts; part++) {
        candidate.motion[part] = unit.chara[monster][part].motion_no;
    }
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    for (int c = 0; c < 3; c++) {
        candidate.at[c] = at[c];
    }
    candidate_of = monster;
    if (live.hp * 100 >= live.max_hp * kMidasLowLife) {
        return; // a statue only if this hit kills it (MidasCheckDmg)
    }
    statues[monster] = candidate;
    MidasTurnEffect(statues[monster].flash);
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
    MidasPolish(statues[monster].at);
}

void MidasDrawn(CMonstorUnit &unit, int monster) {
    if (MidasStatue(unit, monster)) {
        MidasUnpolish();
    }
}

void MidasStep() {
    PutBack();
    candidate_of = -1;
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
            if (!statue.done) {
                statue.clinked = kMidasClinkFrames; // the hit that turned it is not one on it
            }
            statue.done = true;
        } else if (statue.done || live.base_model != statue.base_model) {
            statue = Statue{}; // its slot taken by another monster
            continue;
        }
        if (statue.flash > 0) {
            statue.flash--;
        }
        if (statue.done) {
            StatueHits(unit, monster, statue);
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
        HoldAll(unit, monster);
        MidasPolish(statue.at);
        body.Draw();
        MidasUnpolish();
    }
}

void MidasPlayerMove(float *velocity) {
    if (rebound.speed <= 0.0f) {
        return;
    }
    velocity[0] += rebound.x * rebound.speed;
    velocity[2] += rebound.z * rebound.speed;
    rebound.speed = std::max(rebound.speed - kMidasReboundDecay, 0.0f);
}

int MidasDropItem(const CMonstorUnit &unit, int monster, int drop_item) {
    // An item the floor gave it beforehand (a key) stays its drop.
    if (drop_item != -1 || !InPlay(unit, monster) || !statues[monster].on || ModRoll(100) >= kMidasBarChance) {
        return drop_item;
    }
    return ITEM_GOLD_BULLION;
}

void MidasTurnEffect(int &flash) {
    PlayOwnSound(kMidasTurnSound, false);
    flash = kMidasFlashFrames;
}

void MidasFlashLight(int frames, float *ambient) {
    float rate = static_cast<float>(frames) / static_cast<float>(kMidasFlashFrames);
    for (int c = 0; c < 3; c++) {
        ambient[c] += kMidasFlash[c] * rate;
    }
}

bool MidasSolid(const CMonstorUnit &unit, int monster) {
    return MidasStatue(unit, monster) && unit.monster[monster].state == -1;
}

void MidasPullDrop(float *drop) {
    if (NowMonstorUnit == nullptr) {
        return;
    }
    const float *player = CharaMain.pos;
    for (int monster = 0; monster < kStatues; monster++) {
        if (!MidasSolid(*NowMonstorUnit, monster)) {
            continue;
        }
        bool pull = false;
        EachBlock(*NowMonstorUnit, monster, [&](float cx, float cz, float radius) {
            float stop = kPlayerBlock + radius; // where the sphere stops the player
            if (std::hypot(drop[0] - cx, drop[2] - cz) < stop && std::hypot(player[0] - cx, player[2] - cz) <= stop + kMidasTouch) {
                pull = true; // inside the sphere he is against
            }
        });
        if (pull) {
            float dx = player[0] - drop[0];
            float dz = player[2] - drop[2];
            float apart = std::hypot(dx, dz);
            float step = std::min(kMidasPull, apart);
            if (apart > 0.0f) {
                drop[0] += dx / apart * step;
                drop[2] += dz / apart * step;
            }
            return;
        }
    }
}

MonsterDamageResult MidasCheckDmg(CMonstorUnit &unit, MonsterDamageResult result) {
    int monster = unit.current_monster;
    // Goro's hit that kills it turns it, as it stood when struck.
    if (result == MonsterDamageResult::Killed && monster == candidate_of && InPlay(unit, monster) && !statues[monster].on) {
        statues[monster] = candidate;
        MidasTurnEffect(statues[monster].flash);
    }
    if (monster == candidate_of) {
        candidate_of = -1;
    }
    if (result != MonsterDamageResult::Killed || !MidasStatue(unit, monster)) {
        return result;
    }
    unit.monster[monster].state = -1;
    unit.alive_count--;
    return MonsterDamageResult::None;
}

void MidasLight(const CCharacter &character, float *ambient) {
    // Gold Goro shines as a statue does: his model in play, and the item menu's of him.
    GoroGoldLight(character, ambient);
    if (GoroGold() && ((Playing(CHARA_GORO) && &character == &CharaMain) || &character == &MenuCharaFrame)) {
        for (int c = 0; c < 3; c++) {
            ambient[c] += kMidasShine[c];
        }
        return;
    }
    if (NowMonstorUnit == nullptr) {
        return;
    }
    for (int monster = 0; monster < kStatues; monster++) {
        if (statues[monster].on && &character == &NowMonstorUnit->chara[monster][0]) {
            for (int c = 0; c < 3; c++) {
                ambient[c] += kMidasShine[c];
            }
            MidasFlashLight(statues[monster].flash, ambient);
            return;
        }
    }
}

void MidasReset() {
    turning = nullptr; // the floor's records gone with it
    rebound = {};
    candidate_of = -1;
    for (Statue &statue : statues) {
        statue = Statue{};
    }
    loaded = false;
    gold = -1;
}

} // namespace expanded
