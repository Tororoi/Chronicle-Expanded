#include "expanded/weapons/ruby/thorn_armlet.hpp"
#include "sound_effect.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <numbers>
#include <string>

#include "btactstatus.hpp"
#include "character.hpp"
#include "charaheight.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/confuse_state.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/weapon_hit.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kHitReserve = 16; // free hit records a turned hit leaves the game
constexpr int kNeverSeen = -1000;

struct Armlet {
    CCharacter *model = nullptr;  // the copy, loaded on this floor
    bool        failed = false;   // it could not be: not tried again until the next floor
    bool        standing = false; // raised on a full gauge, until it fades or breaks
    bool        breaking = false; // its last hit taken: clearing away
    bool        cooling = false;  // broken: the gauge has to fill before it stands again
    float       refilled = 0.0f;  // while broken, the gauge its own refill has given back (a hit's refill does not count)
    int         hits = 0;         // the hits it turns before it breaks
    float       alpha = 0.0f;
    float       turn = 0.0f;
};

Armlet armlet;
int    tick = 0;                  // frames stepped this floor
int    seen[kFloorMonsters] = {}; // the frame each monster's melee hit last found the wielder

void Forget() {
    std::fill(std::begin(seen), std::end(seen), kNeverSeen);
}

[[maybe_unused]] const bool kListening = (Forget(), AddEnemyFloorListener(ThornArmletReset), true);

// The ring stood upright: its axis, the model's x, turned a quarter about z onto the root's -y (the model upside
// down, as the user chose). The draw keeps a rigid model's root rotation under the character's turn, so the turn about the
// wielder spins the ring about its axis.
void StandUp(CFrame &root) {
    float row0[3], row1[3];
    std::copy(root.local[0], root.local[0] + 3, row0);
    std::copy(root.local[1], root.local[1] + 3, row1);
    for (int c = 0; c < 3; c++) {
        root.local[0][c] = -row1[c];
        root.local[1][c] = row0[c];
    }
    root.world_valid = false;
}

// The wielder's waist: halfway between her hip and spine joints as posed, else kThornHeightShare of her height above
// her feet.
void Waist(float *waist) {
    CFrame *hip = FrameNamed(CharaMain.frame, kThornWaistJoints[0]);
    CFrame *spine = FrameNamed(CharaMain.frame, kThornWaistJoints[1]);
    if (hip != nullptr && spine != nullptr) {
        float a[4], b[4];
        FrameOrigin(*hip, a);
        FrameOrigin(*spine, b);
        for (int c = 0; c < 3; c++) {
            waist[c] = 0.5f * (a[c] + b[c]);
        }
        return;
    }
    waist[0] = CharaMain.pos[0];
    waist[1] = CharaMain.pos[1] + kThornHeightShare * CharaHeight(UserStatus);
    waist[2] = CharaMain.pos[2];
}

bool Load() {
    if (armlet.model != nullptr) {
        return true;
    }
    if (armlet.failed) {
        return false;
    }
    std::string cfg;
    std::string path = WeaponPackPath(ITEM_WEAPON_THORN_ARMLET, cfg);
    CCharacter *model = EnterFloorCharacter(path.c_str(), cfg.c_str());
    if (model == nullptr || model->frame == nullptr) {
        std::fprintf(stderr, "thorn guard: %s could not be loaded\n", path.c_str());
        armlet.failed = true;
        return false;
    }
    StandUp(*model->frame);
    armlet.model = model;
    return true;
}

int FreeHits() {
    int free = 0;
    for (int active : NowColData->active) {
        free += active == 0 ? 1 : 0;
    }
    return free;
}

// The living monster whose own attack planted hit (its melee: a shot's hit names its monster_no), or -1.
int MeleeMonster(const COLLISION_HIT &hit) {
    int monster = hit.monster_no == -1 ? confuse::Attacker(hit) : -1;
    return monster >= 0 && MonsterLiving(NowMonstorUnit->monster[monster]) ? monster : -1;
}

// The monster's hit turned on it: a hit of the wielder's on its body at that damage, thrown away from her.
void TurnOn(CMonstorUnit &unit, int monster, int damage) {
    if (NowColData == nullptr || NowWeaponHave == nullptr || FreeHits() <= kHitReserve) {
        return;
    }
    float          centre[4];
    float          radius = std::max(kThornHitRadius, MonsterBodyCentre(unit, monster, centre));
    COLLISION_HIT *record = PlantWeaponHit(centre, std::max(1, damage), 1, radius);
    if (record == nullptr) {
        return;
    }
    record->weapon_flags |= kThornMark;
    float origin[4] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2], 1.0f};
    NowColData->SetKickBack(origin, kThornKick.speed, kThornKick.decay, kKnockbackFromOrigin);
    LogEvent("thorns", std::format("\"monster\":{},\"damage\":{}", monster, damage));
}

// The guard on: its action from the raise (the game's own test for a guarded blow), not only once it holds.
bool GuardOn() {
    return BtActStatus.action_on == kGuardAction;
}

// The gauge as the standing armlet's health: kThornHits'ths of it.
float Health() {
    return kFullGauge * static_cast<float>(armlet.hits) / static_cast<float>(kThornHits);
}

// Raised with the guard, on a full gauge (and not while broken): its health the whole gauge.
bool Raise() {
    if (armlet.standing) {
        return true;
    }
    if (armlet.cooling || armlet.breaking || BtActStatus.action_gauge < kThornFullGauge || !Load()) {
        return false;
    }
    armlet.standing = true;
    armlet.hits = kThornHits;
    BtActStatus.action_gauge = kFullGauge;
    return true;
}

// A hit turned: its share off the gauge, and the last breaks it.
void Struck() {
    armlet.hits = std::max(0, armlet.hits - 1);
    BtActStatus.action_gauge = Health();
    if (armlet.hits == 0) {
        armlet.breaking = true;
        armlet.cooling = true;
        armlet.refilled = 0.0f;
        SndSePlay(SE_WEAPON_BREAK, -1, 0);
        LogEvent("thorns", "\"step\":\"broken\"");
    }
}

} // namespace

bool ThornGuardActive() {
    return WeaponActive(ITEM_WEAPON_THORN_ARMLET);
}

bool ThornArmletUp() {
    return armlet.alpha > 0.0f;
}

bool ThornArmletStanding() {
    return armlet.standing && !armlet.breaking;
}

int ThornArmletHits() {
    return armlet.standing && !armlet.breaking ? armlet.hits : 0;
}

bool ThornArmletCooling() {
    return armlet.cooling;
}

float ThornArmletAlpha() {
    return armlet.alpha;
}

float ThornArmletTurn() {
    return armlet.turn;
}

float ThornGuardGaugeGain(float gain) {
    if (!ThornGuardActive()) {
        return gain;
    }
    if (armlet.standing) {
        return 0.0f; // held as the armlet's health
    }
    if (armlet.cooling) {
        if (GuardOn()) {
            return 0.0f; // broken: it refills once she lets go
        }
        armlet.refilled = std::min(kFullGauge, armlet.refilled + gain);
        if (armlet.refilled >= kThornFullGauge) {
            armlet.cooling = false;
        }
    }
    return gain;
}

bool ThornGuardHit(const COLLISION_HIT &hit) {
    if (!ThornGuardActive() || !GuardOn() || NowMonstorUnit == nullptr) {
        return false;
    }
    int monster = MeleeMonster(hit);
    if (monster < 0) {
        return false;
    }
    // A swing already turned passes her by to its end, the armlet broken by it or not.
    if (tick - seen[monster] <= kThornRearmFrames) {
        seen[monster] = tick;
        return true;
    }
    if (!Raise() || armlet.breaking) {
        return false;
    }
    seen[monster] = tick;
    TurnOn(*NowMonstorUnit, monster, hit.damage);
    Struck();
    return true;
}

bool ThornHit(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kThornMark) != 0;
}

void ThornArmletStep() {
    tick++;
    bool guard = ThornGuardActive() && GuardOn();
    // The standing armlet's health is the gauge (a hit on her fills it: put back); broken, it refills, then is free.
    if (armlet.standing && !armlet.breaking && std::fabs(BtActStatus.action_gauge - Health()) > 0.5f) {
        BtActStatus.action_gauge = Health();
    }
    // Broken, the gauge is what its own refill has given back: a hit on her (which fills it) does not count.
    if (armlet.cooling && std::fabs(BtActStatus.action_gauge - armlet.refilled) > 0.5f) {
        BtActStatus.action_gauge = armlet.refilled;
    }
    if (armlet.breaking) {
        armlet.alpha = std::max(0.0f, armlet.alpha - 1.0f / static_cast<float>(kThornBreakFrames));
    } else if (guard && Raise()) {
        armlet.alpha = std::min(1.0f, armlet.alpha + 1.0f / static_cast<float>(kThornFadeFrames));
    } else {
        armlet.alpha = std::max(0.0f, armlet.alpha - 1.0f / static_cast<float>(kThornFadeFrames));
    }
    if (armlet.alpha <= 0.0f && (armlet.breaking || !guard)) {
        armlet.standing = false;
        armlet.breaking = false;
    }
    if (armlet.alpha > 0.0f) {
        armlet.turn = std::fmod(armlet.turn + 2.0f * std::numbers::pi_v<float> / static_cast<float>(kThornTurnFrames),
                                2.0f * std::numbers::pi_v<float>);
    }
}

void ThornArmletDraw() {
    if (armlet.alpha <= 0.0f || armlet.model == nullptr || armlet.model->frame == nullptr || UserStatus == nullptr) {
        return;
    }
    CCharacter &model = *armlet.model;
    float       waist[3];
    Waist(waist);
    // The ring's middle kThornLift above her waist: its axis runs down, so its middle along it is that far below the root.
    model.SetPosition(waist[0], waist[1] + kThornLift + kArmletAxisMiddle * kThornScale, waist[2]);
    model.SetRotation(0.0f, armlet.turn, 0.0f);
    model.SetScale(kThornScale, kThornScale, kThornScale);
    model.ambient_offset[3] = 128.0f * armlet.alpha;
    DrawFloorCharacter(model);
}

CCharacter *ThornArmletCharacter() {
    return armlet.model;
}

void ThornArmletReset() {
    armlet = Armlet{};
    tick = 0;
    Forget();
}

} // namespace expanded
