#include "expanded/weapons/toan/evilcise.hpp"

#include "dun/gameloop.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// chara's equipped Evilcise: the sword (Toan's) or Super Steve carrying its sphere (Xiao's).
bool Cursing(const CUserStatus &status, int chara) {
    const WEAPON_HAVE *weapon = EquippedWeapon(status, chara);
    return weapon != nullptr && CarriesWeapon(*weapon, ITEM_WEAPON_EVILCISE);
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
