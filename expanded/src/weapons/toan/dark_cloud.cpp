#include "expanded/weapons/toan/dark_cloud.hpp"

#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

bool GuardCrushActive() {
    return WeaponActive(ITEM_WEAPON_DARK_CLOUD) || WeaponActive(ITEM_WEAPON_7THHEAVEN);
}

} // namespace expanded
