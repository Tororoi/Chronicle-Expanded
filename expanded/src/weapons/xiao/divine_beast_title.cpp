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
#include "expanded/data/spirit_cat.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "frame.hpp"
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

constexpr int   kHoldLast = 2, kCut = 4;     // SetMotion flags: stop on the last frame; start at once, unblended
constexpr float kBlend = 0.1f;               // the engine's own cross-fade step
constexpr float kFloatToFall = 1.0f / 16.0f; // the float-up's slower fade into the fall pose
constexpr float kRootTouch = 2.0f;           // the touch point's height over the root, with no head to touch with
constexpr float kProbeUp = 8.0f, kProbeDown = 40.0f;
constexpr float kClearance = 4.0f;
constexpr float kTallScale = 1.25f; // a monster drawn this large or larger gets the higher arc
constexpr int   kRetarget = 30;     // a lock-on made since is taken this often
constexpr int   kMark = 0x08000000; // in a hit's weapon flags: the cat's
constexpr int   kMonsters = 2;
constexpr float kGlowSize = 22.5f, kGlowPull = 5.0f;
constexpr float kTint[3] = {12.0f, 24.0f, 48.0f};
constexpr float kHeadFallback[3] = {0.0f, 6.0f, 2.0f};

struct Cat {
    SpiritBeastPhase phase = SpiritBeastPhase::kNone;
    float            pos[4] = {0, 0, 0, 1};
    float            vel[3] = {};
    float            yaw = 0.0f;
    float            floor = 0.0f;
    int              age = 0; // frames since it was let go
    int              phase_age = 0;
    int              fade = -1; // frames into its fade, or -1
    bool             landing = false;
    int              land_age = 0;
    int              leap_frames = kSpiritBeastLeapFrames;
    bool             rising = false;
    int              target = -1;
    bool             hit = false;
    int              damage = 0;
    CSHOT           *shots = nullptr;
    int              slot = -1;
};

Cat         cat;
ShotCharge  charge{kSpiritBeastFrames};
CCharacter *model = nullptr;
bool        failed = false;
float       head[3] = {kHeadFallback[0], kHeadFallback[1], kHeadFallback[2]};

void OnFloor() {
    SpiritBeastReset();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

CCharacter *Load() {
    CCharacter *loaded = EnterFloorCharacter(kSpiritCatModelFile, kSpiritCatCfg);
    static bool logged_missing = false;
    if (loaded == nullptr && !logged_missing) {
        logged_missing = true;
        std::printf("divine beast title: no cat, %s could not be loaded (dcdata expanded <data> builds it)\n", kSpiritCatModelFile);
    }
    return loaded;
}

void Phase(SpiritBeastPhase phase) {
    cat.phase = phase;
    cat.phase_age = 0;
}

void Motion(int key, int flags = 0, float speed = -1.0f) {
    if (model != nullptr && (model->motion_no != key || (flags & kCut) != 0)) {
        model->SetMotion(key, flags, speed);
    }
}

float Flat(const float *a, const float *b) {
    return std::hypot(a[0] - b[0], a[2] - b[2]);
}

void Face(const float *towards) {
    float dx = towards[0] - cat.pos[0], dz = towards[2] - cat.pos[2];
    if (dx * dx + dz * dz > 0.0001f) {
        cat.yaw = std::atan2(dx, dz);
    }
}

// The floor under the cat near its height, the way a thrown item finds it; unchanged over a pit.
void ProbeFloor() {
    if (NowDngMap == nullptr) {
        cat.floor = CharaMain.pos[1];
        return;
    }
    static std::vector<CCPoly> polys(8192);
    float                      centre[4] = {cat.pos[0], cat.pos[1], cat.pos[2], 1.0f};
    int                        count = setCollisionData(NowDngMap, polys.data(), centre, 20.0f, 1.5f);
    float                      from[4] = {cat.pos[0], cat.pos[1] + kProbeUp, cat.pos[2], 1.0f};
    float                      hit[4];
    if (count > 0 && CheckHitVertical(polys.data(), count, from, -(kProbeUp + kProbeDown), hit, 0) >= 0) {
        cat.floor = hit[1];
    }
}

bool Hittable(int monster) {
    if (NowMonstorUnit == nullptr || monster < 0 || monster >= 16) {
        return false;
    }
    const MONSTOR &live = NowMonstorUnit->monster[monster];
    return live.state != -1 && live.revealed != 0 && live.hp > 0;
}

const CCharacter &Body(int monster) {
    return NowMonstorUnit->chara[monster][0];
}

// The monster she is locked on to, else the nearest to her within sight, or -1.
int PickTarget() {
    if (NowMonstorUnit == nullptr) {
        return -1;
    }
    if (lockOnTargetFlag != 0 && Hittable(lockOnTargetNo)) {
        return lockOnTargetNo;
    }
    int   best = -1;
    float nearest = kSpiritBeastSight;
    for (int monster = 0; monster < 16; monster++) {
        if (Hittable(monster) && Flat(Body(monster).pos, CharaMain.pos) <= nearest) {
            best = monster;
            nearest = Flat(Body(monster).pos, CharaMain.pos);
        }
    }
    return best;
}

// The touch, as a pellet's (checkCollision): the head as last posed, else the root, against every live monster's hurt
// spheres; where it met one.
bool Touch(int *monster, float *point) {
    if (NowMonstorUnit == nullptr) {
        return false;
    }
    float   p[3] = {cat.pos[0], cat.pos[1] + kRootTouch, cat.pos[2]};
    CFrame *kao = model != nullptr && model->frame != nullptr ? model->frame->SearchFrame(const_cast<char *>("kao")) : nullptr;
    if (kao != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, at;
        kao->GetWorldPosition(at, origin);
        p[0] = at[0], p[1] = at[1], p[2] = at[2];
    }
    for (int m = 0; m < 16; m++) {
        if (!Hittable(m)) {
            continue;
        }
        const MONSTOR_EFFECT_STATE &effect = NowMonstorUnit->effect[m];
        for (int sphere = 0; sphere < 16; sphere++) {
            if (effect.timer[sphere] == 0) {
                continue;
            }
            const float *c = effect.position[sphere];
            float        d = std::sqrt((c[0] - p[0]) * (c[0] - p[0]) + (c[1] - p[1]) * (c[1] - p[1]) + (c[2] - p[2]) * (c[2] - p[2]));
            if (d <= kSpiritBeastTouch + effect.radius[sphere]) {
                *monster = m;
                point[0] = p[0], point[1] = p[1], point[2] = p[2], point[3] = 1.0f;
                return true;
            }
        }
    }
    return false;
}

// The cat's one hit, at the point that touched.
void Strike(float *point) {
    if (NowColData == nullptr || NowWeaponHave == nullptr) {
        return;
    }
    int hit = NowColData->Set(point, std::max(1, cat.damage), 1, kSpiritBeastHitRadius, 0.0f, kMonsters, 0, 0, 0);
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

void StartFade() {
    if (cat.fade < 0) {
        cat.fade = 0;
    }
}

// Down to the floor under gravity, the land clip started ahead of the touchdown and the momentum kept until the paws
// touch: false while still in the air or landing.
bool Fall(float gravity) {
    ProbeFloor();
    cat.vel[1] -= gravity;
    if (!cat.landing) {
        float drop = std::max(0.0f, cat.pos[1] - cat.floor);
        float up = cat.vel[1];
        float to_floor = (up + std::sqrt(std::max(0.0f, up * up + 2.0f * gravity * drop))) / gravity;
        if (to_floor <= kSpiritBeastLandLead) {
            cat.landing = true;
            cat.land_age = 0;
            if (model != nullptr) {
                model->motion_type.state.blend_step = kBlend;
            }
            Motion(kCatLand, kHoldLast | kCut);
        }
    }
    if (cat.landing && ++cat.land_age >= static_cast<int>(kSpiritBeastLandLead)) {
        cat.vel[0] = cat.vel[2] = 0.0f;
    }
    for (int i = 0; i < 3; i++) {
        cat.pos[i] += cat.vel[i];
    }
    if (cat.pos[1] <= cat.floor) {
        cat.pos[1] = cat.floor;
        cat.vel[1] = 0.0f;
    }
    if (cat.landing && cat.land_age >= kSpiritBeastLandFrames) {
        cat.landing = false;
        return true;
    }
    return false;
}

void Retarget() {
    if (cat.phase_age % kRetarget == 0 && lockOnTargetFlag != 0 && Hittable(lockOnTargetNo)) {
        cat.target = lockOnTargetNo;
    }
    if (!Hittable(cat.target)) {
        cat.target = PickTarget();
    }
}

void Walk() {
    Retarget();
    if (cat.target < 0) {
        Motion(kCatSit);
        if (cat.phase != SpiritBeastPhase::kSitting) {
            Phase(SpiritBeastPhase::kSitting);
        }
        return;
    }
    const float *goal = Body(cat.target).pos;
    if (Flat(goal, cat.pos) <= kSpiritBeastPounce) {
        Face(goal);
        Motion(kCatReady, kHoldLast);
        Phase(SpiritBeastPhase::kReady);
        return;
    }
    if (cat.phase != SpiritBeastPhase::kWalking) {
        Phase(SpiritBeastPhase::kWalking);
    }
    float waypoint[4] = {goal[0], goal[1], goal[2], 1.0f}, reached[4];
    NavigateTowards(cat.pos, goal, kClearance, cat.floor, waypoint, reached);
    float dx = waypoint[0] - cat.pos[0], dz = waypoint[2] - cat.pos[2], length = std::hypot(dx, dz);
    if (length > 0.01f) {
        float step = std::min(kSpiritBeastWalk, length);
        cat.pos[0] += dx / length * step;
        cat.pos[2] += dz / length * step;
        Face(waypoint);
    }
    ProbeFloor();
    cat.pos[1] = cat.floor;
    Motion(kCatWalk, 0, kSpiritBeastWalkRate);
}

// The pounce's arc to the target's feet over the frames left: its horizontal speed, and on the launch its rise.
void Aim(bool launch) {
    const float *goal = Body(cat.target).pos;
    float        left = static_cast<float>(std::max(1, cat.leap_frames - cat.phase_age));
    cat.vel[0] = (goal[0] - cat.pos[0]) / left;
    cat.vel[2] = (goal[2] - cat.pos[2]) / left;
    if (launch) {
        float t = static_cast<float>(cat.leap_frames);
        cat.vel[1] = (goal[1] - cat.pos[1]) / t + 0.5f * kSpiritBeastGravity * t;
    }
    Face(goal);
}

void Place() {
    if (model == nullptr || model->frame == nullptr) {
        return;
    }
    float grow = std::min(1.0f, static_cast<float>(cat.age) / kSpiritBeastGrowFrames);
    float shown = cat.fade < 0 ? 1.0f : 1.0f - static_cast<float>(cat.fade) / kSpiritBeastFadeFrames;
    model->SetPosition(cat.pos[0], cat.pos[1], cat.pos[2]);
    model->SetRotation(0.0f, cat.yaw, 0.0f);
    model->SetScale(grow, grow, grow);
    model->ambient_offset[3] = 128.0f * std::max(0.0f, shown);
    model->Step();
}

// Where the head rests in the cat's own space, from the posed model.
void FindHead() {
    head[0] = kHeadFallback[0], head[1] = kHeadFallback[1], head[2] = kHeadFallback[2];
    if (model == nullptr || model->frame == nullptr) {
        return;
    }
    CFrame *kao = model->frame->SearchFrame(const_cast<char *>("kao"));
    if (kao == nullptr) {
        return;
    }
    model->SetPosition(0.0f, 0.0f, 0.0f);
    model->SetRotation(0.0f, 0.0f, 0.0f);
    model->SetScale(1.0f, 1.0f, 1.0f);
    model->Step();
    sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, at;
    kao->GetWorldPosition(at, origin);
    head[0] = at[0], head[1] = at[1], head[2] = at[2];
}

// On the pellet: the head on its point, the cat grown by k.
void Ride(float k) {
    const float *at = cat.shots->pos[cat.slot];
    float        c = std::cos(cat.yaw), s = std::sin(cat.yaw);
    cat.pos[0] = at[0] - k * (head[0] * c + head[2] * s);
    cat.pos[1] = at[1] - k * head[1];
    cat.pos[2] = at[2] - k * (-head[0] * s + head[2] * c);
}

void Touched() {
    int   monster;
    float point[4];
    if (!cat.hit && Touch(&monster, point)) {
        cat.hit = true;
        Strike(point);
        StartFade();
    }
}

} // namespace

bool SpiritBeastActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponActive(ITEM_WEAPON_DIVINEBEASTTITLE);
}

SpiritBeastPhase SpiritBeastPhaseNow() {
    return cat.phase;
}

bool SpiritBeastFading() {
    return cat.fade >= 0;
}

const float *SpiritBeastPosition() {
    return cat.pos;
}

void SpiritBeastReset() {
    cat = Cat{};
    model = nullptr;
    failed = false;
    charge.Reset();
}

void SpiritBeastStep() {
    bool active = SpiritBeastActive();
    charge.Step(active);
    if (active && model == nullptr && !failed) {
        model = Load();
        failed = model == nullptr;
        if (model != nullptr) {
            Motion(kCatLeap);
            FindHead();
        }
    }
    if (cat.phase == SpiritBeastPhase::kNone || NowMonstorUnit == nullptr) {
        return;
    }
    cat.age++;
    cat.phase_age++;
    if (cat.fade >= 0 && ++cat.fade >= kSpiritBeastFadeFrames) {
        cat.phase = SpiritBeastPhase::kNone;
        return;
    }
    // Its time up, it fades where it stands.
    bool standing = cat.fade >= 0 && !cat.hit;
    if (!standing && (cat.age >= kSpiritBeastLifeFrames || !active)) {
        StartFade();
        standing = true;
    }
    if (!standing) {
        switch (cat.phase) {
            case SpiritBeastPhase::kRiding: {
                // Growing on the held pellet as it shrinks away, with no touch.
                bool  pellet = cat.shots != nullptr && cat.shots->used[cat.slot] != 0;
                float k = std::min(1.0f, static_cast<float>(cat.age) / kSpiritBeastGrowFrames);
                if (pellet) {
                    Ride(k);
                    cat.shots->size[cat.slot] = 1.0f - k;
                }
                // At full size, or with its pellet ended early, it breaks away on the pellet's velocity.
                if (cat.age >= kSpiritBeastGrowFrames || !pellet) {
                    if (pellet) {
                        for (int i = 0; i < 3; i++) {
                            cat.vel[i] = cat.shots->vector[cat.slot][i];
                        }
                        cat.shots->used[cat.slot] = 0;
                    }
                    Phase(SpiritBeastPhase::kFalling);
                }
                break;
            }
            case SpiritBeastPhase::kFalling:
            case SpiritBeastPhase::kLeaping:
                if (cat.phase == SpiritBeastPhase::kLeaping && cat.rising && Hittable(cat.target)) {
                    Aim(false); // re-aimed until the apex
                }
                if (cat.rising && cat.vel[1] - (cat.phase == SpiritBeastPhase::kLeaping ? kSpiritBeastGravity : kSpiritBeastFall) <= 0.0f) {
                    cat.rising = false;
                    if (model != nullptr) {
                        model->motion_type.state.blend_step = kFloatToFall;
                    }
                    Motion(kCatLeap);
                }
                Touched();
                if (Fall(cat.phase == SpiritBeastPhase::kLeaping ? kSpiritBeastGravity : kSpiritBeastFall)) {
                    Phase(SpiritBeastPhase::kLanding);
                    Walk();
                } else if (cat.landing && cat.phase != SpiritBeastPhase::kLanding) {
                    cat.phase = SpiritBeastPhase::kLanding;
                }
                break;
            case SpiritBeastPhase::kLanding:
                Touched();
                if (Fall(kSpiritBeastFall)) {
                    Walk();
                }
                break;
            case SpiritBeastPhase::kWalking:
                Walk();
                break;
            case SpiritBeastPhase::kSitting:
                if (cat.phase_age % kRetarget == 0) {
                    Walk();
                }
                break;
            case SpiritBeastPhase::kReady:
                Retarget();
                if (cat.target < 0) {
                    Walk();
                    break;
                }
                Face(Body(cat.target).pos);
                // Held crouched while the monster cannot be hit.
                if (cat.phase_age >= kSpiritBeastReadyFrames && NowMonstorUnit->monster[cat.target].invincible_timer <= 0) {
                    if (Flat(Body(cat.target).pos, cat.pos) > 2.0f * kSpiritBeastPounce) {
                        Walk();
                    } else {
                        Motion(kCatFloat, kHoldLast);
                        Phase(SpiritBeastPhase::kWindUp);
                    }
                }
                break;
            case SpiritBeastPhase::kWindUp:
                if (!Hittable(cat.target)) {
                    Walk();
                    break;
                }
                Face(Body(cat.target).pos);
                if (cat.phase_age >= kSpiritBeastWindUpFrames) {
                    cat.leap_frames = Body(cat.target).scale[0] >= kTallScale ? kSpiritBeastLeapFramesTall : kSpiritBeastLeapFrames;
                    Phase(SpiritBeastPhase::kLeaping);
                    cat.rising = true;
                    Aim(true);
                }
                break;
            default:
                break;
        }
    }
    Place();
}

void SpiritBeastFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= 12 || !SpiritBeastActive() || !charge.TakeCharged() || model == nullptr || NowWeaponHave == nullptr) {
        return;
    }
    cat = Cat{};
    cat.shots = &shots;
    cat.slot = slot;
    shots.halted[slot] = 1; // held where it was fired, through everything: the cat's touch is the hit
    cat.yaw = std::atan2(shots.vector[slot][0], shots.vector[slot][2]);
    cat.damage = shots.damage[slot] + NowWeaponHave->attack;
    cat.rising = true;
    cat.target = PickTarget();
    if (model != nullptr) {
        model->motion_type.state.blend_step = kBlend;
    }
    Motion(kCatLeap, kCut);
    Phase(SpiritBeastPhase::kRiding);
    Ride(0.0f);
    Place();
}

void SpiritBeastDraw() {
    if (model == nullptr || model->frame == nullptr || cat.phase == SpiritBeastPhase::kNone) {
        return;
    }
    TexManager.ReloadTexture(Vif1Packet, kFloorCharacterBlock);
    model->Draw();
    float   shown = cat.fade < 0 ? 1.0f : 1.0f - static_cast<float>(cat.fade) / kSpiritBeastFadeFrames;
    float   grow = std::min(1.0f, static_cast<float>(cat.age) / kSpiritBeastGrowFrames);
    float   middle[4] = {cat.pos[0], cat.pos[1] + 4.0f * grow, cat.pos[2], 1.0f};
    CFrame *hips = model->frame->SearchFrame(const_cast<char *>("kosibone"));
    CFrame *spine = model->frame->SearchFrame(const_cast<char *>("sebone2"));
    if (hips != nullptr && spine != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, a, b;
        hips->GetWorldPosition(a, origin);
        spine->GetWorldPosition(b, origin);
        for (int i = 0; i < 3; i++) {
            middle[i] = 0.5f * (a[i] + b[i]);
        }
    }
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
