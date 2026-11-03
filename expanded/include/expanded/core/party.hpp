#pragma once

#include "userstatus.hpp"

namespace expanded {

// The party's members: CUserStatus's tables have a row each.
inline constexpr int kPartyMembers = sizeof(CUserStatus::hp) / sizeof(CUserStatus::hp[0]);

// A member's weapon slots: kBagSlots carried, and one more for a weapon waiting for room.
inline constexpr int kWeaponSlots = sizeof(CUserStatus::chara_weapons[0]) / sizeof(CUserStatus::chara_weapons[0][0]);
inline constexpr int kBagSlots = kWeaponSlots - 1;

// The life chara has, the gauge's target while it moves.
inline int MemberLife(const CUserStatus &status, int chara) {
    return status.life_step[chara] != 0 ? status.next_hp[chara] : status.hp[chara];
}

} // namespace expanded
