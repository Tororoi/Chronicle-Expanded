#include "expanded/weapons/ungaga/babels_spear.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <numbers>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/ungaga/babels_spear/beam.hpp"
#include "expanded/weapons/ungaga/babels_spear/copy.hpp"
#include "expanded/weapons/weapon_hit.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

using namespace babels_spear;

constexpr int kHitReserve = 16; // free hit records a hit of the copy's leaves the game

// The copy's age as it starts fading: the vanish's frame 60.
constexpr int kFadeFrom = kBabelStandFrames + kBabelVanishFrames - kBabelFadeFrames;

struct Curse {
    int              chara = -1; // whose it is (-1: no wielder)
    const BabelForm *form = nullptr;
    CopyForm         copy = CopyForm::kSpear;
    int              held = -1; // frames in the guard pose this hold, -1 out of it
    bool             latched = false;
    float            weapon_tint = 0.0f; // the weapon's share of the grey
    bool             up = false;
    int              age = 0;
    int              target = -1;
    float            spot[3] = {}; // x, ground, z
    float            yaw = 0.0f;
    float            rise = 0.0f;       // the root's height while it rises
    float            rise_speed = 0.0f; // how far it sinks this frame (below 0: it climbs)
    float            rise_gain = 0.0f;  // and what that gains each frame
    bool             risen = false, struck = false;
    float            alpha = 1.0f;
    bool             confused[kFloorMonsters] = {}; // the monsters it confused
};

Curse curse;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(CurseOfBabelReset), true);

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_BABEL_S_SPEAR, WeaponShotForm::Weapon, BeamEffect(), 1, kBabelBeamFile),
     RegisterWeaponShotEffect(ITEM_WEAPON_BABEL_S_SPEAR, WeaponShotForm::Sphere, BeamEffect(), 1, kBabelBeamFile), true);

const BabelForm &Form() {
    return *curse.form;
}

float Wrap(float a) {
    while (a > std::numbers::pi_v<float>) {
        a -= 2.0f * std::numbers::pi_v<float>;
    }
    while (a < -std::numbers::pi_v<float>) {
        a += 2.0f * std::numbers::pi_v<float>;
    }
    return a;
}

// The root's height t of the way out (0: its tip kBabelBuried under the floor; 1: its exposed part out).
float RootHeight(float t) {
    return curse.spot[1] - Form().tip * Form().scale + (-kBabelBuried + (Form().exposed * Form().scale + kBabelBuried) * t);
}

bool Live(int monster) {
    const MONSTOR *live = FloorMonster(monster);
    return live != nullptr && MonsterLiving(*live);
}

int FreeHits() {
    int free = 0;
    for (int active : NowColData->active) {
        free += active == 0 ? 1 : 0;
    }
    return free;
}

// One hit on a monster's body: share of the weapon's attack, thrown away from from; marked as a catch when it wears nothing.
void Hit(CMonstorUnit &unit, int monster, float share, KickBack kick, const float *from, bool wears, const char *what) {
    if (NowColData == nullptr || NowWeaponHave == nullptr || FreeHits() <= kHitReserve) {
        return;
    }
    float          centre[4];
    float          radius = std::max(kBabelStrikeRadius, MonsterBodyCentre(unit, monster, centre));
    int            damage = std::max(1, static_cast<int>(std::nearbyint(static_cast<double>(NowWeaponHave->attack * share))));
    COLLISION_HIT *record = PlantWeaponHit(centre, damage, 1, radius);
    if (record == nullptr) {
        return;
    }
    if (!wears) {
        record->weapon_flags |= kBabelCatchMark;
    }
    float origin[4] = {from[0], from[1], from[2], 1.0f};
    NowColData->SetKickBack(origin, kick.speed, kick.decay, kKnockbackFromOrigin);
    LogEvent("babel", std::format("\"step\":\"{}\",\"monster\":{},\"damage\":{}", what, monster, damage));
}

// The monster it rises under and its spot: the one locked on to, else the nearest living one within reach (at its root),
// else -1 and a spot ahead of the wielder at his height.
int Target(float *spot) {
    int target = LockedTarget();
    if (target < 0 && NowMonstorUnit != nullptr) {
        target = NearestMonster(*NowMonstorUnit, CharaMain.pos, kBabelNoLockReach);
    }
    if (target >= 0) {
        float at[4];
        NowMonstorUnit->chara[target][0].GetPosition(at);
        std::copy(at, at + 3, spot);
        return target;
    }
    float yaw = PlayerYaw();
    spot[0] = CharaMain.pos[0] + std::sin(yaw) * kBabelAhead;
    spot[1] = CharaMain.pos[1];
    spot[2] = CharaMain.pos[2] + std::cos(yaw) * kBabelAhead;
    return -1;
}

// Whether a monster can be confused: alive and in play (out of any chest), not a boss.
bool Confusable(int monster) {
    const MONSTOR *live = FloorMonster(monster);
    return live != nullptr && MonsterLiving(*live) && live->state == 2 && live->revealed != 0 && live->kind != MONSTER_KIND_NO_LOCK_ON;
}

// Confused for frames, until the copy has gone; one already confused keeps its own confusion.
void Confuse(int monster, int frames) {
    if (Confusable(monster) && ConfuseMonster(*NowMonstorUnit, monster, frames)) {
        curse.confused[monster] = true;
    }
}

// Every monster that can be and is not yet confused.
void ConfuseAll(int frames) {
    if (NowMonstorUnit == nullptr) {
        return;
    }
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!MonsterConfused(*NowMonstorUnit, monster)) {
            Confuse(monster, frames);
        }
    }
}

void TakeDown() {
    if (!curse.up) {
        return;
    }
    BeamStop();
    SolidColumnDisarm(&curse);
    CopyDown();
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (curse.confused[monster]) {
            ConfuseEnd(monster); // its confusion goes with it
            curse.confused[monster] = false;
        }
    }
    ConfuseStarsFade(1.0f);
    curse.weapon_tint = 0.0f;
    curse.up = false;
    LogEvent("babel", "\"step\":\"gone\"");
}

void Summon() {
    TakeDown();
    curse.target = Target(curse.spot);
    curse.yaw = PlayerYaw();
    curse.risen = curse.struck = false;
    curse.alpha = 1.0f;
    if (!CopySpawn(curse.copy)) {
        return;
    }
    curse.up = true;
    curse.age = -1; // its first step, this frame, is its frame 0
    // The rise: up at 2D/T, slowing by 1/T of that a frame, so it settles as it is out.
    float from = RootHeight(0.0f), to = RootHeight(1.0f);
    float speed = 2.0f * (to - from) / kBabelEmergeFrames;
    curse.rise = from;
    curse.rise_speed = -speed;
    curse.rise_gain = speed / kBabelEmergeFrames;
    float root[3] = {curse.spot[0], from, curse.spot[2]};
    CopyPlace(root, curse.yaw, Form().scale, kBabelTint, 1.0f);
    LogEvent("babel", std::format("\"step\":\"summoned\",\"target\":{},\"x\":{:.1f},\"z\":{:.1f}", curse.target, curse.spot[0], curse.spot[2]));
    if (curse.target >= 0) {
        Confuse(curse.target, kBabelLifeFrames);
    }
    ConfuseAll(kBabelLifeFrames);
    BeamStart(curse.spot, Form().beam_scale);
}

// While it rises and has not struck, it keeps under its monster (its spot across the floor, the beam's with it); gone or
// dead, it stays where it is.
void Follow() {
    if (curse.target < 0 || !Live(curse.target)) {
        return;
    }
    float at[4];
    NowMonstorUnit->chara[curse.target][0].GetPosition(at);
    curse.spot[0] = at[0];
    curse.spot[2] = at[2];
}

void TipStrike() {
    if (curse.target < 0) {
        curse.struck = true;
        return;
    }
    if (!Live(curse.target)) {
        curse.struck = true;
        std::fprintf(stderr, "babel's spear: monster %d went before the tip reached it\n", curse.target);
        return;
    }
    CMonstorUnit &unit = *NowMonstorUnit;
    float         tip = (curse.risen ? RootHeight(1.0f) : curse.rise) + Form().tip * Form().scale;
    float         lowest = MonsterBodyUnderside(unit, curse.target);
    if (tip < lowest || MonsterBodyEdge(unit, curse.target, curse.spot[0], curse.spot[2]) > kBabelStrikeRadius) {
        if (curse.risen) {
            curse.struck = true; // it stopped under it, or the monster left it
            std::fprintf(stderr, "babel's spear: the tip missed monster %d\n", curse.target);
        }
        return;
    }
    curse.struck = true;
    Hit(unit, curse.target, 1.0f, kHeavyKickBack, CharaMain.pos, true, "struck");
}

// Every catch step of its turn, each living monster in its reach takes a catch.
void Catches() {
    int turned = curse.age - kBabelEmergeFrames;
    if (turned < 0 || turned % Form().catch_frames != 0 || NowMonstorUnit == nullptr) {
        return;
    }
    CMonstorUnit &unit = *NowMonstorUnit;
    float         hands[2][3];
    if (Form().hands) {
        for (int h = 0; h < 2; h++) {
            if (!CopyForkPoint(kBabelHands[h], hands[h])) {
                return; // no statue to catch with
            }
        }
    }
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!Live(monster)) {
            continue;
        }
        bool caught = Form().hands ? std::min(MonsterBodyDistance(unit, monster, hands[0]), MonsterBodyDistance(unit, monster, hands[1])) <= kBabelHandReach
                                   : MonsterBodyEdge(unit, monster, curse.spot[0], curse.spot[2]) <= Form().block_radius + kBabelSpikeReach;
        if (caught) {
            Hit(unit, monster, Form().catch_share, kBabelSpikeKick, curse.spot, false, Form().hands ? "hand" : "spike");
        }
    }
}

void Drive() {
    curse.age++;
    if (curse.age >= kBabelLifeFrames) {
        TakeDown();
        return;
    }
    BeamStep(curse.spot, curse.age >= kBabelStandFrames);
    if (!curse.risen && curse.age > 0) {
        curse.rise -= curse.rise_speed;
        curse.rise_speed += curse.rise_gain;
    }
    if (!curse.struck && !curse.risen) {
        Follow();
    }
    if (!curse.struck) {
        TipStrike();
    }
    if (curse.risen) {
        curse.yaw = Wrap(curse.yaw + kBabelSpin);
    } else if (curse.age >= kBabelEmergeFrames) {
        // Risen: its exact height, solid below the tip, turning from the next frame.
        curse.risen = true;
        curse.rise = RootHeight(1.0f);
        SolidColumnArm(curse.spot, Form().block_radius, curse.spot[1] + Form().exposed * Form().scale, &curse);
        LogEvent("babel", "\"step\":\"risen\"");
    }
    curse.alpha = curse.age < kFadeFrom ? 1.0f : std::max(0.0f, 1.0f - static_cast<float>(curse.age - kFadeFrom) / kBabelFadeFrames);
    float root[3] = {curse.spot[0], curse.rise, curse.spot[2]};
    CopyPlace(root, curse.yaw, Form().scale, kBabelTint, curse.alpha);
    if (curse.risen) {
        Catches();
    }
    ConfuseAll(kBabelLifeFrames - curse.age);
    curse.weapon_tint = curse.alpha;
    ConfuseStarsFade(curse.alpha);
}

bool ControllerGuard() {
    return GamePad.On(PAD_R1) != 0;
}

BabelGuardButton guard_button = ControllerGuard;

// A charge dropped: its clock cleared and the weapon's grey off (the copy's, while one stands).
void ChargeOff() {
    if (curse.held >= 0 && !curse.up) {
        curse.weapon_tint = 0.0f;
    }
    curse.held = -1;
}

// The guard pose held long enough raises one; out of the pose the clock starts again; R1 let go, the next hold may.
void Charge() {
    if (!guard_button()) {
        curse.latched = false;
        ChargeOff();
        return;
    }
    if (curse.up || curse.latched) {
        curse.held = -1; // one stands, or this hold raised it
        return;
    }
    if (!Guarding()) {
        ChargeOff();
        return;
    }
    curse.held++;
    curse.weapon_tint = std::min(1.0f, static_cast<float>(curse.held) / kBabelGuardFrames);
    if (curse.held < kBabelGuardFrames) {
        return;
    }
    curse.latched = true;
    ChargeCompleteFlash();
    Summon();
}

void End() {
    TakeDown();
    ConfuseEndAll();
    ConfuseStarsFade(1.0f);
    curse = Curse{};
}

bool Active() {
    return NowMonstorUnit != nullptr && (Playing(CHARA_UNGAGA) || Playing(CHARA_XIAO)) && WeaponActive(ITEM_WEAPON_BABEL_S_SPEAR);
}

} // namespace

bool CurseOfBabelUp() {
    return curse.up;
}

int CurseOfBabelAge() {
    return curse.up ? curse.age : -1;
}

int CurseOfBabelTarget() {
    return curse.up ? curse.target : -1;
}

bool CurseOfBabelRisen() {
    return curse.up && curse.risen;
}

void CurseOfBabelSpot(float *spot) {
    std::copy(curse.spot, curse.spot + 3, spot);
}

float CurseOfBabelYaw() {
    return curse.yaw;
}

float CurseOfBabelAlpha() {
    return curse.up ? curse.alpha : 0.0f;
}

float CurseOfBabelWeaponTint() {
    return curse.weapon_tint;
}

bool CurseOfBabelCatch(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kBabelCatchMark) != 0;
}

void CurseOfBabelStep() {
    bool active = Active();
    if (curse.chara >= 0 && (!active || UserStatus->cur_chara != curse.chara)) {
        End(); // a change of wielder ends it, and every confusion
    }
    if (!active || !MonstersStepping()) {
        return;
    }
    if (curse.chara < 0) {
        curse.chara = UserStatus->cur_chara;
        curse.form = curse.chara == CHARA_XIAO ? &kBabelStatue : &kBabelSpear;
        curse.copy = curse.chara == CHARA_XIAO ? CopyForm::kStatue : CopyForm::kSpear;
    }
    Charge();
    if (curse.up) {
        Drive();
    }
}

void CurseOfBabelDraw() {
    if (curse.up && BtEventMode == 0) {
        CopyDraw();
    }
}

void CurseOfBabelLight(const CCharacter &character, float *ambient) {
    if (curse.chara < 0) {
        return;
    }
    if (&character == NowWeapon) {
        for (int c = 0; c < 3; c++) {
            ambient[c] += kBabelTint * curse.weapon_tint;
        }
        return;
    }
    if (!curse.up || NowMonstorUnit == nullptr) {
        return;
    }
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (&character == &NowMonstorUnit->chara[monster][0]) {
            if (MonsterConfused(*NowMonstorUnit, monster)) {
                for (int c = 0; c < 3; c++) {
                    ambient[c] += kBabelConfusedTint[c] * curse.alpha;
                }
            }
            return;
        }
    }
}

void SetCurseOfBabelGuardButton(BabelGuardButton held) {
    guard_button = held != nullptr ? held : ControllerGuard;
}

void CurseOfBabelReset() {
    BeamStop();
    SolidColumnDisarm(&curse);
    CopyReset();
    ConfuseStarsFade(1.0f);
    curse = Curse{};
}

} // namespace expanded
