#include "expanded/weapons/ungaga/partisan.hpp"

#include "character.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/shamshir.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

// Ungaga's three combo swings (UngagaKey_Play's attack states, played as the motions of the same numbers).
constexpr int kFirstSwing = 0x25;
constexpr int kLastSwing = 0x27;

} // namespace

bool QuickComboWielded() {
    return WeaponWielded(ITEM_WEAPON_PARTISAN);
}

void QuickComboMotion(CCharacter &chara, int chara_no) {
    const MOTION_INFO *info = chara.motion_type.motion_info;

    if (chara_no != CHARA_UNGAGA || chara.motion_no < kFirstSwing || chara.motion_no > kLastSwing || info == nullptr ||
        !QuickComboWielded()) {
        return;
    }

    // A speed the game set itself stands.
    if (chara.motion_speed < 0.0f) {
        chara.motion_speed = info[chara.motion_no].speed * kSwiftStrikesFactor;
    }
}

} // namespace expanded
