#pragma once

#include "userstatus.hpp"

namespace expanded {

// The party's members: CUserStatus's tables have a row each.
inline constexpr int kPartyMembers = sizeof(CUserStatus::hp) / sizeof(CUserStatus::hp[0]);

// A member's weapon slots: kBagSlots carried, and one more for a weapon waiting for room.
inline constexpr int kWeaponSlots = sizeof(CUserStatus::chara_weapons[0]) / sizeof(CUserStatus::chara_weapons[0][0]);
inline constexpr int kBagSlots = kWeaponSlots - 1;

} // namespace expanded
