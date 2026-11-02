#include "expanded/weapons/toan/drain_seeker.hpp"

#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/antique_sword.hpp"
#include "itemdata.hpp"

namespace expanded {

bool BloodScentActive() {
    return WeaponActive(ITEM_WEAPON_DRAIN_SEEKER);
}

bool BloodScentMimic(const CDungeonMap &map, int box) {
    return BloodScentActive() && !RealChest(map, box);
}

} // namespace expanded
