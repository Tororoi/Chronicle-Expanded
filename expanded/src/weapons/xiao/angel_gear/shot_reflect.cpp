#include "expanded/weapons/xiao/angel_gear/shot_reflect.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/xiao/angel_gear/shield_prop.hpp"
#include "frame.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded::angel_gear {

namespace {

constexpr int   kPlayerOnly = 1; // a collision test's target: the player and walls
constexpr int   kElements = 0x1F, kAilments = 0xFF00;
constexpr int   kFreeze = 0x100, kPoison = 0x200, kSlow = 0x800;
constexpr float kLockOnReach = 200.0f; // a lock-on point farther than this from the monster is stale
constexpr int   kReturnTracked = 780;  // frames a returned shot is followed at most (the mod's 260 ticks)

struct Shot {
    int effect = -1; // pack index
    int slot = -1;

    bool operator==(const Shot &other) const = default;
};

// A claimed shot: where it is, and what fires it back.
struct Claim {
    Shot  shot;
    float speed = 0.0f;
    float back[3] = {}; // the way it came, a unit vector
    int   damage = 0;
    s16   owner = -1, source = -1;
    char  quiet = 0, rearm = 0;
    float last[3] = {};
    int   last_delay = 0; // its flight left when last seen: 0 or 1 is a shot that ran out
};

struct Flying {
    Shot shot;
    int  frames = 0;
};

// A monster with life, standing somewhere: the mod's test for one a shot can be fired at or meet.
bool Present(const MONSTOR &live, const float *at) {
    return live.hp > 0 && !(at[0] == 0.0f && at[1] == 0.0f && at[2] == 0.0f);
}

std::vector<Claim>  claims;
std::vector<Claim>  pending;
std::vector<Flying> flying;
Shot                faced;           // the claim the slingshot faces
bool                testing = false; // a returned shot's collision test under way

CSHOT_EFFECT *Effect(int index) {
    return LoadedShotEffect(index);
}

// The pack index of an effect.
int IndexOf(const CSHOT_EFFECT &effect) {
    for (int i = 0; i < ShotEffectCount(); i++) {
        if (Effect(i) == &effect) {
            return i;
        }
    }
    return -1;
}

bool Live(const Shot &shot) {
    CSHOT_EFFECT *effect = Effect(shot.effect);
    return effect != nullptr && effect->effect_data != nullptr && shot.slot >= 0 && shot.slot < effect->slot_count && effect->active[shot.slot] != 0;
}

bool Claimed(const Shot &shot) {
    return std::any_of(claims.begin(), claims.end(), [&](const Claim &c) { return c.shot == shot; });
}

bool Returned(const Shot &shot) {
    return std::any_of(flying.begin(), flying.end(), [&](const Flying &f) { return f.shot == shot; });
}

void Position(CSHOT_EFFECT &effect, int slot, float *out) {
    effect.chara[slot].GetPosition(out);
}

float Dist2(const float *a, const float *b) {
    float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}

// Turns a shot's model along its flight.
void Face(CSHOT_EFFECT &effect, int slot, const float *direction) {
    sceVu0FMATRIX rotation;
    float         d[4] = {direction[0], direction[1], direction[2], 1.0f};
    LookAtMatrixZ(rotation, d);
    effect.chara[slot].frame->SetTransMatrix(rotation);
}

// The freeze, poison and slow of a shot, as CheckDmg gives a monster the ones its own hit carries.
void GiveAilments(MONSTOR &live, int monster, int given) {
    if (live.status_chance == 0) {
        return;
    }
    if ((given & kPoison) != 0 && live.stop_timer == 0 && live.anger_timer == 0) {
        Poison(&live, monster);
    }
    if ((given & kFreeze) != 0) {
        if (live.stop_timer <= 0) {
            live.stop_timer = 300;
            live.poison_timer = 0;
            live.slow_timer = 0;
            live.anger_timer = 0;
            live.movement_speed = 0.0f;
        } else {
            live.stop_timer = 0;
        }
    }
    if ((given & kSlow) != 0 && live.stop_timer == 0 && live.poison_timer == 0 && live.anger_timer == 0) {
        live.slow_timer = 180;
    }
}

// A returned shot met monster at point, reaching radius: the hit planted there, and the shot's ailments given.
void Land(const CSHOT_EFFECT &effect, const float *point, float radius, int monster) {
    if (NowColData == nullptr || NowWeaponHave == nullptr || UserStatus == nullptr) {
        return;
    }
    int   dungeon = std::clamp(selectMapNo, static_cast<s32>(DUNGEON_DIVINE_BEAST_CAVE), static_cast<s32>(DUNGEON_DEMON_SHAFT));
    float base = static_cast<float>(NowWeaponHave->attack) * static_cast<float>(dungeon + 1) / kTierDivisor;
    int   damage = std::max(1, static_cast<int>(std::nearbyint(base))); // halves to even, as the mod rounds
    int   flags = effect.effect_data->hit_flags;
    int   element = (flags & kElements) != 0 && (flags & kAilments) == 0 ? flags & kElements : 0;
    float at[4] = {point[0], point[1], point[2], 1.0f};
    int   hit = NowColData->Set(at, damage, kHitLife, radius, 0.0f, kHitsMonsters, kSwingReaction, element, 0);
    if (hit < 0) {
        return;
    }
    COLLISION_HIT &record = NowColData->hit[hit];
    record.owner = UserStatus->cur_chara;
    record.attack_no = 0;
    record.vs_monster = NowWeaponHave->vs_monster;
    record.weapon_flags = NowWeaponHave->flags;
    if (NowMonstorUnit != nullptr) {
        GiveAilments(NowMonstorUnit->monster[monster], monster, flags & (kFreeze | kPoison | kSlow));
    }
}

// A returned shot's test from position along its move, as Xiao's pellets are tested: a monster's hurt sphere within its
// reach (the monster into monster), else a wall; never the player.
SHOT_COLLISION_RESULT ReturnTest(CSHOT_EFFECT &effect, int slot, float *hit_position, float *position, int *monster) {
    float radius = effect.effect_data->radius[effect.phase[slot]];
    for (int m = 0; NowMonstorUnit != nullptr && m < kFloorMonsters; m++) {
        const MONSTOR &live = NowMonstorUnit->monster[m];
        for (int sphere = 0; live.state != -1 && live.revealed != 0 && sphere < kMonsterSpheres; sphere++) {
            const MONSTOR_EFFECT_STATE &body = NowMonstorUnit->effect[m];
            if (body.timer[sphere] != 0 && DistVector(const_cast<float *>(body.position[sphere]), position) <= radius + body.radius[sphere]) {
                sceVu0CopyVector(hit_position, const_cast<float *>(body.position[sphere]));
                *monster = m;
                return SHOT_COLLISION_MONSTER;
            }
        }
    }
    if (NowDngMap == nullptr) {
        return SHOT_COLLISION_NONE;
    }
    testing = true; // the player's point out of reach
    SHOT_COLLISION_RESULT result = checkCollision(hit_position, position, effect.velocity[slot], kPlayerOnly, radius);
    testing = false;
    return result;
}

} // namespace

void ClaimShots(const float *her) {
    for (int e = 0; e < ShotEffectCount(); e++) {
        CSHOT_EFFECT *effect = Effect(e);
        if (effect == nullptr || effect->effect_data == nullptr || effect->effect_data->target == kHitsMonsters) {
            continue;
        }
        for (int i = 0; i < effect->slot_count && i < 8; i++) {
            Shot shot{e, i};
            if (effect->active[i] == 0 || effect->phase[i] != 1 || Claimed(shot) || Returned(shot)) {
                continue;
            }
            float at[4];
            Position(*effect, i, at);
            float        d[3] = {her[0] - at[0], her[1] - at[1], her[2] - at[2]};
            float        distance = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            const float *v = effect->velocity[i];
            if (distance > kClaimRadius || v[0] * d[0] + v[1] * d[1] + v[2] * d[2] <= 0.0f) {
                continue;
            }
            float speed = std::max(1.0f, std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
            int   delay = effect->phase_delay[i];
            if (delay * speed < distance - kPropAhead - kCaptureRadius) {
                continue; // it would end short of the pouch (or flies for good)
            }
            effect->wait_state[i] = kHeld;
            Claim claim;
            claim.shot = shot;
            claim.speed = speed;
            for (int c = 0; c < 3; c++) {
                claim.back[c] = -v[c] / speed;
                claim.last[c] = at[c];
            }
            claim.damage = effect->damage[i];
            claim.owner = effect->user_id[i];
            claim.source = effect->source_id[i];
            claim.quiet = effect->no_sound[i];
            claim.rearm = effect->wait[i];
            claim.last_delay = delay;
            claims.push_back(claim);
        }
    }
}

void SustainClaims(bool shield_up, bool solid, const float *her, const float *pouch) {
    if (!shield_up) {
        for (const Claim &claim : claims) {
            if (CSHOT_EFFECT *effect = Effect(claim.shot.effect)) {
                effect->wait_state[claim.shot.slot] = 0; // free to land at once
            }
        }
        claims.clear();
        pending.clear();
        return;
    }
    for (int q = static_cast<int>(claims.size()) - 1; q >= 0; q--) {
        Claim        &claim = claims[q];
        CSHOT_EFFECT *effect = Effect(claim.shot.effect);
        int           slot = claim.shot.slot;
        bool          by_game = !Live(claim.shot);
        bool          gone = by_game, caught = false;
        float         body_reach = kAbsorbNear + claim.speed * 3.0f;
        if (!gone) {
            float at[4];
            Position(*effect, slot, at);
            std::copy(at, at + 3, claim.last);
            claim.last_delay = effect->phase_delay[slot];
            float catch_reach = kCaptureRadius + claim.speed * 2.0f;
            float to_pouch = Dist2(pouch, at);
            if (solid && to_pouch < catch_reach * catch_reach) {
                effect->active[slot] = 0; // into the pouch
                gone = caught = true;
            } else if (effect->phase[slot] > 1 && to_pouch < kEngineCatchNear * kEngineCatchNear) {
                effect->active[slot] = 0; // the game stopped it at the pouch: caught there
                gone = caught = true;
            } else if (Dist2(her, at) < body_reach * body_reach || effect->phase[slot] > 1) {
                effect->active[slot] = 0; // quietly into her
                gone = true;
            } else if (solid && claim.shot == faced && to_pouch < kHomingRange * kHomingRange) {
                float *v = effect->velocity[slot];
                float  vl = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]), ql = std::sqrt(to_pouch);
                if (vl > 1e-3f && ql > 1e-3f) {
                    float n[3];
                    for (int c = 0; c < 3; c++) {
                        n[c] = v[c] / vl * (1.0f - kHomingGain) + (pouch[c] - at[c]) / ql * kHomingGain;
                    }
                    float nl = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
                    if (nl > 1e-3f) {
                        for (int c = 0; c < 3; c++) {
                            v[c] = n[c] / nl * vl; // its speed kept
                        }
                        Face(*effect, slot, v);
                    }
                }
            }
        }
        if (!gone) {
            effect->wait_state[slot] = kHeld; // harmless to the end
            continue;
        }
        if (by_game) {
            // Caught when it ended at the pouch, taken into her when at her; ran out or ended elsewhere, it is lost.
            if (claim.last_delay <= 1) {
                claims.erase(claims.begin() + q);
                continue;
            }
            if (Dist2(pouch, claim.last) < kEngineCatchNear * kEngineCatchNear) {
                caught = true;
            } else if (Dist2(her, claim.last) >= body_reach * body_reach) {
                claims.erase(claims.begin() + q);
                continue;
            }
        }
        if (static_cast<int>(pending.size()) < kPendingMax) {
            pending.push_back(claim);
            if (!caught) {
                setUnitAmbientAnime(kAbsorbFlashFrames, 1.0f, kAbsorbFlash, kAbsorbFlash, kAbsorbFlash);
            }
        }
        claims.erase(claims.begin() + q);
    }
}

bool ChooseBearing(const float *her, float yaw, bool firing, float *bearing) {
    *bearing = yaw;
    faced = {};
    float aim[4];
    if (firing || (!pending.empty() && claims.empty())) {
        if (NearestAim(her, aim)) {
            *bearing = std::atan2(aim[0] - her[0], aim[2] - her[2]);
            return true;
        }
        return false;
    }
    float best = 0.0f;
    for (const Claim &claim : claims) {
        CSHOT_EFFECT *effect = Effect(claim.shot.effect);
        if (effect == nullptr) {
            continue;
        }
        float at[4];
        Position(*effect, claim.shot.slot, at);
        float d = (at[0] - her[0]) * (at[0] - her[0]) + (at[2] - her[2]) * (at[2] - her[2]);
        if (faced.effect < 0 || d < best) {
            best = d;
            faced = claim.shot;
            *bearing = std::atan2(at[0] - her[0], at[2] - her[2]);
        }
    }
    if (faced.effect >= 0) {
        return true;
    }
    if (NearestAim(her, aim)) {
        *bearing = std::atan2(aim[0] - her[0], aim[2] - her[2]);
    }
    return true; // with nothing about, straight ahead
}

bool FireReturn(const float *her, const float *muzzle) {
    if (pending.empty()) {
        return true;
    }
    const Claim   claim = pending.front();
    CSHOT_EFFECT *effect = Effect(claim.shot.effect);
    if (effect == nullptr || effect->effect_data == nullptr) {
        pending.erase(pending.begin());
        return true;
    }
    float aim[3] = {claim.back[0], claim.back[1], claim.back[2]};
    float target[4];
    if (NearestAim(muzzle, target)) {
        float d[3] = {target[0] - muzzle[0], target[1] - muzzle[1], target[2] - muzzle[2]};
        float length = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (length >= 1.0f) {
            for (int c = 0; c < 3; c++) {
                aim[c] = d[c] / length;
            }
        }
    }
    // Clear of her: a shot born where she would meet it ends at once. From the muzzle down the aim, or past her body when
    // the aim runs back through her.
    float to_her[3] = {her[0] - muzzle[0], her[1] - muzzle[1], her[2] - muzzle[2]};
    float along = to_her[0] * aim[0] + to_her[1] * aim[1] + to_her[2] * aim[2];
    float off2 = to_her[0] * to_her[0] + to_her[1] * to_her[1] + to_her[2] * to_her[2] - along * along;
    float lead = along > 0.0f && off2 < (kAbsorbNear + 4.0f) * (kAbsorbNear + 4.0f) ? along + kPastHer : kClearLead;
    float from[4] = {muzzle[0] + aim[0] * lead, muzzle[1] + aim[1] * lead, muzzle[2] + aim[2] * lead, 1.0f};
    float to[4] = {from[0] + aim[0], from[1] + aim[1], from[2] + aim[2], 1.0f};
    s32   status = effect->status; // the pack's, which the mod's spawn left alone
    int   slot = effect->Set(from, to, claim.owner, -1, claim.source, nullptr, 1);
    effect->status = status;
    if (slot < 0) {
        return false; // every shot of its kind is out: it waits
    }
    float speed = claim.speed * kReturnBoost;
    for (int c = 0; c < 3; c++) {
        effect->velocity[slot][c] = aim[c] * speed;
    }
    effect->damage[slot] = claim.damage;
    effect->phase_delay[slot] = kReturnFrames;
    effect->no_sound[slot] = claim.quiet;
    effect->wait[slot] = claim.rearm;
    effect->wait_state[slot] = kHeld;
    Face(*effect, slot, aim);
    flying.push_back({
        Shot{claim.shot.effect, slot},
        0
    });
    pending.erase(pending.begin());
    return true;
}

void TrackReturns() {
    std::erase_if(flying, [](Flying &f) {
        if (!Live(f.shot) || ++f.frames > kReturnTracked) {
            return true;
        }
        Effect(f.shot.effect)->wait_state[f.shot.slot] = kHeld; // its own hit never lands
        return false;
    });
}

int ClaimCount() {
    return static_cast<int>(claims.size());
}

int PendingCount() {
    return static_cast<int>(pending.size());
}

void DropPending() {
    pending.clear();
}

bool NearestAim(const float *from, float *aim) {
    if (NowMonstorUnit == nullptr) {
        return false;
    }
    CMonstorUnit &unit = *NowMonstorUnit;
    float         best = 0.0f;
    bool          found = false;
    for (int m = 0; m < kFloorMonsters; m++) {
        const MONSTOR &live = unit.monster[m];
        float          at[4];
        unit.chara[m][0].GetPosition(at);
        if (!Present(live, at)) {
            continue;
        }
        float d = (at[0] - from[0]) * (at[0] - from[0]) + (at[2] - from[2]) * (at[2] - from[2]);
        if (found && d >= best) {
            continue;
        }
        found = true;
        best = d;
        aim[0] = at[0], aim[1] = at[1] + kAimLift, aim[2] = at[2];
        if (live.lockon_frame != nullptr && std::fabs(live.lockon_position[1] - at[1]) < kLockOnReach) {
            std::copy(live.lockon_position, live.lockon_position + 3, aim);
        }
    }
    return found;
}

void ReflectReset() {
    claims.clear(); // their holds run out on their own
    pending.clear();
    flying.clear();
    faced = {};
}

SHOT_COLLISION_RESULT ReflectCollision(CSHOT_EFFECT &effect, int slot, float *hit_position, float *position) {
    int  index = flying.empty() ? -1 : IndexOf(effect);
    auto ours = std::find_if(flying.begin(), flying.end(), [&](const Flying &f) { return f.shot == Shot{index, slot}; });
    if (ours == flying.end()) {
        return checkCollision(hit_position, position, effect.velocity[slot], effect.effect_data->target, effect.effect_data->radius[effect.phase[slot]]);
    }
    effect.wait_state[slot] = kHeld;
    int                   monster = -1;
    SHOT_COLLISION_RESULT result = ReturnTest(effect, slot, hit_position, position, &monster);
    if (result != SHOT_COLLISION_MONSTER) {
        return result;
    }
    Land(effect, hit_position, std::max(0.5f, effect.effect_data->radius[1]) + kHitMargin, monster);
    effect.phase_delay[slot] = 0; // its flight ends with its burst, as the mod ends it
    flying.erase(ours);
    return SHOT_COLLISION_NONE;
}

bool ReturnTesting() {
    return testing;
}

} // namespace expanded::angel_gear
