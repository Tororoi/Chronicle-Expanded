#include "expanded/weapons/goro/tall_hammer.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include "btactstatus.hpp"
#include "character.hpp"
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

// His hammer's swing throw (GoroKey_Play).
constexpr float kGoroKick = 2.5f, kGoroKickDecay = 0.1f;

constexpr float kWalking = 0.05f;          // how far a step must take him to be walking
constexpr float kTrampleReach = 2.0f;      // past where the monster stops against him
constexpr float kSwitchBelow = 5.0f;       // how far under his feet a switch he stands on may lie
constexpr float kColossusHeightOf = 16.0f; // Goro's height (CharaHeight)

enum class Phase {
    Normal,
    Grown,
    Shrinking,
    Spent,
    Popping
};

struct Colossus {
    Phase         phase = Phase::Normal;
    float         scale = 1.0f;
    int           grown = 0;          // frames he has grown for (Grown)
    int           frames = 0;         // in the phase (Grown: since he first grew; Spent: since the shrink began)
    float         shrink_from = 1.0f; // his size as the shrink began
    float         applied = 1.0f;     // the size last put on the model
    float         last[2] = {};       // where he stood the step before
    bool          stood = false;
    int           trampled[kFloorMonsters] = {};
    int           clock = 0;
    bool          shrugging = false; // a hit he shrugs off is being reacted to
    BT_ACT_STATUS kept = {};         // what he was doing as it reached him
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

// The model at his size while he is in play; another member's at its own.
void Apply() {
    float scale = GoroInPlay() ? colossus.scale : 1.0f;
    if (scale != colossus.applied || (GoroInPlay() && CharaMain.scale[0] != scale)) {
        CharaMain.SetScale(scale, scale, scale);
        colossus.applied = scale;
    }
}

} // namespace

float ColossusScale() {
    return colossus.scale;
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
    if (MonstersStepping()) {
        colossus.clock++;
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

void ColossusReset() {
    SummonSmokeEnd(&colossus);
    if (colossus.applied != 1.0f) {
        CharaMain.SetScale(1.0f, 1.0f, 1.0f);
    }
    colossus = Colossus{};
}

} // namespace expanded
