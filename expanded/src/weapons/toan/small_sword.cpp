#include "expanded/weapons/toan/small_sword.hpp"

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The opening swing (action 0x24): its step-in is set at 820 to 820.5, its trail at 824 and its hit at 825.
constexpr int   kOpeningSwing = 0x24;
constexpr float kSwingSteppedIn = 820.5f;
constexpr float kSwingTrail = 824.0f;

// Xiao's shot: the draw, the hold it stops in while the button is down and the shot, whose pellet leaves between
// 251 and 252.
constexpr int   kShotDraw = 11;
constexpr int   kShotHold = 12;
constexpr int   kShotFire = 13;
constexpr float kShotRelease = 251.0f;

bool QuickDrawWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_SMALL_SWORD || weapon_no == ITEM_WEAPON_TSUKIKAGE || weapon_no == ITEM_WEAPON_HEAVEN_S_CLOUD;
}

} // namespace

bool QuickDrawSword() {
    return QuickDrawWeapon(HeldWeapon());
}

bool QuickDrawSphere() {
    return QuickDrawWeapon(SuperSteveSphere());
}

void QuickDrawSwing(CCharacter &chara) {
    float &time = chara.motion_type.state.time;
    if (BtActStatus.action_no == kOpeningSwing && time >= kSwingSteppedIn && time < kSwingTrail && QuickDrawSword()) {
        time = kSwingTrail;
    }
}

void QuickDrawShot(CCharacter &chara, int aimed) {
    if (aimed != 0 || (BtActStatus.action_no != kShotDraw && BtActStatus.action_no != kShotHold) || GamePad.On(PadInput_OK) != 0 || !QuickDrawSphere()) {
        return;
    }

    BtActStatus.action_step = 1;
    BtActStatus.action_no = kShotFire;
    BtActStatus.motion_no = kShotFire;
    driveNoInterpolate = 1;
    chara.motion_type.state.time = kShotRelease;
}

int QuickDrawGaugeSpeed(int speed) {
    if (UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && QuickDrawSphere()) {
        return kQuickDrawShotSpeed;
    }
    return speed;
}

} // namespace expanded
