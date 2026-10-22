#include "expanded/weapons/toan/sun_sword.hpp"

#include "expanded/core/random.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

bool SolarHarvestActive() {
    return WeaponActive(ITEM_WEAPON_SUN_SWORD) || WeaponActive(ITEM_WEAPON_BIG_BANG) || WeaponActive(ITEM_WEAPON_SWORD_OF_ZEUS);
}

int SolarHarvestDrop(const CMonstorUnit &, int, int drop_item) {
    if (drop_item != -1 || !SolarHarvestActive() || ModRoll(100) >= kSolarHarvestPercent) {
        return drop_item;
    }
    return ITEM_ATTACH_SUN;
}

} // namespace expanded
