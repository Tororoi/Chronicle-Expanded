#include "expanded/weapons/solid_column.hpp"

#include <cmath>

#include "bt_shot_effect.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"

namespace expanded {

namespace {

constexpr int   kPlayerShot = 2; // a shot aimed at the monsters
constexpr float kHeadOn = 0.01f; // what is left along the column, squared, below which the slide takes the perpendicular

struct Column {
    bool        armed = false;
    float       x = 0.0f, floor = 0.0f, z = 0.0f;
    float       radius = 0.0f, top = 0.0f;
    const void *owner = nullptr;
};

Column column;

void OnFloor() {
    column = Column{};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

} // namespace

void SolidColumnArm(const float *base, float radius, float top, const void *owner) {
    column = Column{true, base[0], base[1], base[2], radius, top, owner};
}

void SolidColumnDisarm(const void *owner) {
    if (column.owner == owner) {
        column = Column{};
    }
}

bool SolidColumnArmed() {
    return column.armed;
}

void SolidColumnMonsterMove(CMonstorUnit &unit, int monster) {
    MONSTOR &live = unit.monster[monster];
    float    speed = live.movement_speed;
    if (!column.armed || speed == 0.0f) {
        return; // not moving, or already stopped
    }
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    float dx = live.movement[0], dz = live.movement[2];
    float nx = at[0] + dx * speed - column.x, nz = at[2] + dz * speed - column.z;
    float reach = column.radius + live.collision_radius;
    if (reach * reach < nx * nx + nz * nz) {
        return; // its next step is clear of it
    }
    float vx = column.x - at[0], vz = column.z - at[2];
    if (!(0.0f < vx * dx + vz * dz)) {
        return; // heading away, or across
    }
    float vv = vx * vx + vz * vz;
    float k = (dx * vx + dz * vz) / vv;
    float sx = dx - k * vx, sz = dz - k * vz;
    float along = sx * sx + sz * sz;
    if (along < kHeadOn) {
        sx = -vz, sz = vx, along = vv;
    }
    float length = std::sqrt(along);
    live.movement[0] = sx / length;
    live.movement[2] = sz / length;
}

void SolidColumnPlayerMove(const float *position, float *velocity) {
    if (!column.armed) {
        return;
    }
    float nx = position[0] + velocity[0] - column.x, nz = position[2] + velocity[2] - column.z;
    float reach = column.radius + kColumnPlayerReach;
    if (reach * reach < nx * nx + nz * nz) {
        return;
    }
    float vx = column.x - position[0], vz = column.z - position[2];
    float into = velocity[0] * vx + velocity[2] * vz;
    if (!(0.0f < into)) {
        return; // moving away, or across
    }
    float k = into / (vx * vx + vz * vz);
    velocity[0] -= k * vx;
    velocity[2] -= k * vz;
}

SHOT_COLLISION_RESULT SolidColumnShot(const CSHOT_EFFECT &effect, int slot, SHOT_COLLISION_RESULT result, float *hit_position,
                                      const float *position) {
    if (result != SHOT_COLLISION_NONE || !column.armed || effect.effect_data == nullptr || effect.effect_data->target == kPlayerShot) {
        return result;
    }
    const float *velocity = effect.velocity[slot];
    float        next[3] = {position[0] + velocity[0], position[1] + velocity[1], position[2] + velocity[2]};
    if (next[1] < column.floor - kColumnShotBelow || column.top < next[1]) {
        return result;
    }
    float reach = column.radius + effect.effect_data->radius[effect.phase[slot]];
    float dx = next[0] - column.x, dz = next[2] - column.z;
    if (reach * reach < dx * dx + dz * dz) {
        return result;
    }
    // A wall: stopped where it is.
    hit_position[0] = position[0];
    hit_position[1] = position[1];
    hit_position[2] = position[2];
    return SHOT_COLLISION_MAP;
}

} // namespace expanded
