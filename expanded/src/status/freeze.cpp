#include "expanded/status/freeze.hpp"

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "userstatus.hpp"

namespace expanded {

void BreakFreeze(COLLISION_HIT *hit) {
    int chara = UserStatus->cur_chara;

    if (UserStatus->ailments[chara] & AILMENT_FREEZE) {
        UserStatus->ailments[chara] &= ~AILMENT_FREEZE;
        UserStatus->ailment_frames[chara] = 0;
        BtActStatus.movement_locked = 0;
        hit->flags &= ~(0x100 | 0x100000);
    }
}

} // namespace expanded
