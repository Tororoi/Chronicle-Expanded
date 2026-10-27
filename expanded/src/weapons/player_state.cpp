#include "expanded/weapons/player_state.hpp"

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

extern s32 lockOnTargetNo;

namespace expanded {

bool Playing(int chara) {
    return UserStatus != nullptr && UserStatus->cur_chara == chara;
}

float PlayerYaw() {
    float rotation[4];
    CharaMain.GetRotation(rotation);
    return rotation[1];
}

bool Guarding() {
    return BtActStatus.guard_mode == kGuardUp;
}

int LockedTarget() {
    const MONSTOR *live = lockOnTargetFlag != 0 ? FloorMonster(lockOnTargetNo) : nullptr;
    return live != nullptr && MonsterLiving(*live) ? lockOnTargetNo : -1;
}

int SelectedElement() {
    if (NowWeaponHave == nullptr) {
        return -1;
    }
    int element = NowWeaponHave->best_elem;
    return element >= WEAPON_ELEMENT_FIRE && element <= WEAPON_ELEMENT_HOLY ? element : -1;
}

} // namespace expanded
