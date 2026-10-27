#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/xiao/hardshooter.hpp"

#include <cmath>

#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int   kSpheres = 16;
constexpr int   kMonsters = 16;
constexpr float kPelletRadius = 2.0f; // checkCollision's, for a pellet
constexpr float kTwoPi = 6.28318530718f;

constexpr int kDepartTicks = 30; // the most frames a ricochet is stepped clear before the game takes it

bool ricochet[kPelletSlots] = {};
int  departing[kPelletSlots] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // the monster a ricochet is leaving
int  departed[kPelletSlots] = {};
int  partner[kPelletSlots] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // the other pellet of a pair
int  claimed[kPelletSlots] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // the monster the partner's ricochet took

void Unpair(int slot) {
    partner[slot] = -1;
    claimed[slot] = -1;
}

void OnFloor() {
    for (int slot = 0; slot < kPelletSlots; slot++) {
        ricochet[slot] = false;
        departing[slot] = -1;
        Unpair(slot);
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

float Distance(const float *a, const float *b) {
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// A monster checkCollision tests: on the floor and shown.
bool Hittable(const CMonstorUnit &unit, int monster) {
    return unit.monster[monster].state != -1 && unit.monster[monster].revealed != 0;
}

// Whether monster's hurt sphere takes hits now: in use, and within its motion window when it has one (CheckDmg's test).
bool Valid(const CMonstorUnit &unit, int monster, int sphere) {
    const MONSTOR_EFFECT_STATE &effect = unit.effect[monster];
    if (effect.timer[sphere] == 0) {
        return false;
    }
    float time = unit.chara[monster][0].motion_type.state.time;
    return effect.motion_start[sphere] == 0.0f || (effect.motion_start[sphere] < time && time <= effect.motion_end[sphere]);
}

// The centre of monster's largest hurt sphere taking hits now, or nullptr.
const float *LargestSphere(const CMonstorUnit &unit, int monster) {
    const MONSTOR_EFFECT_STATE &effect = unit.effect[monster];
    int                         largest = -1;
    for (int sphere = 0; sphere < kSpheres; sphere++) {
        if (Valid(unit, monster, sphere) && (largest < 0 || effect.radius[sphere] > effect.radius[largest])) {
            largest = sphere;
        }
    }
    return largest < 0 ? nullptr : effect.position[largest];
}

// Whether point lies within a pellet's reach (and margin more) of one of monster's hurt spheres.
bool Inside(const CMonstorUnit &unit, int monster, const float *point, float margin = 0.0f) {
    const MONSTOR_EFFECT_STATE &effect = unit.effect[monster];
    for (int sphere = 0; sphere < kSpheres; sphere++) {
        if (effect.timer[sphere] != 0 && Distance(effect.position[sphere], point) <= kPelletRadius + effect.radius[sphere] + margin) {
            return true;
        }
    }
    return false;
}

// The monster nearest struck (other than it and avoid, within kRicochetRange) with a hurt sphere taking hits now, or -1;
// aim is its largest such sphere.
int NextTarget(const CMonstorUnit &unit, int struck, int avoid, const float *point, const float **aim) {
    int   target = -1;
    float best = kRicochetRange;
    for (int monster = 0; monster < kMonsters; monster++) {
        if (monster == struck || monster == avoid || !Hittable(unit, monster) || unit.monster[monster].hp <= 0) {
            continue;
        }
        const float *sphere = LargestSphere(unit, monster);
        if (sphere != nullptr && Distance(sphere, point) <= best) {
            target = monster;
            *aim = sphere;
            best = Distance(sphere, point);
        }
    }
    return target;
}

} // namespace

bool RicochetActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO &&
           (WeaponActive(ITEM_WEAPON_HARDSHOOTER) || WeaponActive(ITEM_WEAPON_DOUBLE_IMPACT));
}

bool Ricochet(int slot) {
    return slot >= 0 && slot < kPelletSlots && ricochet[slot];
}

int RicochetStruck(const CMonstorUnit &unit, const float *point) {
    for (int monster = 0; monster < kMonsters; monster++) {
        if (!Hittable(unit, monster)) {
            continue;
        }
        const MONSTOR_EFFECT_STATE &effect = unit.effect[monster];
        for (int sphere = 0; sphere < kSpheres; sphere++) {
            const float *centre = effect.position[sphere];
            if (effect.timer[sphere] != 0 && centre[0] == point[0] && centre[1] == point[1] && centre[2] == point[2]) {
                return monster;
            }
        }
    }
    return -1;
}

void RicochetPair(int first, int second) {
    if (first < 0 || first >= kPelletSlots || second < 0 || second >= kPelletSlots) {
        return;
    }
    Unpair(first);
    Unpair(second);
    partner[first] = second;
    partner[second] = first;
}

void RicochetShotFired(int slot) {
    if (slot >= 0 && slot < kPelletSlots) {
        ricochet[slot] = false;
        departing[slot] = -1;
        Unpair(slot);
    }
}

void RicochetStep(CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots || departing[slot] < 0 || NowMonstorUnit == nullptr) {
        return;
    }
    const CMonstorUnit &unit = *NowMonstorUnit;
    int                 from = departing[slot];
    const float        *v = shots.vector[slot];
    for (int i = 0; i < 3; i++) {
        shots.pos[slot][i] += v[i];
    }
    float speed = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    bool  clear = !Hittable(unit, from) || !Inside(unit, from, shots.pos[slot], speed);
    bool  met = false;
    for (int monster = 0; monster < kMonsters && !met; monster++) {
        met = monster != from && Hittable(unit, monster) && Inside(unit, monster, shots.pos[slot]);
    }
    // The game tests monsters in slot order, so one met while still within the struck monster could land on it.
    bool left = clear || !Inside(unit, from, shots.pos[slot]);
    if (clear || (met && left) || ++departed[slot] >= kDepartTicks) {
        shots.halted[slot] = 0;
        departing[slot] = -1;
    }
}

int RicochetShotEnded(CSHOT &shots, int slot, const float *point, bool struck) {
    if (slot < 0 || slot >= kPelletSlots) {
        return -1;
    }
    bool bounced = ricochet[slot];
    int  pair = partner[slot];
    int  avoid = claimed[slot];
    ricochet[slot] = false;
    departing[slot] = -1;
    Unpair(slot);
    if (!struck || bounced || NowMonstorUnit == nullptr || !RicochetActive()) {
        return -1;
    }
    const CMonstorUnit &unit = *NowMonstorUnit;
    int                 from = RicochetStruck(unit, point);
    if (from < 0) {
        return -1;
    }
    int free = -1;
    for (int other = 0; other < kPelletSlots; other++) {
        if (other != slot && shots.used[other] == 0) {
            free = other;
            break;
        }
    }
    if (free < 0) {
        return -1;
    }

    const float *start = shots.pos[slot];
    float        speed = std::sqrt(shots.vector[slot][0] * shots.vector[slot][0] + shots.vector[slot][1] * shots.vector[slot][1] +
                                   shots.vector[slot][2] * shots.vector[slot][2]);
    float        way[3];
    const float *aim = nullptr;
    int          target = NextTarget(unit, from, avoid, point, &aim);
    if (target >= 0) {
        for (int i = 0; i < 3; i++) {
            way[i] = aim[i] - start[i];
        }
    } else {
        float angle = static_cast<float>(ModRoll(3600)) * kTwoPi / 3600.0f;
        way[0] = std::sin(angle);
        way[1] = 0.0f;
        way[2] = std::cos(angle);
    }
    float length = std::sqrt(way[0] * way[0] + way[1] * way[1] + way[2] * way[2]);
    if (length <= 0.0f || speed <= 0.0f) {
        return -1;
    }
    for (float &axis : way) {
        axis /= length;
    }

    // The game takes it once it is clear of the struck monster (RicochetStep): it never comes back at it.
    for (int i = 0; i < 3; i++) {
        shots.pos[free][i] = start[i];
        shots.unk_100[free][i] = start[i];
        shots.vector[free][i] = way[i] * speed;
    }
    shots.pos[free][3] = shots.unk_100[free][3] = 1.0f;
    shots.vector[free][3] = 0.0f;
    shots.halted[free] = 1;
    shots.life[free] = kRicochetLife;
    shots.damage[free] = shots.damage[slot];
    shots.size[free] = shots.size[slot];
    shots.unk_340[free] = shots.unk_340[slot];
    shots.unk_370[free] = shots.unk_370[slot];
    shots.start_flag[free] = 0;
    shots.used[free] = 1;
    ricochet[free] = true;
    departing[free] = from;
    departed[free] = 0;
    // The other pellet of the pair takes another monster.
    if (target >= 0 && pair >= 0 && partner[pair] == slot) {
        claimed[pair] = target;
    }
    return free;
}

} // namespace expanded
