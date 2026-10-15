#include "expanded/kick_back.hpp"

#include <libvu0.h>

#include "collisiondata.hpp"
#include "expanded/baselard.hpp"
#include "monstorunit.hpp"

namespace expanded {

void KickMonster(CMonstorUnit &unit, int slot, const float *origin, KickBack kick) {
    MONSTOR      &monster = unit.monster[slot];
    sceVu0FVECTOR position, from;
    unit.chara[slot][0].GetPosition(position);
    sceVu0CopyVector(from, (float *) origin);
    position[1] = 0;
    from[1] = 0;
    sceVu0SubVector(monster.knockback_direction, position, from);
    sceVu0Normalize(monster.knockback_direction, monster.knockback_direction);
    monster.knockback[0] = kick.speed * monster.knockback[2];
    monster.knockback[1] = kick.decay * monster.knockback[2];
}

void WeaponKickBack(CMonstorUnit &unit, int slot, const COLLISION_HIT &record) {
    if (HeavyHandHit(record)) {
        // A swing throws away from Toan, a pellet away from where it struck.
        KickMonster(unit, slot, record.knockback_mode == 2 ? record.knockback_origin : record.pos, kHeavyKickBack);
    }
}

} // namespace expanded
