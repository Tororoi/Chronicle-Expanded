#include "expanded/weapons/goro/magical_hammer.hpp"

#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"

namespace expanded {

bool BaitWard() {
    return WeaponOwned(ITEM_WEAPON_MAGICAL_HAMMER);
}

} // namespace expanded
