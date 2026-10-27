#include "expanded/enemies/monster_queries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"

namespace expanded {

bool MonsterLiving(const MONSTOR &monster) {
    return monster.state != -1 && monster.hp > 0;
}

bool MonsterInPlay(const MONSTOR &monster) {
    return MonsterLiving(monster) && monster.revealed == -1;
}

bool MonsterShown(const MONSTOR &monster) {
    return monster.state != -1 && monster.revealed != 0;
}

const MONSTOR *FloorMonster(int monster) {
    if (NowMonstorUnit == nullptr || monster < 0 || monster >= kFloorMonsters) {
        return nullptr;
    }
    return &NowMonstorUnit->monster[monster];
}

int NearestMonster(CMonstorUnit &unit, const float *point, float reach) {
    int   nearest = -1;
    float best = reach;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!MonsterLiving(unit.monster[monster])) {
            continue;
        }
        float at[4];
        unit.chara[monster][0].GetPosition(at);
        float distance = std::hypot(at[0] - point[0], at[2] - point[2]);
        if (distance <= best) {
            best = distance;
            nearest = monster;
        }
    }
    return nearest;
}

int MonsterTouching(const CMonstorUnit &unit, const float *point, float reach, int *sphere) {
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!MonsterShown(unit.monster[monster])) {
            continue;
        }
        const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
        for (int s = 0; s < kMonsterSpheres; s++) {
            if (body.timer[s] != 0 && DistVector(const_cast<float *>(body.position[s]), const_cast<float *>(point)) <= reach + body.radius[s]) {
                if (sphere != nullptr) {
                    *sphere = s;
                }
                return monster;
            }
        }
    }
    return -1;
}

float MonsterBodyEdge(const CMonstorUnit &unit, int monster, float x, float z) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       at[4];
    const_cast<CMonstorUnit &>(unit).chara[monster][0].GetPosition(at);
    bool  posed = false;
    float nearest = 0.0f;
    for (int i = 0; i < kMonsterSpheres; i++) {
        const float *centre = body.position[i];
        if (body.timer[i] == 0 || std::fabs(centre[0] - at[0]) > kUnposedSphere || std::fabs(centre[2] - at[2]) > kUnposedSphere) {
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

float MonsterBodyCentre(const CMonstorUnit &unit, int monster, float *centre) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       at[4];
    const_cast<CMonstorUnit &>(unit).chara[monster][0].GetPosition(at);
    centre[0] = at[0], centre[1] = at[1] + kBodyFallback, centre[2] = at[2], centre[3] = 1.0f;
    float radius = kBodyFallback, best = 0.0f;
    for (int i = 0; i < kMonsterSpheres; i++) {
        const float *sphere = body.position[i];
        if (body.timer[i] == 0 || body.radius[i] <= best || std::fabs(sphere[0] - at[0]) > kUnposedSphere ||
            std::fabs(sphere[2] - at[2]) > kUnposedSphere) {
            continue;
        }
        best = radius = body.radius[i];
        std::copy(sphere, sphere + 3, centre);
    }
    return radius;
}

void MonsterAimPoint(const CMonstorUnit &unit, int monster, float x, float z, float *point) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       at[4];
    const_cast<CMonstorUnit &>(unit).chara[monster][0].GetPosition(at);
    point[0] = at[0], point[1] = at[1], point[2] = at[2], point[3] = 1.0f;
    bool  posed = false;
    float nearest = 0.0f, radius = 0.0f;
    for (int i = 0; i < kMonsterSpheres; i++) {
        const float *centre = body.position[i];
        if (body.timer[i] == 0 || std::fabs(centre[0] - at[0]) > kUnposedSphere || std::fabs(centre[2] - at[2]) > kUnposedSphere) {
            continue;
        }
        float edge = std::hypot(x - centre[0], z - centre[2]) - body.radius[i];
        if (!posed || edge < nearest) {
            nearest = edge;
            radius = body.radius[i];
            point[0] = centre[0];
            point[2] = centre[2];
        }
        posed = true;
    }
    float dx = x - point[0], dz = z - point[2], d = std::hypot(dx, dz), depth = radius - kAimDepth;
    if (posed && depth > 0.0f && d > depth) {
        point[0] += dx / d * depth;
        point[2] += dz / d * depth;
    }
}

float MonsterBodyUnderside(const CMonstorUnit &unit, int monster) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       at[4];
    const_cast<CMonstorUnit &>(unit).chara[monster][0].GetPosition(at);
    bool  posed = false;
    float lowest = 0.0f;
    for (int i = 0; i < kMonsterSpheres; i++) {
        const float *centre = body.position[i];
        if (body.timer[i] == 0 || std::fabs(centre[0] - at[0]) > kUnposedSphere || std::fabs(centre[2] - at[2]) > kUnposedSphere) {
            continue;
        }
        float bottom = centre[1] - body.radius[i];
        lowest = posed ? std::min(lowest, bottom) : bottom;
        posed = true;
    }
    return posed ? lowest : at[1];
}

float MonsterBodyDistance(const CMonstorUnit &unit, int monster, const float *point) {
    const MONSTOR_EFFECT_STATE &body = unit.effect[monster];
    float                       at[4];
    const_cast<CMonstorUnit &>(unit).chara[monster][0].GetPosition(at);
    float nearest = std::numeric_limits<float>::max();
    for (int i = 0; i < kMonsterSpheres; i++) {
        const float *centre = body.position[i];
        if (body.timer[i] == 0 || std::fabs(centre[0] - at[0]) > kUnposedSphere || std::fabs(centre[2] - at[2]) > kUnposedSphere) {
            continue;
        }
        float dx = centre[0] - point[0], dy = centre[1] - point[1], dz = centre[2] - point[2];
        nearest = std::min(nearest, std::sqrt(dx * dx + dy * dy + dz * dz) - body.radius[i]);
    }
    return nearest;
}

} // namespace expanded
