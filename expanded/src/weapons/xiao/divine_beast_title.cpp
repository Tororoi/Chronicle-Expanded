#include "expanded/weapons/xiao/divine_beast_title.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/data/monster_scripts.hpp"
#include "expanded/data/spirit_cat.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/divine_beast_title/ground.hpp"
#include "expanded/weapons/xiao/divine_beast_title/targets.hpp"
#include "expanded/weapons/xiao/spirit_cape.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "motionmodel.hpp"
#include "shot_effect.hpp"
#include "shot_utils.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace expanded {

namespace {

using namespace divine_beast_title;

constexpr int         kPlayOnce = 2, kCut = 4; // motion flags: hold on the last frame; start at once, unblended
constexpr float       kBlend = 0.1f;           // the engine's own cross-fade step
constexpr float       kRootTouch = 2.0f;       // the touch point's height over the root, with no head to touch with
constexpr int         kGlowDelay = 4;          // the torch flame's first draws are empty
constexpr float       kGlowSize = 22.5f, kGlowPull = 5.0f;
constexpr float       kHeadFallback[3] = {0.0f, 6.0f, 0.0f};
constexpr const char *kHeadFrame = "cat_kao";
constexpr const char *kHipsFrame = "cat_kosibone", *kSpineFrame = "cat_sebone2";
constexpr const char *kWingFrames[] = {"cat_rwingm", "cat_lwingm"};
constexpr int         kSkippedByParent = 4; // CFrameAttr::draw_on: the parent's draw passes over it

// A weapon's cat: whether it wears the wings or the cape and mask, its glow (the cape's is the element's) and the
// ambient it adds, its pounce range, whether it steers past the apex, its size.
struct Look {
    bool     wings;
    bool     cape;
    GlowLook glow;
    float    tint[3];
    float    range;
    bool     steers;
    float    scale;
};

constexpr Look kTitleLook = {
    false, false, GlowLook::kBlue, {12.0f, 24.0f, 48.0f},
       kSpiritBeastPounce, false, 1.0f
};
constexpr Look kShooterLook = {
    true, false, GlowLook::kWhite, {20.0f, 20.0f, 20.0f},
       kSpiritBeastPounceWinged, true, kAngelShooterCatScale
};
constexpr Look kShooterCapeLook = {
    false, true, GlowLook::kNoElement, {20.0f, 20.0f, 20.0f},
       kSpiritBeastPounceWinged, true, kAngelShooterCatScale
};
constexpr Look kGearLook = {
    true, false, GlowLook::kGold, {27.0f, 26.0f, 20.0f},
       kSpiritBeastPounceWinged, true, kAngelGearCatScale
};
constexpr Look kGearCapeLook = {
    false, true, GlowLook::kNoElement, {20.0f, 20.0f, 20.0f},
       kSpiritBeastPounceWinged, true, kAngelGearCatScale
};
constexpr GlowLook kElementGlows[6] = {GlowLook::kFire, GlowLook::kIce, GlowLook::kThunder, GlowLook::kWind, GlowLook::kHoly, GlowLook::kNoElement};

[[maybe_unused]] const bool kRegistered = (RegisterOwnSound(kSpiritBeastSitSound), true);

// The look of the cat the weapon in Xiao's hand lets go, or null for none.
const Look *LookNow() {
    if (WeaponWielded(ITEM_WEAPON_DIVINEBEASTTITLE)) {
        return &kTitleLook;
    }
    if (WeaponWielded(ITEM_WEAPON_ANGEL_SHOOTER)) {
        return &kShooterLook;
    }
    if (WeaponWielded(ITEM_WEAPON_ANGE_GEAR)) {
        return &kGearLook;
    }
    switch (SuperSteveSphere()) {
        case ITEM_WEAPON_DIVINEBEASTTITLE:
            return &kTitleLook;
        case ITEM_WEAPON_ANGEL_SHOOTER:
            return &kShooterCapeLook;
        case ITEM_WEAPON_ANGE_GEAR:
            return &kGearCapeLook;
        default:
            return nullptr;
    }
}

// The element selected on Xiao's weapon (0 Fire to 4 Holy), else 5.
int Element() {
    int element = SelectedElement();
    return element >= 0 ? element : WEAPON_ELEMENT_NONE;
}

struct Cat {
    SpiritBeastPhase phase = SpiritBeastPhase::kNone;
    float            pos[4] = {0, 0, 0, 1};
    float            vel[3] = {};
    float            dir[2] = {}; // where it faces, and its floor casts lie
    float            yaw = 0.0f;
    float            scale = 0.0f;
    float            floor = 0.0f;
    float            apex = 0.0f;
    float            last_frame = 0.0f; // the clip frame seen last: a lower one is the clip wrapping
    int              age = 0;           // frames since it was bound
    int              grown = 0;         // frames on the pellet before this one
    int              fade = -1;         // frames into its fade, or -1
    bool             pounce = false;    // the flight is a pounce, not the fall from the pellet
    int              target = -1;
    bool             aimed = false;
    float            aim[4] = {0, 0, 0, 1};
    int              leap_frames = kSpiritBeastLeapFrames;
    int              retarget = 0;
    bool             spent = false; // a hit was taken
    int              planted = -1;  // the hit waiting on the monster's CheckDmg
    int              planted_wait = 0;
    bool             taken = false;
    int              damage = 0;
    CSHOT           *shots = nullptr;
    int              slot = -1;
    const Look      *look = &kTitleLook;
};

Cat         cat;
ShotCharge  charge{kSpiritBeastFrames};
CSHOT      *pending_shots = nullptr; // a charged pellet, bound once it has stepped
int         pending_slot = -1;
CCharacter *model = nullptr;
CFrame     *wings[2] = {};
bool        failed = false;
float       head[3] = {kHeadFallback[0], kHeadFallback[1], kHeadFallback[2]};
CatGround   ground;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(SpiritBeastReset), true);

CCharacter *Load() {
    CCharacter *loaded = EnterFloorCharacter(kSpiritCatModelFile, kSpiritCatCfg);
    static bool logged_missing = false;
    if (loaded == nullptr && !logged_missing) {
        logged_missing = true;
        std::fprintf(stderr, "divine beast title: no cat, %s could not be loaded (dcdata expanded <data> builds it)\n", kSpiritCatModelFile);
    }
    return loaded;
}

// The clip its motion is in, as the engine left it: a cross-fade holds it at the clip it fades out of.
float ClipFrame() {
    return model != nullptr ? model->motion_type.state.time : 0.0f;
}

void Key(int key) {
    if (model != nullptr) {
        model->motion_no = key;
    }
}

void Flags(int set, int clear = 0) {
    if (model != nullptr) {
        model->motion_flags = (model->motion_flags & ~clear) | set;
    }
}

void Rate(float speed) {
    if (model != nullptr) {
        model->motion_speed = speed;
    }
}

void BlendStep(float step) {
    if (model != nullptr) {
        model->motion_type.state.blend_step = step;
    }
}

// Turns the cat to the aim point where it is now.
void FaceAim() {
    float dx = cat.aim[0] - cat.pos[0], dz = cat.aim[2] - cat.pos[2], d = std::sqrt(dx * dx + dz * dz);
    if (cat.aimed && d > 0.0f) {
        cat.dir[0] = dx / d;
        cat.dir[1] = dz / d;
    }
}

// The flight re-aimed to come down on the aim point in t frames, the cat turned to it.
void SteerTo(float t) {
    cat.vel[0] = (cat.aim[0] - cat.pos[0]) / t;
    cat.vel[2] = (cat.aim[2] - cat.pos[2]) / t;
    FaceAim();
}

// How much of the cat shows: all of it until it fades.
float Shown() {
    return cat.fade < 0 ? 1.0f : std::max(0.0f, 1.0f - static_cast<float>(cat.fade) / kSpiritBeastFadeFrames);
}

// How near the cat crouches by its target: its look's reach, but kSpiritBeastPounce by a mimic still in its chest.
float Range() {
    return Dormant(cat.target) ? kSpiritBeastPounce : cat.look->range;
}

// Whether the target cannot be hit: a mimic in its chest, or a monster in its untouchable frames.
bool Hold() {
    return cat.target >= 0 && Untouchable(cat.target);
}

void Aim() {
    if (cat.target >= 0 && Targetable(cat.target)) {
        AimPoint(cat.target, cat.aim);
        cat.aimed = true;
    }
}

void Target(int monster) {
    cat.target = monster;
    if (monster >= 0) {
        cat.leap_frames = TallLeap(monster) ? kSpiritBeastLeapFramesTall : kSpiritBeastLeapFrames;
        Aim();
    } else {
        cat.aimed = false;
    }
}

// Each frame: the aim follows the target, a lock-on made since is taken (from the crouch, back to walking), and walking
// or sitting it finds a new target when it has none.
void Watch() {
    bool walking = cat.phase == SpiritBeastPhase::kWalking || cat.phase == SpiritBeastPhase::kSitting;
    if (cat.target >= 0 && !Targetable(cat.target)) {
        if (walking) {
            Target(PickTarget());
        } else {
            cat.target = -1; // the pounce goes on to where it last was
        }
    }
    Aim();
    if (cat.target >= 0 && (walking || cat.phase == SpiritBeastPhase::kReady) && ++cat.retarget >= kSpiritBeastRetarget) {
        cat.retarget = 0;
        int locked = LockOnCandidate();
        if (locked >= 0 && locked != cat.target) {
            Target(locked);
            if (cat.phase == SpiritBeastPhase::kReady) {
                Flags(0, kPlayOnce);
                cat.phase = SpiritBeastPhase::kWalking;
            }
        }
    }
    if (cat.target < 0 && walking) {
        cat.aimed = false;
        if (++cat.retarget >= kSpiritBeastRetarget) {
            cat.retarget = 0;
            Target(PickTarget());
        }
    }
}

// The cat's touch point: its head as last posed, else its root.
void HeadPoint(float *point) {
    point[0] = cat.pos[0], point[1] = cat.pos[1] + kRootTouch, point[2] = cat.pos[2], point[3] = 1.0f;
    CFrame *kao = model != nullptr ? FrameNamed(model->frame, kHeadFrame) : nullptr;
    if (kao != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, at;
        kao->GetWorldPosition(at, origin);
        point[0] = at[0], point[1] = at[1], point[2] = at[2];
    }
}

// The pellet's own test from point along move: a live monster's hurt sphere within its reach, else the map.
SHOT_COLLISION_RESULT TouchTest(float *point, float *move) {
    float out[4];
    if (NowDngMap != nullptr) {
        return checkCollision(out, point, move, kHitsMonsters, kSpiritBeastTouch);
    }
    // With no map, its monster test alone.
    return MonsterTouching(*NowMonstorUnit, point, kSpiritBeastTouch) >= 0 ? SHOT_COLLISION_MONSTER : SHOT_COLLISION_NONE;
}

// Falling or landing: the monster its head touches is planted the cat's hit, which waits on its CheckDmg.
void Touch() {
    if (cat.planted >= 0 || cat.spent || NowColData == nullptr || NowWeaponHave == nullptr || NowMonstorUnit == nullptr) {
        return;
    }
    float point[4];
    HeadPoint(point);
    if (TouchTest(point, cat.vel) != SHOT_COLLISION_MONSTER) {
        return;
    }
    int element = SelectedElement();
    int attribute = element >= 0 && NowWeaponHave->elem[element] > 0 ? GetWeaponElementAttr(element) : 0;
    int hit = NowColData->Set(point, std::max(1, cat.damage), 1, kSpiritBeastHitRadius, 0.0f, kHitsMonsters, kSwingReaction, attribute, 0);
    if (hit < 0) {
        return;
    }
    COLLISION_HIT &record = NowColData->hit[hit];
    record.owner = CHARA_XIAO;
    record.attack_no = 0;
    record.weapon_flags = NowWeaponHave->flags | kSpiritBeastMark;
    record.vs_monster = NowWeaponHave->vs_monster;
    sceVu0CopyVector(record.knockback_origin, point);
    record.knockback_speed = kSpiritBeastKickSpeed;
    record.knockback_decay = kSpiritBeastKickDecay;
    record.knockback_mode = kSpiritBeastSphereKick;
    cat.planted = hit;
    cat.planted_wait = kSpiritBeastHitWait;
    cat.taken = false;
}

void StartFade() {
    if (cat.fade < 0) {
        cat.fade = 0;
    }
}

// The planted hit's fate: taken, the cat is spent and fades; gone untaken, or not taken in time, it may touch again.
void Resolve() {
    if (cat.planted < 0 || NowColData == nullptr) {
        return;
    }
    if (NowColData->active[cat.planted] == 0) {
        if (cat.taken) {
            cat.spent = true;
            StartFade();
        }
        cat.planted = -1;
    } else if (--cat.planted_wait <= 0) {
        NowColData->active[cat.planted] = 0;
        cat.planted = -1;
    }
}

void StartLand() {
    BlendStep(kBlend);
    Rate(-1.0f);
    Flags(kPlayOnce | kCut);
    Key(kCatLand);
    cat.last_frame = 0.0f;
    cat.phase = SpiritBeastPhase::kLanding;
}

void FallPose() {
    if (model != nullptr && model->motion_no == kCatFloat) {
        Flags(0, kPlayOnce);
        BlendStep(1.0f / kSpiritBeastFallBlend);
        Rate(-1.0f);
    }
    Key(kCatLeap);
}

// The frames until the arc reaches the floor.
float Horizon() {
    float drop = cat.pos[1] - cat.floor, up = cat.vel[1];
    return (up + std::sqrt(up * up + 2.0f * kSpiritBeastGravity * drop)) / kSpiritBeastGravity;
}

// The fall, off the pellet or the pounce's arc: it moves, then gravity; the land clip cut in ahead of the touchdown.
void Falling() {
    for (int i = 0; i < 3; i++) {
        cat.pos[i] += cat.vel[i];
    }
    cat.vel[1] -= kSpiritBeastGravity;
    if (!(cat.floor < cat.pos[1])) {
        cat.pos[1] = cat.floor;
        StartLand();
    } else {
        float t = Horizon();
        if (!(kSpiritBeastLandLead < t)) {
            StartLand();
        } else if (cat.pounce) {
            // Re-aimed at the target while it rises, over the frames still to fly; a steering cat on until it has fallen
            // halfway from the apex to the floor.
            bool rising = cat.vel[1] > 0.0f;
            if (rising && cat.look->steers) {
                cat.apex = cat.pos[1];
            }
            if ((rising || (cat.look->steers && cat.apex + cat.floor < 2.0f * cat.pos[1])) && cat.aimed) {
                SteerTo(t);
            }
            // The float-up held until its fade into the fall ends as the land clip starts.
            if (kSpiritBeastLandLead + kSpiritBeastFallBlend < t) {
                Key(kCatFloat);
            } else {
                FallPose();
            }
        } else {
            FallPose();
        }
    }
    Touch();
}

// The landing: its momentum while in the air, or until the paws touch; at the clip's end, walking.
bool Landing() {
    float frame = ClipFrame(), last = cat.last_frame;
    cat.last_frame = frame;
    if (!(frame < kSpiritBeastLandEnd - 1.0f) || frame < last) {
        cat.pounce = false;
        Flags(0, kPlayOnce);
        cat.phase = SpiritBeastPhase::kWalking;
        return true;
    }
    bool airborne = cat.floor < cat.pos[1];
    if (airborne) {
        cat.pos[1] += cat.vel[1];
        cat.vel[1] -= kSpiritBeastGravity;
        if (!(cat.floor < cat.pos[1])) {
            cat.pos[1] = cat.floor;
        }
    }
    if (airborne || frame < kSpiritBeastLandTouch) {
        // A steering cat's pounce re-aimed on while above halfway, until kSpiritBeastCommitHorizon frames out.
        if (airborne && cat.look->steers && cat.pounce && cat.aimed && cat.apex + cat.floor < 2.0f * cat.pos[1]) {
            float t = Horizon();
            if (!(t < kSpiritBeastCommitHorizon)) {
                SteerTo(t);
            }
        }
        cat.pos[0] += cat.vel[0];
        cat.pos[2] += cat.vel[2];
    }
    Key(kCatLand);
    Touch();
    return false;
}

// Straight at the aim point, crouching once within range; a wall ahead stops it standing; with nothing to go for it sits.
void Walking() {
    if (!cat.aimed) {
        if (model != nullptr && model->motion_no != kCatSit) {
            Flags(0, kPlayOnce);
            Key(kCatSit);
            Rate(-1.0f);
            PlayOwnSound(kSpiritBeastSitSound, false);
        }
        cat.phase = SpiritBeastPhase::kSitting;
        return;
    }
    cat.phase = SpiritBeastPhase::kWalking;
    float dx = cat.aim[0] - cat.pos[0], dz = cat.aim[2] - cat.pos[2], d = std::sqrt(dx * dx + dz * dz);
    if (d > 0.0f) {
        cat.dir[0] = dx / d;
        cat.dir[1] = dz / d;
        if (!(Range() < d)) {
            Flags(kPlayOnce);
            Key(kCatReady);
            Rate(-1.0f);
            cat.last_frame = 0.0f;
            cat.phase = SpiritBeastPhase::kReady;
            return;
        }
    }
    float walk = kSpiritBeastWalk * cat.look->scale, move[2] = {cat.dir[0] * walk, cat.dir[1] * walk};
    if (ground.WallAhead(cat.pos, cat.floor, move)) {
        Key(kCatStand);
        Rate(-1.0f);
    } else {
        cat.pos[0] += move[0];
        cat.pos[2] += move[1];
        Key(kCatWalk);
        Rate(kSpiritBeastWalkRate);
    }
    cat.pos[1] = cat.floor;
}

// The crouch, turning to the target, until its clip ends and nothing holds it: then the float-up.
void Ready() {
    float frame = ClipFrame();
    bool  done = false;
    if (!(frame < kSpiritBeastReadyStart) && !(kSpiritBeastReadyEnd + 1.0f < frame)) {
        float last = cat.last_frame;
        cat.last_frame = frame;
        done = !(frame < kSpiritBeastReadyEnd - 1.0f) || frame < last;
    }
    if (done && !Hold()) {
        Flags(kPlayOnce);
        Key(kCatFloat);
        Rate(-1.0f);
        cat.last_frame = 0.0f;
        cat.phase = SpiritBeastPhase::kWindUp;
        return;
    }
    FaceAim();
    Key(kCatReady);
}

// The float-up's wind-up, turning to the target, until its feet leave the floor: then the leap, at where the target is
// now, unless it is more than twice the range away.
void WindUp() {
    float frame = ClipFrame();
    if (frame < kSpiritBeastFloatStart || frame < kSpiritBeastFloatLaunch) {
        FaceAim();
        Key(kCatFloat);
        return;
    }
    float dx = cat.aim[0] - cat.pos[0], dz = cat.aim[2] - cat.pos[2], d = std::sqrt(dx * dx + dz * dz);
    if (!cat.aimed || (d > 0.0f && 2.0f * Range() < d)) {
        Flags(0, kPlayOnce);
        cat.phase = SpiritBeastPhase::kWalking;
        return;
    }
    if (d > 0.0f) {
        cat.dir[0] = dx / d;
        cat.dir[1] = dz / d;
    }
    float t = static_cast<float>(cat.leap_frames);
    cat.pounce = true;
    cat.vel[0] = dx / t;
    cat.vel[2] = dz / t;
    cat.vel[1] = (cat.aim[1] - cat.floor) / t + 0.5f * kSpiritBeastGravity * t; // through the aim point's height at t
    cat.apex = cat.pos[1];
    Flags(kPlayOnce);
    Key(kCatFloat);
    cat.phase = SpiritBeastPhase::kLeaping;
}

// On the pellet: the head on its point, the cat grown by the frames before this one; at full size it breaks away.
void Riding() {
    if (cat.shots == nullptr || cat.shots->used[cat.slot] == 0) { // its pellet ended under it: full size where it was, fading
        cat.scale = cat.look->scale;
        StartFade();
        return;
    }
    const float *at = cat.shots->pos[cat.slot], *v = cat.shots->vector[cat.slot];
    float        k = std::min(1.0f, static_cast<float>(cat.grown) / kSpiritBeastGrowFrames) * cat.look->scale;
    float        h = std::sqrt(v[0] * v[0] + v[2] * v[2]), off[2] = {};
    if (h > 0.0f) {
        off[0] = (head[0] * v[2] + head[2] * v[0]) / h;
        off[1] = (head[2] * v[2] - head[0] * v[0]) / h;
    }
    cat.pos[0] = at[0] - k * off[0];
    cat.pos[1] = at[1] - k * head[1];
    cat.pos[2] = at[2] - k * off[1];
    cat.scale = k;
    Key(kCatLeap);
    cat.shots->size[cat.slot] = 1.0f - k / cat.look->scale;
    if (cat.grown++ < kSpiritBeastGrowFrames) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        cat.vel[i] = v[i];
    }
    cat.dir[0] = h > 0.0f ? v[0] / h : 0.0f;
    cat.dir[1] = h > 0.0f ? v[2] / h : 0.0f;
    cat.shots->life[cat.slot] = 1;
    cat.shots->halted[cat.slot] = 1;
    cat.shots = nullptr;
    cat.pounce = false;
    cat.phase = SpiritBeastPhase::kFalling;
}

void Place() {
    if (model == nullptr || model->frame == nullptr) {
        return;
    }
    if (cat.phase != SpiritBeastPhase::kRiding && cat.dir[0] * cat.dir[0] + cat.dir[1] * cat.dir[1] > 1e-6f) {
        cat.yaw = std::atan2(cat.dir[0], cat.dir[1]);
    }
    model->SetPosition(cat.pos[0], cat.pos[1], cat.pos[2]);
    model->SetRotation(0.0f, cat.yaw, 0.0f);
    model->SetScale(cat.scale, cat.scale, cat.scale);
    model->ambient_offset[3] = 128.0f * Shown();
    for (CFrame *wing : wings) {
        if (wing != nullptr) {
            wing->attr.draw_on = cat.look->wings ? wing->attr.draw_on & ~kSkippedByParent : wing->attr.draw_on | kSkippedByParent;
        }
    }
    model->Step();
}

// Where the head rests in the cat's own space: its rest pose's matrices up to the root.
void FindHead() {
    head[0] = kHeadFallback[0], head[1] = kHeadFallback[1], head[2] = kHeadFallback[2];
    CFrame *kao = model != nullptr ? FrameNamed(model->frame, kHeadFrame) : nullptr;
    if (kao == nullptr) {
        return;
    }
    float p[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    for (CFrame *frame = kao; frame != nullptr && frame != model->frame; frame = frame->parent) {
        float q[4];
        for (int c = 0; c < 4; c++) {
            q[c] = p[0] * frame->local[0][c] + p[1] * frame->local[1][c] + p[2] * frame->local[2][c] + p[3] * frame->local[3][c];
        }
        std::copy(q, q + 4, p);
    }
    head[0] = p[0], head[1] = p[1], head[2] = p[2];
}

// The charged pellet, now it has stepped once: the cat bound to it.
void Bind() {
    CSHOT *shots = pending_shots;
    int    slot = pending_slot;
    pending_shots = nullptr;
    if (shots == nullptr || shots->used[slot] == 0 || NowWeaponHave == nullptr) {
        return;
    }
    cat = Cat{};
    cat.look = LookNow();
    cat.shots = shots;
    cat.slot = slot;
    shots->halted[slot] = 1; // held, through everything: the cat's touch is the hit
    const float *v = shots->vector[slot];
    cat.yaw = std::hypot(v[0], v[2]) >= 1e-3f ? std::atan2(v[0], v[2]) : CharaMain.rotation.y;
    cat.damage = shots->damage[slot] + NowWeaponHave->attack;
    Target(PickTarget());
    cat.floor = cat.target >= 0 ? Body(cat.target).pos[1] : CharaMain.pos[1];
    BlendStep(kBlend);
    Flags(kCut);
    Key(kCatLeap);
    cat.phase = SpiritBeastPhase::kRiding;
}

// The cat gone, a pellet it still rides let go on its way.
void TakeAway() {
    if (cat.phase == SpiritBeastPhase::kRiding && cat.shots != nullptr && cat.shots->used[cat.slot] != 0) {
        cat.shots->halted[cat.slot] = 0;
        cat.shots->size[cat.slot] = 1.0f;
    }
    cat.phase = SpiritBeastPhase::kNone;
}

} // namespace

bool SpiritBeastActive() {
    return Playing(CHARA_XIAO) && BtEventMode == 0 && LookNow() != nullptr;
}

float SpiritBeastScale() {
    return cat.look->scale;
}

bool SpiritBeastWinged() {
    return cat.look->wings;
}

bool SpiritBeastCaped() {
    return cat.look->cape;
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

CCharacter *SpiritBeastModel() {
    return model;
}

void SpiritBeastReset() {
    cat = Cat{};
    model = nullptr;
    wings[0] = wings[1] = nullptr;
    failed = false;
    pending_shots = nullptr;
    charge.Reset();
    SpiritCapeReset();
}

void SpiritBeastStep() {
    bool active = SpiritBeastActive();
    bool holding = charge.Holding();
    charge.Step(active);
    if (active && model == nullptr && !failed) {
        model = Load();
        failed = model == nullptr;
        if (model != nullptr) {
            FindHead();
            Key(kCatLeap);
            for (int i = 0; i < 2; i++) {
                wings[i] = FrameNamed(model->frame, kWingFrames[i]);
            }
            SpiritCapeTake(*model); // off the cat's own cloth list, its mask out of the cat's draw: only a caped cat wears them
        }
    }
    // Another character or weapon in hand, or an event: the cat is gone at once.
    if (!active) {
        TakeAway();
        pending_shots = nullptr;
        return;
    }
    // A charged shot let go takes away the cat that is out, and so does a change of look.
    if ((holding && !charge.Holding() && charge.Ready()) || (cat.phase != SpiritBeastPhase::kNone && LookNow() != cat.look)) {
        TakeAway();
    }
    bool bound = false;
    if (pending_shots != nullptr) {
        Bind();
        bound = cat.phase == SpiritBeastPhase::kRiding;
    }
    if (cat.phase == SpiritBeastPhase::kNone || NowMonstorUnit == nullptr) {
        return;
    }
    if (cat.fade >= 0 && ++cat.fade >= kSpiritBeastFadeFrames) {
        cat.phase = SpiritBeastPhase::kNone;
        return;
    }
    Resolve();
    // Its time up, it fades where it stands.
    if (!cat.spent && cat.fade < 0 && cat.phase != SpiritBeastPhase::kRiding && cat.age >= kSpiritBeastLifeFrames) {
        StartFade();
    }
    bool standing = cat.fade >= 0 && !cat.spent;
    if (!standing) {
        if (cat.phase != SpiritBeastPhase::kRiding) {
            Watch();
        }
        switch (cat.phase) {
            case SpiritBeastPhase::kRiding:
                Riding();
                break;
            case SpiritBeastPhase::kFalling:
            case SpiritBeastPhase::kLeaping:
                ground.Probe(cat.pos, cat.dir, cat.floor);
                Falling();
                break;
            case SpiritBeastPhase::kLanding:
                ground.Probe(cat.pos, cat.dir, cat.floor);
                if (Landing()) {
                    Walking();
                }
                break;
            case SpiritBeastPhase::kWalking:
            case SpiritBeastPhase::kSitting:
                ground.Probe(cat.pos, cat.dir, cat.floor);
                Walking();
                break;
            case SpiritBeastPhase::kReady:
                Ready();
                break;
            case SpiritBeastPhase::kWindUp:
                WindUp();
                break;
            default:
                break;
        }
    }
    cat.age++;
    Place();
    if (cat.look->cape && bound) {
        SpiritCapeSize(cat.look->scale);
        SpiritCapeReseed();
    } else if (cat.look->cape && cat.phase != SpiritBeastPhase::kNone) {
        SpiritCapeStep(cat.yaw, cat.floor, cat.look->scale);
    }
}

void SpiritBeastFired(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots || !SpiritBeastActive() || !charge.TakeCharged() || model == nullptr) {
        return;
    }
    pending_shots = &shots;
    pending_slot = slot;
}

void SpiritBeastDraw() {
    if (model == nullptr || model->frame == nullptr || cat.phase == SpiritBeastPhase::kNone) {
        return;
    }
    DrawFloorCharacter(*model);
    if (cat.look->cape) {
        SpiritCapeDraw(*model, Element(), Shown());
    }
    if (cat.age <= kGlowDelay) {
        return;
    }
    float   middle[4] = {cat.pos[0], cat.pos[1] + kRootTouch, cat.pos[2], 1.0f};
    CFrame *hips = FrameNamed(model->frame, kHipsFrame);
    CFrame *spine = FrameNamed(model->frame, kSpineFrame);
    if (hips != nullptr && spine != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, a, b;
        hips->GetWorldPosition(a, origin);
        spine->GetWorldPosition(b, origin);
        for (int i = 0; i < 3; i++) {
            middle[i] = 0.5f * (a[i] + b[i]);
        }
    }
    DrawGlowDisc(cat.look->cape ? kElementGlows[Element()] : cat.look->glow, middle, kGlowSize * cat.look->scale * Shown(), kGlowPull);
}

void SpiritBeastLight(const CCharacter &character, float *ambient) {
    if (model == nullptr || &character != model || cat.phase == SpiritBeastPhase::kNone) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        ambient[i] += cat.look->tint[i]; // kept through its fade
    }
}

bool SpiritBeastHit(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kSpiritBeastMark) != 0;
}

void SpiritBeastHitTaken(const COLLISION_HIT &hit) {
    if (SpiritBeastHit(hit)) {
        cat.taken = true;
    }
}

int SpiritBeastSpherePercent(const MONSTOR_EFFECT_STATE &effect, int sphere, int owner, const COLLISION_HIT &hit, int percent) {
    return owner == CHARA_XIAO && hit.knockback_mode == effect.body_parameter[sphere][1] ? effect.body_parameter[sphere][0] : percent;
}

} // namespace expanded
