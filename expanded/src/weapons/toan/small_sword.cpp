#include "expanded/weapons/toan/small_sword.hpp"

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/swings.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The opening swing (kToanFirstSwing): its step-in is set at 820 to 820.5, its trail at kToanFirstSwingTrail and its hit
// at 825.
constexpr float kSwingSteppedIn = 820.5f;

// Xiao's shot (kXiaoShot): its pellet leaves between 251 and 252.
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
    if (BtActStatus.action_no == kToanFirstSwing && time >= kSwingSteppedIn && time < kToanFirstSwingTrail && QuickDrawSword()) {
        time = kToanFirstSwingTrail;
    }
}

void QuickDrawShot(CCharacter &chara, int aimed) {
    if (aimed != 0 || (BtActStatus.action_no != kXiaoDraw && BtActStatus.action_no != kXiaoHold) || GamePad.On(PadInput_OK) != 0 || !QuickDrawSphere()) {
        return;
    }

    BtActStatus.action_step = 1;
    BtActStatus.action_no = kXiaoShot;
    BtActStatus.motion_no = kXiaoShot;
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
