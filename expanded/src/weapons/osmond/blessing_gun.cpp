#include "expanded/weapons/osmond/blessing_gun.hpp"

#include <format>

#include "ebattle.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/ownership.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"

namespace expanded {

bool BlessedBaitKept() {
    if (!WeaponOwned(ITEM_WEAPON_BLESSING_GUN)) {
        return false;
    }
    LogEvent("bait_kept", std::format("\"item\":{},\"sank\":{}", FishingGetEsaItemNo(), chara_fishing == ED_FISHING_SINK));
    return true;
}

} // namespace expanded
