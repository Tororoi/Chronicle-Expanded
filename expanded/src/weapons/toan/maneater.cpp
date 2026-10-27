#include "expanded/weapons/toan/maneater.hpp"

#include <algorithm>

#include "dun/gameloop.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int kMembers = 6;

// chara's equipped Maneater: the sword in Toan's hand or Super Steve carrying its sphere in Xiao's, else nullptr.
WEAPON_HAVE *Maneater(CUserStatus &status, int chara) {
    int slot = status.equipped_weapon_slot[chara];
    if (slot < 0 || slot >= 11) {
        return nullptr;
    }
    WEAPON_HAVE &weapon = status.chara_weapons[chara][slot];
    if (chara == CHARA_TOAN && weapon.item_no == ITEM_WEAPON_MANEATER && WeaponAbilitiesOn()) {
        return &weapon;
    }
    if (chara == CHARA_XIAO && weapon.item_no == ITEM_WEAPON_SUPER_STEVE && SphereWeapon(weapon) == ITEM_WEAPON_MANEATER) {
        return &weapon;
    }
    return nullptr;
}

bool Cursing(const CUserStatus &status, int chara) {
    return Maneater(const_cast<CUserStatus &>(status), chara) != nullptr;
}

// Lifting it costs nothing.
void Lifted(CUserStatus &, int) {}

constexpr CurseRule kBloodPrice = {.cursing = Cursing, .lifted = Lifted, .immune = false};

[[maybe_unused]] const bool kRule = AddCurseRule(&kBloodPrice);

int frames[kMembers] = {};

// The life the member has, the gauge's target while it moves.
int Life(const CUserStatus &status, int chara) {
    return status.life_step[chara] != 0 ? status.next_hp[chara] : status.hp[chara];
}

void Feed(CUserStatus &status, int chara) {
    WEAPON_HAVE *sword = Maneater(status, chara);
    if (sword == nullptr) {
        frames[chara] = 0;
        return;
    }
    if (++frames[chara] < kBloodPriceFrames) {
        return;
    }
    frames[chara] = 0;
    bool warning = sword->durability_f > 0.0f && sword->durability_f <= kBloodPriceWarning * sword->durability;
    bool down = (status.ailments[chara] & AILMENT_KNOCKED_OUT) != 0;
    if (!warning || down || Life(status, chara) <= 1) {
        return;
    }
    status.AddNowLife(chara, -1, 0.0f);
    sword->durability_f = std::min(sword->durability_f + 1.0f, static_cast<float>(sword->durability));
}

} // namespace

bool BloodPriceCursing(int chara) {
    return CurseOn(chara) == &kBloodPrice;
}

void BloodPriceStep() {
    if (UserStatus == nullptr) {
        return;
    }
    for (int chara = 0; chara < kMembers; chara++) {
        Feed(*UserStatus, chara);
    }
}

} // namespace expanded
