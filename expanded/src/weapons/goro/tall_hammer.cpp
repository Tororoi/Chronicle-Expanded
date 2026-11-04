#include "expanded/weapons/goro/tall_hammer.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <iterator>
#include <vector>

#include "bound.hpp"
#include "btactstatus.hpp"
#include "camerafollow.hpp"
#include "character.hpp"
#include "cloth.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/hit_marks.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/summon_smoke.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/weapon_hit.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// Goro's charge (GoroKey_Play): the held charge, full from its step kFullCharge.
constexpr int kChargeAction = 13;
constexpr int kFullCharge = 45;

// The pop's smoke frames as game frames from its start.
constexpr float kPopFrom = (kColossusPopFrom - kSmokeClipStart) / kSmokeRate;
constexpr float kPopPeak = (kColossusPopPeakFrame - kSmokeClipStart) / kSmokeRate;
constexpr float kPopEnd = (kColossusPopFrame - kSmokeClipStart) / kSmokeRate;

// The motions his stride plays in: the run (1), the walk (2), the strafes while locked on (0x12-0x16), the walk while
// charging (0x1E) and the walk while guarding (0x21).
constexpr int kStrideMotions[] = {1, 2, 0x12, 0x13, 0x14, 0x15, 0x16, 0x1E, 0x21};

// His hammer's swing throw (GoroKey_Play).
constexpr float kGoroKick = 2.5f, kGoroKickDecay = 0.1f;

constexpr float kWalking = 0.05f;          // how far a step must take him to be walking
constexpr float kTrampleReach = 2.0f;      // past where the monster stops against him
constexpr float kSwitchBelow = 5.0f;       // how far under his feet a switch he stands on may lie
constexpr float kColossusHeightOf = 16.0f; // Goro's height (CharaHeight)

// A monster's melee hit's owner (CMonstorUnit: 200 + its slot times 5), its attack_no the sphere that dealt it.
constexpr int kMonsterOwner = 200, kMonsterOwnerStride = 5;

// How long a shot's hit record stays its (CSHOT_EFFECT::Step sets it for 2 frames).
constexpr int kShotHitLife = 2;

// An attack that has hit him at his full size, whose hits pass him by while it goes on: a monster's sphere while its
// hit window stays open (time: how far its motion had got), a shot or, for anything else, the hit record, until a
// step of his clock.
struct Spared {
    int                 monster = -1;
    int                 sphere = -1;
    float               time = 0.0f;
    const CSHOT_EFFECT *effect = nullptr;
    int                 slot = -1;
    int                 record = -1;
    int                 until = 0;
};

// The shot that set a hit record, and when.
struct ShotHit {
    CSHOT_EFFECT *effect = nullptr;
    int           slot = -1;
    int           at = -1;
};

// A cloth's ties (to its neighbours across and down) and wind at his own size, and a bound's extents.
struct CapeBase {
    CCloth *cloth = nullptr;
    float   rest[16][16][2] = {};
    float   wind = 0.0f;
};

struct BoundBase {
    CBound *bound = nullptr;
    float   extent[3] = {};
};

enum class Phase {
    Normal,
    Grown,
    Shrinking,
    Spent,
    Popping
};

struct Colossus {
    Phase                                             phase = Phase::Normal;
    float                                             scale = 1.0f;
    int                                               grown = 0;                  // frames he has grown for (Grown)
    int                                               frames = 0;                 // in the phase (Grown: since he first grew; Spent: since the shrink began)
    float                                             shrink_from = 1.0f;         // his size as the shrink began
    float                                             applied = 1.0f;             // the size last put on the model
    float                                             camera = 1.0f;              // the size the camera's distance and height were last kept at
    CCloth                                           *cape[CHARA_CLOTH_MAX] = {}; // the cloth he wore when it was last sized
    float                                             cape_size = 1.0f;           // the size its ties and bounds were last kept at
    std::vector<CapeBase>                             cape_base;                  // its ties and wind at his own size
    std::vector<BoundBase>                            bound_base;                 // the bounds it drapes over, at his own size
    float                                             stride = -1.0f;             // the play rate last put on his walk, or -1
    float                                             last[2] = {};               // where he stood the step before
    bool                                              stood = false;
    int                                               trampled[kFloorMonsters] = {};
    int                                               clock = 0;
    bool                                              shrugging = false; // a hit he shrugs off is being reacted to
    BT_ACT_STATUS                                     kept = {};         // what he was doing as it reached him
    std::vector<Spared>                               spared;
    ShotHit                                           shot_hits[std::size(CCollisionData{}.active)];
    std::vector<std::pair<const CSHOT_EFFECT *, int>> stopping;         // shots that meet him as a wall on their next step
    CSHOT_EFFECT                                     *impact = nullptr; // his charge attack's ground impact, at impact_size
    int                                               impact_slot = -1;
    float                                             impact_size = 1.0f;
};

Colossus colossus;

const char *PhaseName(Phase phase) {
    switch (phase) {
        case Phase::Normal:
            return "normal";
        case Phase::Grown:
            return "grown";
        case Phase::Shrinking:
            return "shrinking";
        case Phase::Spent:
            return "spent";
        case Phase::Popping:
            return "popping";
    }
    return "";
}

void Enter(Phase phase) {
    colossus.phase = phase;
    LogEvent("colossus", std::format("\"phase\":\"{}\",\"scale\":{:.3f}", PhaseName(phase), colossus.scale));
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(ColossusReset), true);

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_TALL_HAMMER, WeaponShotForm::Weapon, SummonSmokeEffect(), kSmokeShots, kSummonSmokeFile), true);

bool GoroInPlay() {
    return UserStatus != nullptr && Playing(CHARA_GORO);
}

// The size's effects reach Goro in play at any size but his own.
bool Sized() {
    return GoroInPlay() && colossus.scale != 1.0f;
}

bool HoldingFullCharge() {
    return BtActStatus.action_on == 1 && BtActStatus.action_no == kChargeAction && BtActStatus.action_step >= kFullCharge;
}

void Grow() {
    if (colossus.scale >= kColossusMost || !WeaponWielded(ITEM_WEAPON_TALL_HAMMER) || !HoldingFullCharge()) {
        return;
    }
    if (colossus.phase == Phase::Normal) {
        Enter(Phase::Grown);
        colossus.frames = 0;
    }
    colossus.grown = std::min(colossus.grown + 1, kColossusGrowFrames);
    if (colossus.grown == kColossusGrowFrames) {
        LogEvent("colossus", "\"phase\":\"most\"");
    }
    colossus.scale = 1.0f + (kColossusMost - 1.0f) * static_cast<float>(colossus.grown) / kColossusGrowFrames;
}

// A hammer hit on every monster he walks into at his full size, each once every kColossusTrampleFrames.
void Trample() {
    float at[2] = {CharaMain.pos[0], CharaMain.pos[2]};
    bool  walking = colossus.stood && std::hypot(at[0] - colossus.last[0], at[1] - colossus.last[1]) > kWalking;
    colossus.last[0] = at[0], colossus.last[1] = at[1];
    colossus.stood = true;
    if (!walking || colossus.scale < kColossusMost || NowMonstorUnit == nullptr || NowColData == nullptr || NowWeaponHave == nullptr) {
        return;
    }
    CMonstorUnit &unit = *NowMonstorUnit;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        const MONSTOR &body = unit.monster[monster];
        if (!MonsterLiving(body) || colossus.clock - colossus.trampled[monster] < kColossusTrampleFrames) {
            continue;
        }
        float there[4];
        unit.chara[monster][0].GetPosition(there);
        float reach = ColossusBlockRadius(kColossusBodyRadius) + body.body_radius + kTrampleReach;
        if (std::hypot(there[0] - at[0], there[2] - at[1]) > reach) {
            continue;
        }
        float          centre[4];
        float          radius = MonsterBodyCentre(unit, monster, centre);
        int            damage = std::max(1, static_cast<int>(std::lround(ColossusAttack(NowWeaponHave->attack) * kColossusTrampleShare)));
        COLLISION_HIT *hit = PlantWeaponHit(centre, damage, 1, radius);
        if (hit == nullptr) {
            return;
        }
        hit->weapon_flags |= kTrampleMark;
        colossus.trampled[monster] = colossus.clock;
        LogEvent("colossus", std::format("\"trample\":{}", monster));
        float origin[4] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2], 1.0f};
        NowColData->SetKickBack(origin, ColossusKickSpeed(kGoroKick), kGoroKickDecay, kKnockbackFromOrigin);
    }
}

void Pop() {
    Enter(Phase::Popping);
    colossus.frames = 0;
    float spot[3] = {CharaMain.pos[0], CharaMain.pos[1], CharaMain.pos[2]};
    SummonSmokeStart(spot, PlayerYaw(), kColossusPopSmokeScale, &colossus, kColossusSmokeTint);
}

// The pop's size: small in the smoke, then past his size and back to it, evenly in time.
float PopScale(float age) {
    if (age < kPopFrom) {
        return kColossusLeast;
    }
    if (age < kPopPeak) {
        return kColossusLeast + (kColossusPopPeak - kColossusLeast) * (age - kPopFrom) / (kPopPeak - kPopFrom);
    }
    return kColossusPopPeak + (1.0f - kColossusPopPeak) * std::clamp((age - kPopPeak) / (kPopEnd - kPopPeak), 0.0f, 1.0f);
}

void Advance() {
    colossus.frames++;
    switch (colossus.phase) {
        case Phase::Normal:
            break;
        case Phase::Grown:
            if (colossus.frames >= kColossusFrames) {
                Enter(Phase::Shrinking);
                colossus.shrink_from = colossus.scale;
                colossus.frames = 0;
            }
            break;
        case Phase::Shrinking: {
            float t = std::min(1.0f, static_cast<float>(colossus.frames) / kColossusShrinkFrames);
            colossus.scale = colossus.shrink_from + (kColossusLeast - colossus.shrink_from) * t;
            if (colossus.frames >= kColossusShrinkFrames) {
                Enter(Phase::Spent); // its frames run on from the shrink's start
            }
            break;
        }
        case Phase::Spent:
            if (colossus.frames >= kColossusSpentFrames) {
                Pop();
            }
            break;
        case Phase::Popping:
            SummonSmokeStep();
            colossus.scale = PopScale(static_cast<float>(colossus.frames));
            if (static_cast<float>(colossus.frames) >= kPopEnd) {
                colossus.scale = 1.0f;
                Enter(Phase::Normal);
                colossus.grown = 0;
            }
            break;
    }
}

// His cape at his size. Its ties and the bounds it drapes over were measured once, in his own units, and its pinned
// edge and the sheet it pulls toward come through his scaled frame (the Spirit Cape's sizing): ties, bounds and wind
// are his size times what they were at his own, read when he first wore it. A cloth he did not wear when it was last
// sized (another member's, a reload) is read afresh.
void SizeCape(float scale) {
    if (!std::equal(std::begin(colossus.cape), std::end(colossus.cape), CharaMain.cloth, CharaMain.cloth + CHARA_CLOTH_MAX)) {
        std::copy(CharaMain.cloth, CharaMain.cloth + CHARA_CLOTH_MAX, colossus.cape);
        colossus.cape_size = 1.0f;
        colossus.cape_base.clear();
        colossus.bound_base.clear();
        for (CCloth *cloth : colossus.cape) {
            if (cloth == nullptr) {
                continue;
            }
            CapeBase &base = colossus.cape_base.emplace_back();
            base.cloth = cloth;
            base.wind = cloth->wind_effect;
            for (int i = 0; i < cloth->num_i; i++) {
                for (int j = 0; j < cloth->num_j; j++) {
                    base.rest[i][j][0] = cloth->rest[i][j][0];
                    base.rest[i][j][1] = cloth->rest[i][j][1];
                }
            }
            for (CBound *bound = cloth->bound; bound != nullptr; bound = bound->next) {
                auto seen = [bound](const BoundBase &other) { return other.bound == bound; };
                if (std::find_if(colossus.bound_base.begin(), colossus.bound_base.end(), seen) != colossus.bound_base.end()) {
                    break; // a tail another piece shares
                }
                colossus.bound_base.push_back({
                    bound, {bound->extent[0], bound->extent[1], bound->extent[2]}
                });
            }
        }
    }
    if (scale == colossus.cape_size) {
        return;
    }
    for (const CapeBase &base : colossus.cape_base) {
        CCloth *cloth = base.cloth;
        for (int i = 0; i < cloth->num_i; i++) {
            for (int j = 0; j < cloth->num_j; j++) {
                cloth->rest[i][j][0] = base.rest[i][j][0] * scale;
                cloth->rest[i][j][1] = base.rest[i][j][1] * scale;
            }
        }
        cloth->wind_effect = base.wind * scale;
    }
    for (const BoundBase &base : colossus.bound_base) {
        for (int k = 0; k < 3; k++) {
            base.bound->extent[k] = base.extent[k] * scale;
            base.bound->reciprocal[k] = base.bound->extent[k] > 0.0f ? 1.0f / base.bound->extent[k] : 0.0f;
        }
    }
    colossus.cape_size = scale;
}

// The model at his size while he is in play; another member's at its own.
void Apply() {
    float scale = GoroInPlay() ? colossus.scale : 1.0f;
    if (scale != colossus.applied || (GoroInPlay() && CharaMain.scale[0] != scale)) {
        CharaMain.SetScale(scale, scale, scale);
        colossus.applied = scale;
    }
    if (GoroInPlay() && CharaMain.cloth != nullptr) {
        SizeCape(scale);
    } else {
        std::fill(std::begin(colossus.cape), std::end(colossus.cape), nullptr); // his cloth went with him
        colossus.cape_size = 1.0f;
        colossus.cape_base.clear();
        colossus.bound_base.clear();
    }
}

// His walk's play rate at his size, and how far a step takes him: his stride is his size times his own.
float StrideRate() {
    return std::pow(colossus.applied, -kColossusStrideRate);
}

} // namespace

float ColossusScale() {
    return colossus.scale;
}

float ColossusSoundPitch() {
    return GoroInPlay() && colossus.applied != 1.0f ? -kColossusPitch * std::log2(colossus.applied) : 0.0f;
}

float ColossusCameraScale() {
    return colossus.applied;
}

float ColossusMoveScale() {
    return GoroInPlay() ? colossus.applied * StrideRate() : 1.0f;
}

void ColossusMotion(CCharacter &chara, int chara_no) {
    // Ours from last step, or the game's own (-1): a rate anyone else put on it stays.
    bool ours = colossus.stride >= 0.0f && chara.motion_speed == colossus.stride;
    if (!ours && chara.motion_speed >= 0.0f) {
        colossus.stride = -1.0f;
        return;
    }
    bool walking = chara_no == CHARA_GORO && colossus.applied != 1.0f && chara.motion_type.motion_info != nullptr &&
                   std::find(std::begin(kStrideMotions), std::end(kStrideMotions), chara.motion_no) != std::end(kStrideMotions);
    if (walking) {
        colossus.stride = chara.motion_type.motion_info[chara.motion_no].speed * StrideRate();
        chara.motion_speed = colossus.stride;
    } else if (ours) {
        chara.motion_speed = -1.0f;
        colossus.stride = -1.0f;
    }
}

void ColossusCamera(CCameraFollow &camera, const float *pos) {
    float scale = colossus.applied;
    if (scale != colossus.camera) {
        camera.distance *= scale / colossus.camera;
        camera.height *= scale / colossus.camera;
        colossus.camera = scale;
    }
    for (int i = 0; i < 3; i++) {
        camera.follow[i] = pos[i] + (camera.follow[i] - pos[i]) * scale;
    }
}

int ColossusAttack(int attack) {
    return Sized() ? static_cast<int>(std::lround(static_cast<double>(attack) * colossus.scale)) : attack;
}

int ColossusDefense(int chara, int defense) {
    if (chara != CHARA_GORO || colossus.scale == 1.0f) {
        return defense;
    }
    float share = colossus.scale > 1.0f ? (colossus.scale - 1.0f) / (kColossusMost - 1.0f) : -(1.0f - colossus.scale) / (1.0f - kColossusLeast);
    return defense + static_cast<int>(std::lround(share * kColossusDefense));
}

float ColossusHitRadius(float radius) {
    return Sized() ? radius * colossus.scale : radius;
}

float ColossusKickSpeed(float speed) {
    return Sized() ? speed * std::sqrt(colossus.scale) : speed;
}

float ColossusHeight(float height) {
    return Sized() ? height * colossus.scale : height;
}

float ColossusHurtReach() {
    return Sized() && colossus.scale > 1.0f ? kColossusBodyRadius * (colossus.scale - 1.0f) : 0.0f;
}

float ColossusBlockRadius(float radius) {
    return Sized() ? radius * colossus.scale : radius;
}

void ColossusShotPoint(float *point) {
    if (Sized()) {
        point[1] = CharaMain.pos[1] + (point[1] - CharaMain.pos[1]) * colossus.scale;
    }
}

bool AtMost() {
    return GoroInPlay() && colossus.scale >= kColossusMost;
}

bool ColossusShrugsOff(int kind) {
    colossus.shrugging = AtMost() && kind == kLightHit;
    if (colossus.shrugging) {
        colossus.kept = BtActStatus;
    }
    return colossus.shrugging;
}

void ColossusShrugged(bool blocked) {
    if (colossus.shrugging && blocked) {
        BtActStatus = colossus.kept;
    }
    colossus.shrugging = false;
}

int ColossusHitKind(int kind) {
    if (!AtMost()) {
        return kind;
    }
    return kind == kGuardBreakingHit ? kKnockbackHit : kind == kKnockbackHit ? kLightHit
                                                                             : kind;
}

// The hit's record in the hit table, or -1 for a hit outside it.
int RecordOf(const COLLISION_HIT &hit) {
    if (NowColData == nullptr) {
        return -1;
    }
    const COLLISION_HIT *first = std::begin(NowColData->hit);
    const COLLISION_HIT *end = std::end(NowColData->hit);
    bool                 inside = !std::less<const COLLISION_HIT *>{}(&hit, first) && std::less<const COLLISION_HIT *>{}(&hit, end);
    return inside ? static_cast<int>(&hit - first) : -1;
}

// The shot that set the hit's record, if one set it just now.
ShotHit *ShotOf(int record) {
    if (record < 0) {
        return nullptr;
    }
    ShotHit &shot = colossus.shot_hits[record];
    return shot.effect != nullptr && colossus.clock - shot.at <= kShotHitLife ? &shot : nullptr;
}

// Whether a monster's sphere is still in the attack that hit him: in use, inside its hit window, and its motion not
// started over since (a new swing).
bool StillAttacking(Spared &spared) {
    if (NowMonstorUnit == nullptr || spared.monster < 0 || spared.monster >= kFloorMonsters || spared.sphere < 0 || spared.sphere >= 16) {
        return false;
    }
    const MONSTOR_EFFECT_STATE2 &hits = NowMonstorUnit->effect2[spared.monster];
    float                        time = NowMonstorUnit->chara[spared.monster][0].motion_type.state.time;
    bool                         still = hits.active[spared.sphere] != 0 && time >= spared.time && time >= hits.motion_start[spared.sphere] && time <= hits.motion_end[spared.sphere];
    spared.time = time;
    return still;
}

// Whether a spared attack goes on.
bool GoesOn(Spared &spared) {
    return spared.sphere >= 0 ? StillAttacking(spared) : colossus.clock < spared.until;
}

bool ColossusPassesBy(const COLLISION_HIT &hit) {
    if (colossus.spared.empty()) {
        return false;
    }
    int            record = RecordOf(hit);
    const ShotHit *shot = ShotOf(record);
    bool           by_shot = shot != nullptr;
    int            monster = hit.owner >= kMonsterOwner ? (hit.owner - kMonsterOwner) / kMonsterOwnerStride : -1;
    for (Spared &spared : colossus.spared) {
        if (by_shot        ? spared.effect == shot->effect && spared.slot == shot->slot
            : monster >= 0 ? spared.monster == monster && spared.sphere == hit.attack_no
                           : spared.record == record) {
            return GoesOn(spared);
        }
    }
    return false;
}

void ColossusHitTaken(const COLLISION_HIT &hit) {
    if (!AtMost()) {
        return;
    }
    int    record = RecordOf(hit);
    Spared spared;
    if (const ShotHit *shot = ShotOf(record); shot != nullptr) {
        // A shot: it hits nothing more and, still in flight, meets him as a wall.
        spared.effect = shot->effect;
        spared.slot = shot->slot;
        spared.until = colossus.clock + kShotHitLife + 1;
        shot->effect->life_time[shot->slot] = 0;
        if (shot->effect->phase[shot->slot] < 2) {
            colossus.stopping.emplace_back(shot->effect, shot->slot);
        }
    } else if (hit.owner >= kMonsterOwner && hit.attack_no >= 0) {
        spared.monster = (hit.owner - kMonsterOwner) / kMonsterOwnerStride;
        spared.sphere = hit.attack_no;
        if (NowMonstorUnit != nullptr && spared.monster < kFloorMonsters) {
            spared.time = NowMonstorUnit->chara[spared.monster][0].motion_type.state.time;
        }
    } else if (record >= 0) {
        spared.record = record;
        spared.until = colossus.clock + hit.life + 1;
    } else {
        return;
    }
    colossus.spared.push_back(spared);
}

void ColossusShotHitSet(CSHOT_EFFECT &effect, int slot, const COLLISION_HIT &hit) {
    if (int record = RecordOf(hit); record >= 0) {
        colossus.shot_hits[record] = {&effect, slot, colossus.clock};
        if (&effect == colossus.impact && slot == colossus.impact_slot) {
            NowColData->hit[record].radius *= colossus.impact_size;
        }
    }
}

void ColossusImpact(CSHOT_EFFECT &effect, int slot) {
    if (slot < 0 || slot >= static_cast<int>(std::size(effect.chara))) {
        return;
    }
    float size = GoroInPlay() ? colossus.applied : 1.0f;
    effect.chara[slot].SetScale(size, size, size);
    colossus.impact = size != 1.0f ? &effect : nullptr;
    colossus.impact_slot = size != 1.0f ? slot : -1;
    colossus.impact_size = size;
}

SHOT_COLLISION_RESULT ColossusShotCollision(const CSHOT_EFFECT &effect, int slot, SHOT_COLLISION_RESULT result) {
    auto stopping = std::find(colossus.stopping.begin(), colossus.stopping.end(), std::pair{&effect, slot});
    if (stopping == colossus.stopping.end()) {
        return result;
    }
    colossus.stopping.erase(stopping);
    return effect.phase[slot] < 2 ? SHOT_COLLISION_MAP : result;
}

bool ColossusTrample(const COLLISION_HIT &hit) {
    return (hit.weapon_flags & kTrampleMark) != 0;
}

bool ColossusTriggersSwitches() {
    return GoroInPlay() && colossus.scale >= kColossusSwitchSize;
}

bool ColossusStepsOn(int chara_no, const float *pos, float radius) {
    if (chara_no != CHARA_GORO || !ColossusTriggersSwitches()) {
        return false;
    }
    float rise = pos[1] - CharaMain.pos[1];
    return std::hypot(pos[0] - CharaMain.pos[0], pos[2] - CharaMain.pos[2]) <= radius && rise >= -kSwitchBelow &&
           rise <= kColossusHeightOf * colossus.scale;
}

void ColossusStep() {
    if (UserStatus == nullptr) {
        return;
    }
    SndSetVoicePitch(ColossusSoundPitch());
    if (colossus.impact != nullptr && colossus.impact->active[colossus.impact_slot] == 0) {
        // Its slot done: whatever plays there next is at its own size.
        colossus.impact->chara[colossus.impact_slot].SetScale(1.0f, 1.0f, 1.0f);
        colossus.impact = nullptr;
        colossus.impact_slot = -1;
    }
    if (MonstersStepping()) {
        colossus.clock++;
        std::erase_if(colossus.spared, [](Spared &spared) { return !GoesOn(spared); });
        if (GoroInPlay() && colossus.phase != Phase::Shrinking && colossus.phase != Phase::Spent && colossus.phase != Phase::Popping) {
            Grow();
        }
        Advance();
        if (GoroInPlay()) {
            Trample();
        } else {
            colossus.stood = false;
        }
    }
    Apply();
}

void ColossusQuiet() {
    SndSetVoicePitch(0.0f);
}

void ColossusReset() {
    SummonSmokeEnd(&colossus);
    ColossusQuiet();
    if (colossus.applied != 1.0f) {
        CharaMain.SetScale(1.0f, 1.0f, 1.0f);
    }
    if (CharaMain.cloth != nullptr) {
        SizeCape(1.0f);
    }
    if (colossus.stride >= 0.0f && CharaMain.motion_speed == colossus.stride) {
        CharaMain.motion_speed = -1.0f;
    }
    colossus = Colossus{};
}

} // namespace expanded
