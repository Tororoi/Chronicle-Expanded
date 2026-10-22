#include "expanded/weapons/wear.hpp"

#include "dun/gameloop.hpp"
#include "userstatus.hpp"

namespace expanded {

void WearWeapon(float base_whp) {
    if (UserStatus == nullptr || base_whp <= 0.0f) {
        return;
    }
    SwordDmgCheck1(base_whp / kSwingWear, 0);
}

} // namespace expanded
