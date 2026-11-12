#include "expanded/weapons/xiao/moves_while_firing.hpp"

#include "btactstatus.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

// BtActStatus.action_on while a member's weapon action plays (Xiao's: her shot).
constexpr int kWeaponAction = 1;

} // namespace

bool OsmondWeapon(int weapon_no) {
    const WEAPON_DATA *data = weapon_no >= ITEM_WEAPON_START ? GetWeaponData(weapon_no) : nullptr;
    return data != nullptr && data->owner == CHARA_OSMOND;
}

int OsmondGunType(int weapon_no) {
    if (!OsmondWeapon(weapon_no)) {
        return -1;
    }
    switch (weapon_no) {
        case ITEM_WEAPON_HEXA_BLASTER:
        case ITEM_WEAPON_SUPERNOVA:
        case ITEM_WEAPON_SNAIL:
            return kLauncher;
        case ITEM_WEAPON_BLESSING_GUN:
        case ITEM_WEAPON_SKUNK:
            return kFlamethrower;
        default:
            return kMachineGun;
    }
}

bool MovesWhileFiring() {
    return OsmondWeapon(SuperSteveSphere());
}

bool FiringLeavesMove() {
    return Playing(CHARA_XIAO) && BtActStatus.action_on == kWeaponAction && MovesWhileFiring();
}

} // namespace expanded
