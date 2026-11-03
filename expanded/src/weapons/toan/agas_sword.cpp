#include "expanded/weapons/toan/agas_sword.hpp"

#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

int DefensiveLegacyDefense(int chara, int defense) {
    return EquippedCarries(chara, ITEM_WEAPON_AGA_S_SWORD) ? defense + kDefensiveLegacy : defense;
}

} // namespace expanded
