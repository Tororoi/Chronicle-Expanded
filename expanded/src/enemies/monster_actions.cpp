#include "expanded/enemies/monster_actions.hpp"

#include "monstorunit.hpp"

namespace expanded {

void PlayMonsterMotion(CMonstorUnit &unit, int monster, int motion, int flags) {
    float speed = unit.chara[monster][0].motion_type.motion_info[motion].speed;
    unit.monster[monster].requested_motion = static_cast<s16>(motion);
    unit.monster[monster].requested_motion_flags = static_cast<s16>(flags);
    unit.monster[monster].requested_motion_speed = speed;
    for (int i = 0; i <= unit.monster[monster].attachment_count; i++) {
        unit.chara[monster][i].SetMotion(motion, flags);
        unit.chara[monster][i].SetMotionSpeed(speed);
    }
}

void FadeMonster(CMonstorUnit &unit, int monster, float step) {
    unit.monster[monster].palette_alpha_step = step;
    unit.monster[monster].palette_delay = 0;
}

bool RemoveFadedMonster(CMonstorUnit &unit, int monster) {
    if (unit.monster[monster].palette_alpha > 0.0f) {
        return false;
    }
    RemoveMonster(unit, monster);
    return true;
}

void RemoveMonster(CMonstorUnit &unit, int monster) {
    if (unit.monster[monster].state != -1) {
        unit.monster[monster].state = -1;
        unit.alive_count--;
    }
}

} // namespace expanded
