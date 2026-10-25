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
#include "expanded/data/monster_scripts.hpp"
#include "expanded/data/spirit_cat.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/miniboss.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "expanded/weapons/xiao/spirit_cape.hpp"
#include "frame.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "motionmodel.hpp"
#include "shot_effect.hpp"
#include "shot_utils.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

extern s32 lockOnTargetNo;

namespace expanded {

namespace {

constexpr int   kPlayOnce = 2, kCut = 4; // motion flags: hold on the last frame; start at once, unblended
constexpr float kBlend = 0.1f;           // the engine's own cross-fade step
constexpr float kRootTouch = 2.0f;       // the touch point's height over the root, with no head to touch with
constexpr float kProbeUp = 8.0f, kProbeDown = 40.0f;
constexpr float kProbeAhead = 3.0f, kProbeBehind = 2.0f; // the floor's extra casts along the cat: it stands on the highest
constexpr int   kCharacterSkip = 1;                      // the polygons a character's floor and walls skip
constexpr float kWallHeight = 6.0f, kWallFrames = 3.0f;  // the wall probe: this far up, this many frames of walking ahead
constexpr float kTallScale = 1.25f;                      // a monster drawn this large or larger gets the higher arc
// Flyers, hoverers and the tall or large, and their stronger kinds: the higher arc.
constexpr Monster kTallLeapers[] = {
    Monster::HORNET,
    Monster::WITCH_HELLZA,
    Monster::WITCH_HELLZA_ENHANCED,
    Monster::WITCH_ILLZA,
    Monster::GHOST,
    Monster::LICH,
    Monster::LICH_ENHANCED,
    Monster::PHANTOM,
    Monster::CAVE_BAT,
    Monster::CAVE_BAT_ENHANCED,
    Monster::EVIL_BAT,
    Monster::EVIL_BAT_ENHANCED,
    Monster::NIKAPOUS,
    Monster::CRESCENT_BARON,
    Monster::CRESCENT_BARON_ENHANCED,
    Monster::MASK_OF_PRAJNA,
    Monster::MASK_OF_PRAJNA_ENHANCED,
    Monster::ALEXANDER,
    Monster::ALEXANDER_ENHANCED,
    Monster::ARTHUR,
    Monster::ARTHUR_ENHANCED,
    Monster::BLACK_KNIGHT,
    Monster::BLACK_KNIGHT_MOUNT,
    Monster::DARK_GENIE,
    Monster::DARK_GENIE_RIGHT_HAND,
    Monster::DARK_GENIE_FINAL,
    Monster::MINOTAUR_JOE,
    Monster::KINGS_CURSE,
    Monster::KINGS_CURSE_COFFIN,
    Monster::MASTER_UTAN,
    Monster::DRAN,
    Monster::GOL,
    Monster::GOL_ENHANCED,
    Monster::SIL,
    Monster::SIL_ENHANCED,
    Monster::BLIZZARD,
    Monster::STEEL_GIANT,
    Monster::STEEL_GIANT_ENHANCED,
    Monster::TITAN,
    Monster::TITAN_ENHANCED,
    Monster::GOLEM,
    Monster::BLACK_DRAGON,
    Monster::BLUE_DRAGON,
    Monster::DRAGON,
    Monster::OPAR,
};
constexpr int         kMark = 0x08000000; // in a hit's weapon flags: the cat's
constexpr int         kMonsters = 2;
constexpr int         kHitKind = 2;   // the reaction kind a pellet's entry carries
constexpr int         kGlowDelay = 4; // the torch flame's first draws are empty
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
    switch (SuperSteveSphere()) {
        case ITEM_WEAPON_DIVINEBEASTTITLE:
            return &kTitleLook;
        case ITEM_WEAPON_ANGEL_SHOOTER:
            return &kShooterCapeLook;
        default:
            return nullptr;
    }
}

// The element selected on Xiao's weapon (0 Fire to 4 Holy), else 5.
int Element() {
    return NowWeaponHave != nullptr && NowWeaponHave->best_elem >= 0 && NowWeaponHave->best_elem < 5 ? NowWeaponHave->best_elem : 5;
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
    bool             blocked = false;
    int              target = -1;
    bool             aimed = false;
    float            aim[4] = {0, 0, 0, 1};
    int              leap_frames = kSpiritBeastLeapFrames;
    int              retarget = 0;
    bool             spent = false; // a hit was taken
    bool             ended = false; // its pellet ended under it
    int              planted = -1;  // the hit waiting on the monster's CheckDmg
    int              planted_wait = 0;
    bool             taken = false;
    int              damage = 0;
    CSHOT           *shots = nullptr;
    int              slot = -1;
    const Look      *look = &kTitleLook;
};

Cat                 cat;
ShotCharge          charge{kSpiritBeastFrames};
CSHOT              *pending_shots = nullptr; // a charged pellet, bound once it has stepped
int                 pending_slot = -1;
CCharacter         *model = nullptr;
CFrame             *wings[2] = {};
bool                failed = false;
float               head[3] = {kHeadFallback[0], kHeadFallback[1], kHeadFallback[2]};
std::vector<CCPoly> polys(8192);
int                 poly_count = 0;

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

float Flat(const float *a, const float *b) {
    return std::hypot(a[0] - b[0], a[2] - b[2]);
}

// Turns the cat to the aim point where it is now.
void FaceAim() {
    float dx = cat.aim[0] - cat.pos[0], dz = cat.aim[2] - cat.pos[2], d = std::sqrt(dx * dx + dz * dz);
    if (cat.aimed && d > 0.0f) {
        cat.dir[0] = dx / d;
        cat.dir[1] = dz / d;
    }
}

// The floor under the cat near its height, the way a thrown item finds it, by the character's polygons: cast at its root,
// kProbeAhead ahead and kProbeBehind behind, the highest hit (so on stairs it rides its uphill end); unchanged over a pit.
// The polygons gathered stay for the wall probe.
void ProbeFloor() {
    poly_count = 0;
    if (NowDngMap == nullptr) {
        return;
    }
    poly_count = setCollisionData(NowDngMap, polys.data(), cat.pos, 20.0f, 1.5f);
    bool  found = false;
    float highest = 0.0f;
    for (float along : {0.0f, kProbeAhead, -kProbeBehind}) {
        float x = cat.pos[0] + cat.dir[0] * along, z = cat.pos[2] + cat.dir[1] * along;
        float from[4] = {x, cat.pos[1] + kProbeUp, z, 1.0f}, to[4] = {x, cat.pos[1] - kProbeDown, z, 1.0f}, hit[4];
        if (poly_count > 0 && CheckHit(polys.data(), poly_count, from, to, hit, 1, kCharacterSkip) >= 0 && (!found || hit[1] > highest)) {
            highest = hit[1];
            found = true;
        }
    }
    if (found) {
        cat.floor = highest;
    }
}

// Whether a wall stands within kWallFrames of walking along move.
bool WallAhead(const float *move) {
    if (poly_count <= 0) {
        return false;
    }
    float y = cat.floor + kWallHeight;
    float from[4] = {cat.pos[0], y, cat.pos[2], 1.0f}, to[4] = {cat.pos[0] + kWallFrames * move[0], y, cat.pos[2] + kWallFrames * move[1], 1.0f}, hit[4];
    return CheckHit(polys.data(), poly_count, from, to, hit, 1, kCharacterSkip) >= 0;
}

bool Live(int monster) {
    if (NowMonstorUnit == nullptr || monster < 0 || monster >= 16) {
        return false;
    }
    const MONSTOR &live = NowMonstorUnit->monster[monster];
    return live.state >= 1 && live.hp > 0;
}

// Whether the monster is a mimic not yet out of its chest and in play.
bool Dormant(int monster) {
    if (!Live(monster)) {
        return false;
    }
    const MONSTOR &live = NowMonstorUnit->monster[monster];
    return (live.kind == MONSTER_KIND_MIMIC_SMALL || live.kind == MONSTER_KIND_MIMIC_LARGE) && live.state != 2;
}

const CCharacter &Body(int monster) {
    return NowMonstorUnit->chara[monster][0];
}

// The percentage of the cat's hit a sphere takes: the one it names for the cat's kick, else Xiao's.
int CatPercent(const MONSTOR_EFFECT_STATE &effect, int sphere) {
    return effect.body_parameter[sphere][1] == kSpiritBeastSphereKick ? effect.body_parameter[sphere][0] : effect.parameter[sphere][CHARA_XIAO];
}

// Where the cat walks and leaps to: the centre of the monster's largest hurt sphere the cat's hit damages (else its largest
// in use), the furthest forward of equals; with none, or a monster not yet in play (a mimic in its chest), its feet.
void AimPoint(int monster, float *out) {
    const CCharacter           &body = Body(monster);
    const MONSTOR_EFFECT_STATE &effect = NowMonstorUnit->effect[monster];
    out[0] = body.pos[0], out[1] = body.pos[1], out[2] = body.pos[2], out[3] = 1.0f;
    if (NowMonstorUnit->monster[monster].state != 2) {
        return; // its spheres are not set up until it is in play
    }
    float fx = std::sin(body.rotation.y), fz = std::cos(body.rotation.y);
    float best = -1.0f, best_forward = -INFINITY;
    for (int pass = 0; pass < 2 && best < 0; pass++) {
        for (int sphere = 0; sphere < 16; sphere++) {
            if (effect.timer[sphere] == 0 || (pass == 0 && CatPercent(effect, sphere) <= 0)) {
                continue;
            }
            const float *c = effect.position[sphere];
            float        r = effect.radius[sphere], forward = (c[0] - body.pos[0]) * fx + (c[2] - body.pos[2]) * fz;
            if (r > best + 0.01f || (std::fabs(r - best) <= 0.01f && forward > best_forward)) {
                best = r;
                best_forward = forward;
                out[0] = c[0], out[1] = c[1], out[2] = c[2];
            }
        }
    }
}

// Whether the monster gets the higher arc: a flyer, hoverer or tall body, a miniboss, or one drawn large.
bool TallLeap(int monster) {
    for (Monster kind : kTallLeapers) {
        if (IsKind(*NowMonstorUnit, monster, kind)) {
            return true;
        }
    }
    return IsMiniboss(monster) || Body(monster).scale[0] >= kTallScale;
}

// How near the cat crouches by its target: its look's reach, but kSpiritBeastPounce by a mimic still in its chest.
float Range() {
    return Dormant(cat.target) ? kSpiritBeastPounce : cat.look->range;
}

// Whether the target cannot be hit: a mimic in its chest, or a monster in its untouchable frames.
bool Hold() {
    return cat.target >= 0 && (Dormant(cat.target) || NowMonstorUnit->monster[cat.target].invincible_timer > 0);
}

// The monster she has locked on to, or would lock on to, if alive.
int LockedTarget() {
    return Live(lockOnTargetNo) ? lockOnTargetNo : -1;
}

// That monster, else the nearest alive to her within sight, or -1.
int PickTarget() {
    if (NowMonstorUnit == nullptr) {
        return -1;
    }
    int locked = LockedTarget();
    if (locked >= 0) {
        return locked;
    }
    int   best = -1;
    float nearest = INFINITY;
    for (int monster = 0; monster < 16; monster++) {
        float d = Flat(Body(monster).pos, CharaMain.pos);
        if (Live(monster) && d < nearest && d <= kSpiritBeastSight) {
            best = monster;
            nearest = d;
        }
    }
    return best;
}

void Aim() {
    if (cat.target >= 0 && Live(cat.target)) {
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

// What the mod's thread does around the cave each frame: the aim follows the target, a lock-on made since is taken (from
// the crouch, back to walking), and walking or sitting it finds a new target when it has none.
void Watch() {
    bool walking = cat.phase == SpiritBeastPhase::kWalking || cat.phase == SpiritBeastPhase::kSitting;
    if (cat.target >= 0 && !Live(cat.target)) {
        if (walking) {
            Target(PickTarget());
        } else {
            cat.target = -1; // the pounce goes on to where it last was
        }
    }
    Aim();
    if (cat.target >= 0 && (walking || cat.phase == SpiritBeastPhase::kReady) && ++cat.retarget >= kSpiritBeastRetarget) {
        cat.retarget = 0;
        int locked = LockedTarget();
        if (locked >= 0 && locked != cat.target) {
            Target(locked);
            if (cat.phase == SpiritBeastPhase::kReady) {
                Flags(0, kPlayOnce);
                Phase(SpiritBeastPhase::kWalking);
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
    CFrame *kao = model != nullptr && model->frame != nullptr ? model->frame->SearchFrame(const_cast<char *>(kHeadFrame)) : nullptr;
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
        return checkCollision(out, point, move, kMonsters, kSpiritBeastTouch);
    }
    for (int m = 0; m < 16; m++) { // with no map, its monster test alone
        const MONSTOR &live = NowMonstorUnit->monster[m];
        for (int sphere = 0; live.state != -1 && live.revealed != 0 && sphere < 16; sphere++) {
            const MONSTOR_EFFECT_STATE &effect = NowMonstorUnit->effect[m];
            if (effect.timer[sphere] != 0 && DistVector(const_cast<float *>(effect.position[sphere]), point) <= kSpiritBeastTouch + effect.radius[sphere]) {
                return SHOT_COLLISION_MONSTER;
            }
        }
    }
    return SHOT_COLLISION_NONE;
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
    int element = NowWeaponHave->best_elem;
    int attribute = element >= 0 && element < 5 && NowWeaponHave->elem[element] > 0 ? GetWeaponElementAttr(element) : 0;
    int hit = NowColData->Set(point, std::max(1, cat.damage), 1, kSpiritBeastHitRadius, 0.0f, kMonsters, kHitKind, attribute, 0);
    if (hit < 0) {
        return;
    }
    COLLISION_HIT &record = NowColData->hit[hit];
    record.owner = CHARA_XIAO;
    record.attack_no = 0;
    record.weapon_flags = NowWeaponHave->flags | kMark;
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
    Phase(SpiritBeastPhase::kLanding);
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
                cat.vel[0] = (cat.aim[0] - cat.pos[0]) / t;
                cat.vel[2] = (cat.aim[2] - cat.pos[2]) / t;
                FaceAim();
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
        Phase(SpiritBeastPhase::kWalking);
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
                cat.vel[0] = (cat.aim[0] - cat.pos[0]) / t;
                cat.vel[2] = (cat.aim[2] - cat.pos[2]) / t;
                FaceAim();
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
        cat.blocked = false;
        Phase(SpiritBeastPhase::kSitting);
        return;
    }
    Phase(SpiritBeastPhase::kWalking);
    float dx = cat.aim[0] - cat.pos[0], dz = cat.aim[2] - cat.pos[2], d = std::sqrt(dx * dx + dz * dz);
    if (d > 0.0f) {
        cat.dir[0] = dx / d;
        cat.dir[1] = dz / d;
        if (!(Range() < d)) {
            Flags(kPlayOnce);
            Key(kCatReady);
            Rate(-1.0f);
            cat.last_frame = 0.0f;
            Phase(SpiritBeastPhase::kReady);
            return;
        }
    }
    float walk = kSpiritBeastWalk * cat.look->scale, move[2] = {cat.dir[0] * walk, cat.dir[1] * walk};
    cat.blocked = WallAhead(move);
    if (cat.blocked) {
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
        Phase(SpiritBeastPhase::kWindUp);
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
        Phase(SpiritBeastPhase::kWalking);
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
    Phase(SpiritBeastPhase::kLeaping);
}

// On the pellet: the head on its point, the cat grown by the frames before this one; at full size it breaks away.
void Riding() {
    if (cat.shots == nullptr || cat.shots->used[cat.slot] == 0) {
        cat.ended = true; // full size where it was, fading
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
    Phase(SpiritBeastPhase::kFalling);
}

void Place() {
    if (model == nullptr || model->frame == nullptr) {
        return;
    }
    if (cat.phase != SpiritBeastPhase::kRiding && cat.dir[0] * cat.dir[0] + cat.dir[1] * cat.dir[1] > 1e-6f) {
        cat.yaw = std::atan2(cat.dir[0], cat.dir[1]);
    }
    float shown = cat.fade < 0 ? 1.0f : 1.0f - static_cast<float>(cat.fade) / kSpiritBeastFadeFrames;
    model->SetPosition(cat.pos[0], cat.pos[1], cat.pos[2]);
    model->SetRotation(0.0f, cat.yaw, 0.0f);
    model->SetScale(cat.scale, cat.scale, cat.scale);
    model->ambient_offset[3] = 128.0f * std::max(0.0f, shown);
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
    CFrame *kao = model != nullptr && model->frame != nullptr ? model->frame->SearchFrame(const_cast<char *>(kHeadFrame)) : nullptr;
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
    Phase(SpiritBeastPhase::kRiding);
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
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && BtEventMode == 0 && LookNow() != nullptr;
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
                wings[i] = model->frame != nullptr ? model->frame->SearchFrame(const_cast<char *>(kWingFrames[i])) : nullptr;
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
                ProbeFloor();
                Falling();
                break;
            case SpiritBeastPhase::kLanding:
                ProbeFloor();
                if (Landing()) {
                    Walking();
                }
                break;
            case SpiritBeastPhase::kWalking:
            case SpiritBeastPhase::kSitting:
                ProbeFloor();
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
    if (slot < 0 || slot >= 12 || !SpiritBeastActive() || !charge.TakeCharged() || model == nullptr) {
        return;
    }
    pending_shots = &shots;
    pending_slot = slot;
}

void SpiritBeastDraw() {
    if (model == nullptr || model->frame == nullptr || cat.phase == SpiritBeastPhase::kNone) {
        return;
    }
    TexManager.ReloadTexture(Vif1Packet, kFloorCharacterBlock);
    model->Draw();
    float shown = cat.fade < 0 ? 1.0f : 1.0f - static_cast<float>(cat.fade) / kSpiritBeastFadeFrames;
    if (cat.look->cape) {
        SpiritCapeDraw(*model, Element(), std::max(0.0f, shown));
    }
    if (cat.age <= kGlowDelay) {
        return;
    }
    float   middle[4] = {cat.pos[0], cat.pos[1] + kRootTouch, cat.pos[2], 1.0f};
    CFrame *hips = model->frame->SearchFrame(const_cast<char *>(kHipsFrame));
    CFrame *spine = model->frame->SearchFrame(const_cast<char *>(kSpineFrame));
    if (hips != nullptr && spine != nullptr) {
        sceVu0FVECTOR origin = {0.0f, 0.0f, 0.0f, 1.0f}, a, b;
        hips->GetWorldPosition(a, origin);
        spine->GetWorldPosition(b, origin);
        for (int i = 0; i < 3; i++) {
            middle[i] = 0.5f * (a[i] + b[i]);
        }
    }
    DrawGlowDisc(cat.look->cape ? kElementGlows[Element()] : cat.look->glow, middle, kGlowSize * cat.look->scale * std::max(0.0f, shown), kGlowPull);
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
    return (hit.weapon_flags & kMark) != 0;
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
