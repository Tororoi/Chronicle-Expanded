#include "expanded/weapons/osmond/skunk.hpp"

#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

float FlameSpacing(float spacing) {
    return WeaponWielded(ITEM_WEAPON_SKUNK) ? spacing * kLongerFlame : spacing;
}

float FlamePuffSize(float size) {
    return WeaponWielded(ITEM_WEAPON_SKUNK) ? size * kFlamePuffScale : size;
}

} // namespace expanded
