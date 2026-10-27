#include "expanded/weapons/toan/brave_ark.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/party.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

constexpr int kTimed = AILMENT_FREEZE | AILMENT_STAMINA;

} // namespace

bool HerosCourage(int chara) {
    const WEAPON_HAVE *weapon = UserStatus != nullptr ? EquippedWeapon(*UserStatus, chara) : nullptr;
    return weapon != nullptr && CarriesWeapon(*weapon, ITEM_WEAPON_BRAVE_ARK);
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
    for (int chara = 0; chara < kPartyMembers; chara++) {
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
