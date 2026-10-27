#include "expanded/enemies/boss_moves.hpp"

#include <algorithm>
#include <cmath>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/enemies/boss_table.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/roster.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

constexpr float kBodyBuffer = 10.0f; // kept from walls beyond the body's radius
constexpr int   kReplanSteps = 8;    // a path is planned again this often
constexpr float kReplanMove = 8.0f;  // or when what it is made for moves this far
constexpr float kWaypointReached = 10.0f;
constexpr int   kAimSteps = 2;       // an aimed point steers this long after the script last named it
constexpr int   kRegionLimit = 4000; // cells of the floor an arena boss's reach is worked out for
constexpr int   kRegionBudget = 25;  // cells of it taken in each step (4 tests each)

struct Mover {
    bool  active = false;
    bool  arena = false;
    float anchor[3] = {};
    bool  moving = false;
    float goal[3] = {};
    float aim[3] = {};
    int   aim_age = kAimSteps + 1;
    float waypoint[3] = {};
    float planned_for[3] = {};
    int   replan = 0;
    int   region = -1; // where an arena boss can reach from its anchor
};

Mover movers[kFloorMonsters];

float Clearance(const CMonstorUnit &unit, int monster) {
    const MONSTOR &live = unit.monster[monster];
    return std::max(live.body_radius, live.collision_radius) + kBodyBuffer;
}

float Distance2(const float *a, const float *b) {
    float dx = a[0] - b[0];
    float dz = a[2] - b[2];
    return std::sqrt(dx * dx + dz * dz);
}

// The middle of the floor's largest room (the nearest to at of equals), or false.
bool LargestRoom(const float *at, float *middle) {
    if (NowDngMap == nullptr || NowDngMap->map_type != 1) {
        return false;
    }
    int   best = -1;
    float best_distance = 0.0f;
    for (int i = 0; i < NowDngMap->room_num && i < 16; i++) {
        const ROOM_INFO &room = NowDngMap->rooms[i];
        float            x = 160.0f * (static_cast<float>(room.x) + static_cast<float>(room.width - 1) / 2.0f);
        float            z = 160.0f * (static_cast<float>(room.y) + static_cast<float>(room.height - 1) / 2.0f);
        float            distance = std::hypot(x - at[0], z - at[2]);
        int              area = room.width * room.height;
        int              best_area = best < 0 ? 0 : NowDngMap->rooms[best].width * NowDngMap->rooms[best].height;
        if (area > best_area || (area == best_area && distance < best_distance)) {
            best = i;
            best_distance = distance;
            middle[0] = x;
            middle[1] = at[1];
            middle[2] = z;
        }
    }
    return best >= 0;
}

void OnSpawn(CMonstorUnit &unit, int monster) {
    Mover &mover = movers[monster];
    mover = {};
    const BossEntry *boss = BossEntryOf(ModelKind(unit.monster[monster].base_model));
    if (LoadedRoster() == nullptr || boss == nullptr) {
        return;
    }
    mover.active = true;
    if (!boss->arena_frame) {
        return;
    }
    // Its arena is the floor's largest room, the boss in its middle.
    mover.arena = true;
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    float middle[3];
    if (LargestRoom(at, middle)) {
        float spot[3];
        NavigateNearest(middle, middle, Clearance(unit, monster), at[1], spot);
        at[0] = spot[0];
        at[2] = spot[2];
        unit.chara[monster][0].SetPosition(at);
    }
    std::copy(at, at + 3, mover.anchor);
    mover.region = NavigateRegion(mover.anchor, Clearance(unit, monster), at[1], kRegionLimit);
}

[[maybe_unused]] const bool kListening = (AddEnemySpawnListener(OnSpawn), true);

} // namespace

void BossPointIn(CMonstorUnit &unit, int monster, float *point, BossPoint use) {
    if (monster < 0 || monster >= kFloorMonsters || !movers[monster].arena) {
        return;
    }
    Mover &mover = movers[monster];
    for (int i = 0; i < 3; i++) {
        point[i] += mover.anchor[i];
    }
    if (use == BossPoint::Target) {
        return;
    }
    float spot[3];
    if (NavigateRegionNearest(mover.region, point, spot)) {
        point[0] = spot[0];
        point[2] = spot[2];
    }
    if (use == BossPoint::Aim) {
        std::copy(point, point + 3, mover.aim);
        mover.aim_age = 0;
    }
}

bool BossInArenaFrame(int monster) {
    return monster >= 0 && monster < kFloorMonsters && movers[monster].arena;
}

void BossPointOut(int monster, float *point) {
    if (monster < 0 || monster >= kFloorMonsters || !movers[monster].arena) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        point[i] -= movers[monster].anchor[i];
    }
}

void BossMoveGoal(int monster, const float *goal) {
    if (monster < 0 || monster >= kFloorMonsters || !movers[monster].active) {
        return;
    }
    std::copy(goal, goal + 3, movers[monster].goal);
    movers[monster].moving = true;
}

void BossSteer(CMonstorUnit &unit, int monster) {
    if (monster < 0 || monster >= kFloorMonsters || !movers[monster].active) {
        return;
    }
    Mover   &mover = movers[monster];
    MONSTOR &live = unit.monster[monster];
    NavigateRegionGrow(mover.region, kRegionBudget);
    mover.aim_age++;
    if (!mover.moving || live.movement_speed == 0.0f || live.knockback[0] > 0.0f) {
        mover.moving = live.movement_speed != 0.0f && mover.moving;
        return;
    }
    // An arena boss steers for the point it is making for; any other for where it moves.
    bool         aiming = mover.arena && mover.aim_age <= kAimSteps;
    const float *target = aiming ? mover.aim : mover.goal;
    float        at[4];
    unit.chara[monster][0].GetPosition(at);
    // Within a step of it: on as the script set (past where it moves to, as in retail).
    if (Distance2(at, target) <= std::max(live.movement_speed, kWaypointReached)) {
        mover.moving = aiming;
        return;
    }
    if (--mover.replan <= 0 || Distance2(target, mover.planned_for) > kReplanMove || Distance2(at, mover.waypoint) < kWaypointReached) {
        float reached[3];
        float ground = mover.arena ? mover.anchor[1] : at[1];
        NavigateTowards(at, target, Clearance(unit, monster), ground, mover.waypoint, reached);
        std::copy(target, target + 3, mover.planned_for);
        mover.replan = kReplanSteps;
    }
    float dx = mover.waypoint[0] - at[0];
    float dz = mover.waypoint[2] - at[2];
    float across = std::sqrt(dx * dx + dz * dz);
    if (across < 0.01f) {
        return;
    }
    // Up or down as the script meant, spread over the way to its target.
    float rise = (target[1] - at[1]) / std::max(Distance2(at, target), 1.0f) * across;
    float length = std::sqrt(across * across + rise * rise);
    live.movement[0] = dx / length;
    live.movement[1] = live.falls != 0 ? live.movement[1] : rise / length;
    live.movement[2] = dz / length;
    // Turning to the target turns to the way there.
    if (live.turn_speed > 0.0f && Distance2(live.turn_target, target) < 4.0f) {
        live.turn_target[0] = mover.waypoint[0];
        live.turn_target[2] = mover.waypoint[2];
    }
}

bool BossLineClear(CMonstorUnit &unit, int monster, const float *to) {
    if (monster < 0 || monster >= kFloorMonsters || !movers[monster].active) {
        return true;
    }
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    float ground = movers[monster].arena ? movers[monster].anchor[1] : at[1];
    return NavigateClear(at, to, Clearance(unit, monster), ground);
}

} // namespace expanded
