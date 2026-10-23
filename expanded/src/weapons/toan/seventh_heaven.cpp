#include "expanded/weapons/toan/seventh_heaven.hpp"

#include "btactstatus.hpp"
#include "collisiondata.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

bool DivineGuardActive() {
    return WeaponActive(ITEM_WEAPON_7THHEAVEN);
}

void DivineGuardHit(COLLISION_HIT &hit) {
    if (hit.kind == kGuardBreakingHit && DivineGuardActive()) {
        hit.kind = kKnockbackHit;
    }
}

void DivineGuardBlocked(bool blocked) {
    if (!blocked || !DivineGuardActive()) {
        return;
    }
    BtActStatus.action_on = kGuardAction;
    BtActStatus.action_no = kGuardHold;
    BtActStatus.motion_no = kGuardHold;
    BtActStatus.guard_mode = kGuardHeld;
}

} // namespace expanded
