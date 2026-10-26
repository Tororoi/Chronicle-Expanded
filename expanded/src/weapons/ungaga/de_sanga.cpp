#include "expanded/weapons/ungaga/de_sanga.hpp"

#include <algorithm>

#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

// The kill is the active character's as the game's kill ABS is: the monster's last_attacker. The heal goes to the
// equipped record's WHP, the one the HUD shows and the game's wear takes from, and never lowers a WHP above the max.
void VampireKill(const CMonstorUnit &unit, int monster) {
    if (UserStatus == nullptr || !WeaponActive(ITEM_WEAPON_DE_SANGA)) {
        return;
    }
    int chara = UserStatus->cur_chara;
    if (unit.monster[monster].last_attacker != chara) {
        return;
    }
    int          slot = UserStatus->equipped_weapon_slot[chara];
    WEAPON_HAVE &weapon = UserStatus->chara_weapons[chara][slot];
    float        healed = std::min(static_cast<float>(weapon.durability), weapon.durability_f + kVampireHeal);
    weapon.durability_f = std::max(weapon.durability_f, healed);
}

} // namespace expanded
