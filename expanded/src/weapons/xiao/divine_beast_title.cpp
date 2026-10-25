#include "expanded/weapons/xiao/divine_beast_title.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "character.hpp"
#include "collision.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "dungeonparts.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "motionmodel.hpp"
#include "shot_effect.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

extern s32 lockOnTargetNo;

namespace expanded {

namespace {

constexpr const char *kCatFile = "gedit/s86/chara/c04cat.chr";
constexpr const char *kCatCfg = "info.cfg";
// The town cat's keys (KEY_START 0).
constexpr int   kKeyStand = 0, kKeySit = 1, kKeyReady = 3, kKeyRun = 4, kKeyTakeOff = 5, kKeyLeap = 6, kKeyLand = 7;
constexpr int   kHoldLast = 2; // SetMotion: stop on the last frame
constexpr int   kBlend = 6;
constexpr int   kLandFrames = 27;
constexpr int   kReadyFrames = 25;
constexpr float kReadyRate = 0.4f;  // the crouch, 95..105, played this fast
constexpr int   kMark = 0x08000000; // in a hit's weapon flags: the cat's
constexpr int   kHitLife = 1;       // one frame, as a pellet's: it lands once
constexpr int   kHitKind = 3;       // a blow
constexpr int   kMonsters = 2;
constexpr float kBody = 4.0f; // the cat's middle over its feet
constexpr float kClearance = 4.0f;
constexpr float kProbeUp = 8.0f, kProbeDown = 40.0f;
constexpr float kGlowSize = 22.0f, kGlowPull = 5.0f;
constexpr float kTint[3] = {12.0f, 24.0f, 48.0f};
constexpr int   kRetarget = 15;

struct Cat {
    SpiritBeastPhase phase = SpiritBeastPhase::kNone;
    float            pos[4] = {0, 0, 0, 1};
    float            vel[3] = {};
    float            yaw = 0.0f;
    float            ground = 0.0f;
    int              age = 0;       // frames since it was let go
    int              phase_age = 0; // frames in this phase
    int              fade = 0;      // frames into the fade
    int              target = -1;
    bool             failed = false; // the model would not load on this floor
};

Cat         cat;
ShotCharge  charge{kSpiritBeastFrames};
CCharacter *model = nullptr;

void OnFloor() {
    SpiritBeastReset();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

void Phase(SpiritBeastPhase phase) {
    cat.phase = phase;
    cat.phase_age = 0;
}

void Motion(int key, int flags = 0, float speed = -1.0f) {
    if (model != nullptr && model->motion_no != key) {
        model->SetMotion(key, flags, speed);
    }
}

// The floor under (x, z) near height y: false where there is none within reach.
bool FloorAt(float x, float y, float z, float *ground) {
    if (NowDngMap == nullptr) {
        *ground = CharaMain.pos[1];
        return true;
    }
    static std::vector<CCPoly> polys(8192);
    float                      centre[4] = {x, y, z, 1.0f};
    int                        count = setCollisionData(NowDngMap, polys.data(), centre, 20.0f, 1.5f);
    float                      from[4] = {x, y + kProbeUp, z, 1.0f};
    float                      hit[4];
    if (count <= 0 || CheckHitVertical(polys.data(), count, from, -(kProbeUp + kProbeDown), hit, 0) < 0) {
        return false;
    }
    *ground = hit[1];
    return true;
}

bool Hittable(const CMonstorUnit &unit, int monster) {
    const MONSTOR &live = unit.monster[monster];
    return live.state != -1 && live.revealed != 0 && live.hp > 0;
}

float Flat(const float *a, const float *b) {
    return std::hypot(a[0] - b[0], a[2] - b[2]);
}

// The monster she is locked on to, else the nearest to her within sight, or -1.
int PickTarget() {
    if (NowMonstorUnit == nullptr) {
        return -1;
    }
    const CMonstorUnit &unit = *NowMonstorUnit;
    if (lockOnTargetFlag != 0 && lockOnTargetNo >= 0 && lockOnTargetNo < 16 && Hittable(unit, lockOnTargetNo)) {
        return lockOnTargetNo;
    }
    int   best = -1;
    float nearest = kSpiritBeastSight;
    for (int monster = 0; monster < 16; monster++) {
        if (!Hittable(unit, monster)) {
            continue;
        }
        float distance = Flat(unit.chara[monster][0].pos, CharaMain.pos);
        if (distance <= nearest) {
            best = monster;
            nearest = distance;
        }
    }
    return best;
}

// A monster whose hurt sphere the cat's middle touches, or -1.
int Touching() {
    if (NowMonstorUnit == nullptr) {
        return -1;
    }
    const CMonstorUnit &unit = *NowMonstorUnit;
    float               middle[3] = {cat.pos[0], cat.pos[1] + kBody, cat.pos[2]};
    for (int monster = 0; monster < 16; monster++) {
        if (!Hittable(unit, monster)) {
            continue;
        }
        const MONSTOR_EFFECT_STATE &effect = unit.effect[monster];
        for (int sphere = 0; sphere < 16; sphere++) {
            if (effect.timer[sphere] == 0) {
                continue;
            }
            const float *c = effect.position[sphere];
            float        d = std::sqrt((c[0] - middle[0]) * (c[0] - middle[0]) + (c[1] - middle[1]) * (c[1] - middle[1]) +
                                       (c[2] - middle[2]) * (c[2] - middle[2]));
            if (d <= effect.radius[sphere] + kSpiritBeastTouch) {
                return monster;
            }
        }
    }
    return -1;
}

// The cat's one hit, on the monster it touched.
void Strike(int monster) {
    if (NowColData == nullptr || NowWeaponHave == nullptr) {
        return;
    }
    float centre[4];
    sceVu0CopyVector(centre, NowMonstorUnit->chara[monster][0].pos);
    centre[1] += std::max(kBody, NowMonstorUnit->chara[monster][0].body_height * 0.5f);
    int damage = std::max(1, static_cast<int>(std::lround(kSpiritBeastDamage * static_cast<float>(NowWeaponHave->attack))));
    int hit = NowColData->Set(centre, damage, kHitLife, std::max(kSpiritBeastHitRadius, NowMonstorUnit->chara[monster][0].body_height * 0.5f),
                              0.0f, kMonsters, kHitKind, 0, 0);
    if (hit < 0) {
        return;
    }
    COLLISION_HIT &record = NowColData->hit[hit];
    record.owner = CHARA_XIAO;
    record.attack_no = 0;
    record.weapon_flags = NowWeaponHave->flags | kMark;
    record.vs_monster = NowWeaponHave->vs_monster;
    if (NowWeaponHave->best_elem >= 0 && NowWeaponHave->best_elem < 5) {
        record.flags = GetWeaponElementAttr(NowWeaponHave->best_elem);
    }
    sceVu0CopyVector(record.knockback_origin, cat.pos);
    record.knockback_speed = kSpiritBeastKickSpeed;
    record.knockback_decay = kSpiritBeastKickDecay;
    record.knockback_mode = 2;
}

void Fade() {
    if (cat.phase != SpiritBeastPhase::kFading) {
        Phase(SpiritBeastPhase::kFading);
        cat.fade = 0;
    }
}

void Face(const float *towards) {
    float dx = towards[0] - cat.pos[0], dz = towards[2] - cat.pos[2];
    if (dx * dx + dz * dz > 0.01f) {
        cat.yaw = std::atan2(dx, dz);
    }
}

// Onto the floor under it: true when it has landed there.
bool Settle() {
    float ground;
    if (FloorAt(cat.pos[0], cat.pos[1], cat.pos[2], &ground)) {
        cat.ground = ground;
    }
    return cat.pos[1] <= cat.ground;
}

void StepFlying() {
    cat.vel[1] -= kSpiritBeastFall;
    for (int i = 0; i < 3; i++) {
        cat.pos[i] += cat.vel[i];
    }
    if (Settle() && cat.vel[1] <= 0.0f) {
        cat.pos[1] = cat.ground;
        Motion(kKeyLand, kHoldLast);
        Phase(SpiritBeastPhase::kLanding);
    }
}

void StepGround() {
    if (cat.phase_age % kRetarget == 1 || cat.target < 0 || !Hittable(*NowMonstorUnit, cat.target)) {
        cat.target = PickTarget();
    }
    if (cat.target < 0) {
        Motion(kKeySit);
        Phase(SpiritBeastPhase::kSitting);
        return;
    }
    const float *goal = NowMonstorUnit->chara[cat.target][0].pos;
    if (Flat(goal, cat.pos) <= kSpiritBeastPounce) {
        Face(goal);
        Motion(kKeyReady, kHoldLast, kReadyRate);
        Phase(SpiritBeastPhase::kReady);
        return;
    }
    float waypoint[4] = {goal[0], goal[1], goal[2], 1.0f}, reached[4];
    NavigateTowards(cat.pos, goal, kClearance, cat.ground, waypoint, reached);
    float dx = waypoint[0] - cat.pos[0], dz = waypoint[2] - cat.pos[2], length = std::hypot(dx, dz);
    if (length > 0.01f) {
        float step = std::min(kSpiritBeastRun, length);
        cat.pos[0] += dx / length * step;
        cat.pos[2] += dz / length * step;
        Face(waypoint);
    }
    float ground;
    if (FloorAt(cat.pos[0], cat.pos[1], cat.pos[2], &ground)) {
        cat.ground = ground;
        cat.pos[1] = ground;
    }
    if (cat.phase != SpiritBeastPhase::kRunning) {
        Phase(SpiritBeastPhase::kRunning);
    }
    Motion(kKeyRun);
}

void Leap() {
    const CCharacter &body = NowMonstorUnit->chara[cat.target][0];
    float             aim[3] = {body.pos[0], body.pos[1] + std::min(body.body_height * 0.5f, 10.0f), body.pos[2]};
    float             t = static_cast<float>(kSpiritBeastLeapFrames);
    cat.vel[0] = (aim[0] - cat.pos[0]) / t;
    cat.vel[2] = (aim[2] - cat.pos[2]) / t;
    cat.vel[1] = (aim[1] - kBody - cat.pos[1]) / t + 0.5f * kSpiritBeastGravity * t;
    Face(aim);
    Motion(kKeyTakeOff, kHoldLast);
    Phase(SpiritBeastPhase::kLeaping);
}

void StepLeaping() {
    if (cat.phase_age > 8) {
        Motion(kKeyLeap);
    }
    cat.vel[1] -= kSpiritBeastGravity;
    for (int i = 0; i < 3; i++) {
        cat.pos[i] += cat.vel[i];
    }
    if (Settle() && cat.vel[1] <= 0.0f) {
        cat.pos[1] = cat.ground;
        Motion(kKeyLand, kHoldLast);
        Phase(SpiritBeastPhase::kLanding);
    }
}

void Place() {
    if (model == nullptr || model->frame == nullptr) {
        return;
    }
    float grow = std::min(1.0f, static_cast<float>(cat.age) / kSpiritBeastGrowFrames);
    float shown = cat.phase == SpiritBeastPhase::kFading ? 1.0f - static_cast<float>(cat.fade) / kSpiritBeastFadeFrames : 1.0f;
    model->SetPosition(cat.pos[0], cat.pos[1], cat.pos[2]);
    model->SetRotation(0.0f, cat.yaw, 0.0f);
    model->SetScale(grow, grow, grow);
    model->ambient_offset[3] = 128.0f * std::max(0.0f, shown);
    model->Step();
}

} // namespace

bool SpiritBeastActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponActive(ITEM_WEAPON_DIVINEBEASTTITLE);
}

SpiritBeastPhase SpiritBeastPhaseNow() {
    return cat.phase;
}

const float *SpiritBeastPosition() {
    return cat.pos;
}

void SpiritBeastReset() {
    cat = Cat{};
    model = nullptr;
    charge.Reset();
}

void SpiritBeastStep() {
    bool active = SpiritBeastActive();
    charge.Step(active);
    if (active && model == nullptr && !cat.failed) {
        model = EnterCharacter(kCatFile, kCatCfg);
        cat.failed = model == nullptr;
        if (model != nullptr) {
            model->SetMotion(kKeyStand, 0);
        }
    }
    if (cat.phase == SpiritBeastPhase::kNone || NowMonstorUnit == nullptr) {
        return;
    }
    cat.age++;
    cat.phase_age++;
    if (cat.phase != SpiritBeastPhase::kFading) {
        int touched = Touching();
        if (touched >= 0) {
            Strike(touched);
            Fade();
        } else if (cat.age >= kSpiritBeastLifeFrames || !active) {
            Fade();
        }
    }
    switch (cat.phase) {
        case SpiritBeastPhase::kFlying:
            StepFlying();
            break;
        case SpiritBeastPhase::kLanding:
            if (cat.phase_age >= kLandFrames) {
                StepGround();
            }
            break;
        case SpiritBeastPhase::kRunning:
            StepGround();
            break;
        case SpiritBeastPhase::kSitting:
            if (cat.phase_age % kRetarget == 0 && PickTarget() >= 0) {
                StepGround();
            }
            break;
        case SpiritBeastPhase::kReady:
            if (cat.target < 0 || !Hittable(*NowMonstorUnit, cat.target)) {
                StepGround();
            } else {
                Face(NowMonstorUnit->chara[cat.target][0].pos);
                if (cat.phase_age >= kReadyFrames) {
                    Leap();
                }
            }
            break;
        case SpiritBeastPhase::kLeaping:
            StepLeaping();
            break;
        case SpiritBeastPhase::kFading:
            if (++cat.fade >= kSpiritBeastFadeFrames) {
                cat.phase = SpiritBeastPhase::kNone;
                return;
            }
            break;
        default:
            break;
    }
    Place();
}

void SpiritBeastFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= 12 || !SpiritBeastActive() || !charge.TakeCharged() || model == nullptr) {
        return;
    }
    cat = Cat{};
    for (int i = 0; i < 3; i++) {
        cat.pos[i] = shots.pos[slot][i];
        cat.vel[i] = shots.vector[slot][i];
    }
    cat.yaw = std::atan2(cat.vel[0], cat.vel[2]);
    cat.ground = CharaMain.pos[1];
    model->SetMotion(kKeyLeap, 0);
    Phase(SpiritBeastPhase::kFlying);
    Place();
    // The cat in the pellet's place.
    shots.used[slot] = 0;
}

void SpiritBeastDraw() {
    if (model == nullptr || model->frame == nullptr || cat.phase == SpiritBeastPhase::kNone) {
        return;
    }
    TexManager.ReloadTexture(Vif1Packet, kShotEffectBlock);
    model->Draw();
    float shown = cat.phase == SpiritBeastPhase::kFading ? 1.0f - static_cast<float>(cat.fade) / kSpiritBeastFadeFrames : 1.0f;
    float grow = std::min(1.0f, static_cast<float>(cat.age) / kSpiritBeastGrowFrames);
    float middle[4] = {cat.pos[0], cat.pos[1] + kBody * grow, cat.pos[2], 1.0f};
    DrawGlowDisc(GlowLook::kBlue, middle, kGlowSize * grow * shown, kGlowPull);
}

void SpiritBeastLight(const CCharacter &character, float *ambient) {
    if (model == nullptr || &character != model || cat.phase == SpiritBeastPhase::kNone) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        ambient[i] += kTint[i];
    }
}

bool SpiritBeastHit(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kMark) != 0;
}

} // namespace expanded
