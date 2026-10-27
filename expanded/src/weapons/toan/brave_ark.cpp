#include "expanded/weapons/toan/brave_ark.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

constexpr int kMembers = 6;
constexpr int kTimed = AILMENT_FREEZE | AILMENT_STAMINA;

} // namespace

bool HerosCourage(int chara) {
    if (UserStatus == nullptr || chara < 0 || chara >= kMembers) {
        return false;
    }
    int slot = UserStatus->equipped_weapon_slot[chara];
    if (slot < 0 || slot >= 11) {
        return false;
    }
    const WEAPON_HAVE &weapon = UserStatus->chara_weapons[chara][slot];
    return (weapon.item_no == ITEM_WEAPON_BRAVE_ARK && !PlainWeapons()) ||
           (weapon.item_no == ITEM_WEAPON_SUPER_STEVE && SphereWeapon(weapon) == ITEM_WEAPON_BRAVE_ARK);
}

void HerosCourageHit(COLLISION_HIT &hit) {
    if (UserStatus != nullptr && HerosCourage(UserStatus->cur_chara)) {
        hit.flags &= ~kHerosCourageHit;
    }
}

bool HerosCourageAllowsAilment(int ailment) {
    return UserStatus == nullptr || (ailment & kHerosCourage) == 0 || !HerosCourage(UserStatus->cur_chara);
}

void HerosCourageStep() {
    if (UserStatus == nullptr) {
        return;
    }
    for (int chara = 0; chara < kMembers; chara++) {
        if ((UserStatus->ailments[chara] & kHerosCourage) == 0 || !HerosCourage(chara)) {
            continue;
        }
        UserStatus->ailments[chara] &= ~kHerosCourage;
        if ((UserStatus->ailments[chara] & kTimed) == 0) {
            UserStatus->ailment_frames[chara] = 0;
        }
    }
}

} // namespace expanded
