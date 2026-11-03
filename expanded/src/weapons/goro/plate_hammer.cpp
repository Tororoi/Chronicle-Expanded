#include "expanded/weapons/goro/plate_hammer.hpp"

#include <algorithm>
#include <format>

#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

void TemperingRepaired(WEAPON_HAVE &weapon) {
    if (!CarriesWeapon(weapon, ITEM_WEAPON_PLATE_HAMMER) || weapon.durability >= kTemperingMost) {
        return;
    }
    weapon.durability = static_cast<s16>(std::min(kTemperingMost, weapon.durability + kTemperingGain));
    if (NowWeaponHave != nullptr && NowWeaponHave != &weapon) {
        // The copy in play, with its attachments' values: its max as the record's.
        NowWeaponHave->durability = weapon.durability;
    }
    LogEvent("tempered", std::format("\"weapon\":{},\"whp_max\":{}", weapon.item_no, weapon.durability));
}

} // namespace expanded
