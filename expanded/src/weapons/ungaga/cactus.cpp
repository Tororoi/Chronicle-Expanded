#include "expanded/weapons/ungaga/cactus.hpp"
#include "sound_effect.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/floor_height.hpp"
#include "expanded/dungeon/ground_shadow.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/summon_smoke.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/ungaga/cactus/bloom_prop.hpp"
#include "expanded/weapons/weapon_hit.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

using namespace cactus;

// The rock, metal and undead kinds: every kind sharing their name.
constexpr Monster kDryKinds[] = {
    Monster::MASTER_JACKET,
    Monster::SKELETON_SOLDIER,
    Monster::STATUE,
    Monster::PIRATES_CHARIOT,
    Monster::GOLEM,
    Monster::MR_BLARE,
    Monster::DUNE,
    Monster::TITAN,
    Monster::ARTHUR,
    Monster::LIVING_ARMOR,
    Monster::STEEL_GIANT,
    Monster::BILLY,
    Monster::VULCAN,
    Monster::ROCKANOFF,
    Monster::GOL,
    Monster::SIL,
    Monster::STATUE_DOG,
    Monster::GACIOUS,
    Monster::SILVER_GEAR,
    Monster::HORN_HEAD,
    Monster::ARTHUR_ENHANCED,
    Monster::SIL_ENHANCED,
    Monster::MASTER_JACKET_ENHANCED,
    Monster::VULCAN_ENHANCED,
    Monster::ROCKANOFF_ENHANCED,
    Monster::STEEL_GIANT_ENHANCED,
    Monster::GOL_ENHANCED,
    Monster::PIRATES_CHARIOT_ENHANCED,
    Monster::GACIOUS_ENHANCED,
    Monster::STATUE_DOG_ENHANCED,
    Monster::TITAN_ENHANCED,
    Monster::LIVING_ARMOR_ENHANCED,
};

constexpr int kKickAway = 2;    // thrown away from the hit's origin
constexpr int kHitReserve = 16; // free hit records a prick leaves the game

// The smoke's frames as game frames from the summon.
constexpr float kEmergeFrames = (kBloomEmergedFrame - kSmokeClipStart) / kSmokeRate;
constexpr float kPeakFrames = (kBloomPeakFrame - kSmokeClipStart) / kSmokeRate;
constexpr float kRisenFrames = (kBloomRisenFrame - kSmokeClipStart) / kSmokeRate;

// The body test's reach: a sphere farther than this from its monster has not been posed this frame; a monster with none
// is its root, this wide.
constexpr float kUnposed = 80.0f;
constexpr float kBodyFallback = 10.0f;

// The floor under the spot: the highest at or below the wielder's height plus this.
constexpr float kFloorAbove = 20.0f;

struct Form {
    BloomModel model;
    BloomShape shape;
    bool       pricks;
};

const Form kCactusForm = {
    {"cactus", true, nullptr, nullptr},
    kBloomCactus,
    true
};
const Form kTreesForm = {
    {"Queens' trees", false, kQueensTreesFile, kQueensTreesCfg},
    kBloomTrees,
    false
};

struct Bloom {
    int         chara = -1; // whose it is (-1: no wielder)
    const Form *form = nullptr;
    GuardHold   guard; // latched once it raises one, until R1 is let go
    bool        up = false;
    int         age = 0;
    float       spot[3] = {};      // x, ground, z
    float       yaw = 0.0f;        // the wielder's facing at the summon
    float       rise = 0.0f;       // the root's height while it emerges
    float       rise_speed = 0.0f; // how far it sinks this frame (below 0: it climbs)
    float       rise_gain = 0.0f;  // and what that gains each frame
    bool        emerged = false, peaked = false, risen = false;
    float       scale = 0.0f;
    float       root[3] = {};
    float       alpha = 1.0f;
    int         last_prick = -1;
};

Bloom bloom;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(DesertBloomReset), true);

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_CACTUS, WeaponShotForm::Weapon, SummonSmokeEffect(), kSmokeShots, kSummonSmokeFile),
     RegisterWeaponShotEffect(ITEM_WEAPON_CACTUS, WeaponShotForm::Sphere, SummonSmokeEffect(), kSmokeShots, kSummonSmokeFile), true);

const BloomShape &Shape() {
    return bloom.form->shape;
}

float HerYaw() {
    float rotation[4];
    CharaMain.GetRotation(rotation);
    return rotation[1];
}

// The copy's turn: the wielder's facing at the summon and the form's own, within a half-turn either way.
float CopyYaw() {
    float yaw = bloom.yaw + Shape().turn;
    while (yaw > std::numbers::pi_v<float>) {
        yaw -= 2.0f * std::numbers::pi_v<float>;
    }
    while (yaw < -std::numbers::pi_v<float>) {
        yaw += 2.0f * std::numbers::pi_v<float>;
    }
    return yaw;
}

// The root's height at scale s, t of the way out (0: its top kBloomBuried under the floor; 1: its exposed part out, the
// same part at any size).
float RootHeight(float t, float s) {
    return bloom.spot[1] - Shape().top * s + (-kBloomBuried + (Shape().exposed * s + kBloomBuried) * t);
}

// The root across the ground at scale s: the form's centre, turned with the copy, on the spot.
void RootAcross(float s, float *root) {
    float c = std::cos(CopyYaw()), n = std::sin(CopyYaw());
    root[0] = bloom.spot[0] - (c * Shape().base_x + n * Shape().base_z) * s;
    root[2] = bloom.spot[2] - (-n * Shape().base_x + c * Shape().base_z) * s;
}

// How far (x, z) is across the ground from the nearest edge of a monster's posed body spheres (0 inside one); with none,
// from its root less kBodyFallback.
float BodyEdge(CMonstorUnit &unit, int monster, float x, float z) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       at[4];
    unit.chara[monster][0].GetPosition(at);
    bool  posed = false;
    float nearest = 0.0f;
    for (int i = 0; i < kMonsterSpheres; i++) {
        const float *centre = body.position[i];
        if (body.timer[i] == 0 || std::fabs(centre[0] - at[0]) > kUnposed || std::fabs(centre[2] - at[2]) > kUnposed) {
            continue;
        }
        float edge = std::hypot(centre[0] - x, centre[2] - z) - body.radius[i];
        nearest = posed ? std::min(nearest, edge) : edge;
        posed = true;
    }
    if (!posed) {
        nearest = std::hypot(at[0] - x, at[2] - z) - kBodyFallback;
    }
    return std::max(0.0f, nearest);
}

// A monster's largest posed body sphere (its centre into centre, its radius); its root and kBodyFallback with none.
float BodyCentre(CMonstorUnit &unit, int monster, float *centre) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       at[4];
    unit.chara[monster][0].GetPosition(at);
    centre[0] = at[0], centre[1] = at[1] + kBodyFallback, centre[2] = at[2], centre[3] = 1.0f;
    float radius = kBodyFallback, best = 0.0f;
    for (int i = 0; i < kMonsterSpheres; i++) {
        const float *sphere = body.position[i];
        if (body.timer[i] == 0 || body.radius[i] <= best || std::fabs(sphere[0] - at[0]) > kUnposed || std::fabs(sphere[2] - at[2]) > kUnposed) {
            continue;
        }
        best = radius = body.radius[i];
        std::copy(sphere, sphere + 3, centre);
    }
    return radius;
}

int FreeHits() {
    int free = 0;
    for (int active : NowColData->active) {
        free += active == 0 ? 1 : 0;
    }
    return free;
}

// One prick on a monster's body: the weapon's own hit at a share of its attack, thrown away from the cactus.
void Prick(CMonstorUnit &unit, int monster) {
    if (NowColData == nullptr || NowWeaponHave == nullptr || FreeHits() <= kHitReserve) {
        return;
    }
    float          centre[4];
    float          radius = std::max(kBloomHitRadius, BodyCentre(unit, monster, centre));
    int            damage = std::max(1, static_cast<int>(std::nearbyint(static_cast<double>(NowWeaponHave->attack * kBloomHitShare))));
    COLLISION_HIT *record = PlantWeaponHit(centre, damage, 1, radius);
    if (record == nullptr) {
        return;
    }
    record->weapon_flags |= kPrickMark;
    float origin[4] = {bloom.spot[0], bloom.spot[1], bloom.spot[2], 1.0f};
    NowColData->SetKickBack(origin, kBloomKick.speed, kBloomKick.decay, kKickAway);
}

void Pricks() {
    if (NowMonstorUnit == nullptr) {
        return;
    }
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (MonsterLiving(NowMonstorUnit->monster[monster]) &&
            BodyEdge(*NowMonstorUnit, monster, bloom.spot[0], bloom.spot[2]) <= Shape().block_radius + kBloomHitReach) {
            Prick(*NowMonstorUnit, monster);
        }
    }
}

void TakeDown() {
    SummonSmokeEnd(&bloom);
    if (!bloom.up) {
        return;
    }
    SolidColumnDisarm(&bloom);
    GroundShadowHide(&bloom);
    PropDown();
    bloom.up = false;
    bloom.emerged = bloom.peaked = bloom.risen = false;
}

void Summon() {
    TakeDown(); // one standing moves
    bloom.yaw = HerYaw();
    bloom.spot[0] = CharaMain.pos[0] + std::sin(bloom.yaw) * kBloomAhead;
    bloom.spot[2] = CharaMain.pos[2] + std::cos(bloom.yaw) * kBloomAhead;
    if (!FloorUnder(bloom.spot[0], bloom.spot[2], CharaMain.pos[1] + kFloorAbove, &bloom.spot[1])) {
        bloom.spot[1] = CharaMain.pos[1]; // the wielder's own floor
    }
    if (!PropSpawn(bloom.form->model)) {
        return;
    }
    bloom.up = true;
    bloom.age = -1; // its first step, this frame, is its frame 0
    bloom.emerged = bloom.peaked = bloom.risen = false;
    bloom.last_prick = -1;
    bloom.alpha = 1.0f;
    bloom.scale = Shape().scale * kBloomSpawnShare;
    // The rise out of the floor at the spawn size: up at 2D/T, slowing by 1/T of that a frame, so it settles as it is out.
    float from = RootHeight(0.0f, bloom.scale), to = RootHeight(1.0f, bloom.scale);
    float speed = 2.0f * (to - from) / kEmergeFrames;
    bloom.rise = from;
    bloom.rise_speed = -speed;
    bloom.rise_gain = speed / kEmergeFrames;
    RootAcross(bloom.scale, bloom.root);
    bloom.root[1] = from;
    PropPlace(bloom.root, CopyYaw(), bloom.scale, bloom.alpha);
    SummonSmokeStart(bloom.spot, bloom.yaw, kBloomSmokeScale, &bloom);
    ChargeCompleteFlash();
    SndSePlay(SE_BOX_OPEN, -1, 0); // the poof
}

bool ControllerGuard() {
    return GamePad.On(PAD_R1) != 0;
}

GuardButtonHeld guard_button = ControllerGuard;

// The guard pose held long enough raises one; R1 let go, the next hold may.
void Charge() {
    bloom.guard.Step(guard_button());
    if (bloom.guard.Reached(kBloomGuardFrames)) {
        bloom.guard.Latch();
        Summon();
    }
}

// Its growth once out, evenly in time: to the peak (shown at least one frame), then back to its size.
void Grow() {
    float age = static_cast<float>(bloom.age);
    float peak = Shape().scale * kBloomPeakShare, spawn = Shape().scale * kBloomSpawnShare;
    if (age < kPeakFrames || !bloom.peaked) {
        float u = std::clamp((age - kEmergeFrames) / (kPeakFrames - kEmergeFrames), 0.0f, 1.0f);
        bloom.scale = spawn + (peak - spawn) * u;
        bloom.peaked = bloom.peaked || u >= 1.0f;
    } else {
        float u = std::clamp((age - kPeakFrames) / (kRisenFrames - kPeakFrames), 0.0f, 1.0f);
        bloom.scale = peak + (Shape().scale - peak) * u;
        bloom.risen = bloom.risen || u >= 1.0f;
    }
    RootAcross(bloom.scale, bloom.root);
    bloom.root[1] = RootHeight(1.0f, bloom.scale);
    if (bloom.risen) {
        // Settled: solid, up to the top of what stands out.
        SolidColumnArm(bloom.spot, Shape().block_radius, bloom.spot[1] + Shape().exposed * Shape().scale, &bloom);
    }
}

void Drive() {
    bloom.age++;
    if (bloom.age >= kBloomStandFrames + kBloomFadeFrames) {
        TakeDown();
        return;
    }
    SummonSmokeStep();
    if (!bloom.emerged && static_cast<float>(bloom.age) >= kEmergeFrames) {
        bloom.emerged = true;
    }
    if (!bloom.emerged && bloom.age > 0) {
        bloom.rise -= bloom.rise_speed;
        bloom.rise_speed += bloom.rise_gain;
        bloom.root[1] = bloom.rise;
    } else if (bloom.emerged && !bloom.risen) {
        Grow();
    }
    bloom.alpha = bloom.age < kBloomStandFrames ? 1.0f : std::max(0.0f, 1.0f - static_cast<float>(bloom.age - kBloomStandFrames) / kBloomFadeFrames);
    PropPlace(bloom.root, CopyYaw(), bloom.scale, bloom.alpha);
    // Its shadow, gone as it starts to fade.
    if (bloom.age < kBloomStandFrames && Shape().shadow_radius > 0.0f) {
        GroundShadowShow(bloom.spot[0], bloom.spot[1], bloom.spot[2], Shape().shadow_radius * bloom.scale, &bloom);
    } else {
        GroundShadowHide(&bloom);
    }
    if (bloom.risen && bloom.form->pricks && (bloom.last_prick < 0 || bloom.age - bloom.last_prick >= kBloomHitFrames)) {
        bloom.last_prick = bloom.age;
        Pricks();
    }
}

void End() {
    TakeDown();
    bloom = Bloom{};
}

} // namespace

bool AbsorbActive() {
    return UserStatus != nullptr && WeaponActive(ITEM_WEAPON_CACTUS);
}

bool CactusDryKind(int kind) {
    return std::any_of(std::begin(kDryKinds), std::end(kDryKinds), [kind](Monster dry) { return No(dry) == kind; });
}

void CactusMonsterDamaged(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    if (!AbsorbActive() || hit.owner != UserStatus->cur_chara) {
        return;
    }
    // The life it took: its life now is past the hit, under 0 for one that took more than it had.
    int taken = amount + std::min(0, unit.monster[monster].hp);
    if (taken <= 0 || CactusDryKind(ModelKind(unit.monster[monster].base_model))) {
        return;
    }
    int    chara = UserStatus->cur_chara;
    float &water = UserStatus->water_now[chara];
    float  most = UserStatus->water_max[chara];
    if (most > 0.0f && water >= most) {
        return;
    }
    float gained = static_cast<float>(taken) / kAbsorbDamagePerWater;
    water = most > 0.0f ? std::min(water + gained, most) : water + gained;
}

bool DesertBloomActive() {
    return UserStatus != nullptr && NowMonstorUnit != nullptr && (UserStatus->cur_chara == CHARA_UNGAGA || UserStatus->cur_chara == CHARA_XIAO) &&
           WeaponActive(ITEM_WEAPON_CACTUS);
}

bool DesertBloomUp() {
    return bloom.up;
}

int DesertBloomAge() {
    return bloom.up ? bloom.age : -1;
}

float DesertBloomScale() {
    return bloom.up ? bloom.scale : 0.0f;
}

void DesertBloomSpot(float *spot) {
    std::copy(bloom.spot, bloom.spot + 3, spot);
}

float DesertBloomAlpha() {
    return bloom.up ? bloom.alpha : 0.0f;
}

bool DesertBloomPrick(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kPrickMark) != 0;
}

void DesertBloomStep() {
    bool active = DesertBloomActive();
    if (bloom.chara >= 0 && (!active || UserStatus->cur_chara != bloom.chara)) {
        End(); // a change of wielder ends it: the next one's starts afresh
    }
    if (!active) {
        return;
    }
    if (bloom.chara < 0) {
        bloom.chara = UserStatus->cur_chara;
        bloom.form = bloom.chara == CHARA_XIAO ? &kTreesForm : &kCactusForm;
    }
    Charge();
    if (bloom.up) {
        Drive();
    }
}

void DesertBloomDraw() {
    if (bloom.up && BtEventMode == 0) {
        PropDraw();
    }
}

void SetDesertBloomGuardButton(GuardButtonHeld held) {
    guard_button = held != nullptr ? held : ControllerGuard;
}

void DesertBloomReset() {
    SummonSmokeEnd(&bloom);
    SolidColumnDisarm(&bloom);
    GroundShadowHide(&bloom);
    PropReset();
    bloom = Bloom{};
}

} // namespace expanded
