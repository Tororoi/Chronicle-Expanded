#include "expanded/weapons/toan/crysknife.hpp"

#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

int CrystalAffinityDamage(int damage) {
    return WeaponActive(ITEM_WEAPON_CRYSKNIFE) ? damage * kCrystalAffinityDamage : damage;
}

} // namespace expanded
