#include "expanded/enemies/monster_queries.hpp"

#include <cmath>

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

} // namespace expanded
