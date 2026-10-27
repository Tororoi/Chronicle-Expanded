#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/toan/shamshir.hpp"

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

bool SwiftStrikesWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_SHAMSHIR || weapon_no == ITEM_WEAPON_DUSACK || weapon_no == ITEM_WEAPON_7BRANCH_SWORD ||
           weapon_no == ITEM_WEAPON_ATLAMILLIA_SWORD || weapon_no == ITEM_WEAPON_CHRONICLE_SWORD;
}

// The speed a motion's own entry gives it, or 0 where it has none.
float TableSpeed(const CCharacter &chara, int motion_no) {
    const MOTION_INFO *info = chara.motion_type.motion_info;
    return info != nullptr ? info[motion_no].speed : 0.0f;
}

} // namespace

bool SwiftStrikesSword() {
    return SwiftStrikesWeapon(HeldWeapon());
}

bool SwiftStrikesSphere() {
    int sphere = SuperSteveSphere();
    return SwiftStrikesWeapon(sphere) || sphere == ITEM_WEAPON_PARTISAN;
}

void SwiftStrikesMotion(CCharacter &chara, int chara_no) {
    int   motion_no = chara.motion_no;
    float speed = 0.0f;

    if (chara_no == CHARA_TOAN && motion_no >= kToanFirstSwing && motion_no <= kToanLastSwing && SwiftStrikesSword()) {
        speed = TableSpeed(chara, motion_no) * kSwiftStrikesFactor;
    } else if (chara_no == CHARA_XIAO && motion_no == kXiaoDraw && SwiftStrikesSphere()) {
        speed = TableSpeed(chara, motion_no) * kSwiftDrawFactor;
    } else if (chara_no == CHARA_XIAO && motion_no == kXiaoShot && SwiftStrikesSphere()) {
        speed = kSwiftShotSpeed;
    }

    // A speed the game set itself stands.
    if (speed > 0.0f && chara.motion_speed < 0.0f) {
        chara.motion_speed = speed;
    }
}

} // namespace expanded
