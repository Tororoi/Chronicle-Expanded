#include "expanded/weapons/toan/gladius.hpp"

#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace expanded {

int JacketHunterAbs(const CMonstorUnit &unit, int monster, int abs) {
    // The slot is released as its kill is paid: its kind, not whether it is alive.
    int  kind = ModelKind(unit.monster[monster].base_model);
    bool jacket = kind == No(Monster::MASTER_JACKET) || kind == No(Monster::MASTER_JACKET_ENHANCED);
    return jacket && WeaponActive(ITEM_WEAPON_GLADIUS) ? abs * kJacketHunterAbs : abs;
}

} // namespace expanded
