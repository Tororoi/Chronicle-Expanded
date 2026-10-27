#include "expanded/weapons/player_state.hpp"

#include "btactstatus.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"

extern s32 lockOnTargetNo;

namespace expanded {

bool Guarding() {
    return BtActStatus.guard_mode == kGuardUp;
}

int LockedTarget() {
    if (lockOnTargetFlag == 0 || lockOnTargetNo < 0 || lockOnTargetNo >= kFloorMonsters || NowMonstorUnit == nullptr) {
        return -1;
    }
    const MONSTOR &live = NowMonstorUnit->monster[lockOnTargetNo];
    return live.state != -1 && live.hp > 0 ? lockOnTargetNo : -1;
}

} // namespace expanded
