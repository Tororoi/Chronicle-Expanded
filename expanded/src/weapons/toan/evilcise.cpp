#include "expanded/weapons/toan/evilcise.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

bool Cursing(const CUserStatus &status, int chara) {
    int slot = status.equipped_weapon_slot[chara];
    if (slot < 0) {
        return false;
    }

    const WEAPON_HAVE &weapon = status.chara_weapons[chara][slot];
    if (chara == CHARA_TOAN) {
        return weapon.item_no == ITEM_WEAPON_EVILCISE && !PlainWeapons();
    }
    return chara == CHARA_XIAO && weapon.item_no == ITEM_WEAPON_SUPER_STEVE && SphereWeapon(weapon) == ITEM_WEAPON_EVILCISE;
}

// The price of lifting it: poisoned, and 1 HP left.
void Lifted(CUserStatus &status, int chara) {
    status.ailments[chara] |= AILMENT_POISON;
    status.hp[chara] = 1;
}

constexpr CurseRule kJealousSoul = {.cursing = Cursing, .lifted = Lifted, .immune = true};

[[maybe_unused]] const bool kRule = AddCurseRule(&kJealousSoul);

} // namespace

bool JealousSoulCursing(int chara) {
    return CurseOn(chara) == &kJealousSoul;
}

} // namespace expanded
