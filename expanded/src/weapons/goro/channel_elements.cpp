#include "expanded/weapons/goro/channel_elements.hpp"

#include <algorithm>
#include <optional>
#include <string>

#include "battlemenu.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "motionmodel.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace expanded {

namespace {

constexpr int   kElements = 5;
constexpr float kMagicRate = 0.005f;   // the game's: the magic part's share of Magic
constexpr float kElementRate = 0.004f; // and of an element's points

// The warning 0 the weapon menu shows on the element page reads as Channel Elements' while a channeling hammer is
// the menu's selection.
std::optional<std::string> ChannelWarning() {
    const WEAPON_HAVE *weapon = MenuWeapon();
    if (weapon == nullptr || !ChannelElements(*weapon)) {
        return std::nullopt;
    }
    return LocalizeText("expanded.magical_hammer.channel", "All five elements\nare channeled\nat once.");
}

[[maybe_unused]] const bool kRegistered = (SetMenuMessage(kChannelMessage, ChannelWarning), true);

void Hold(WEAPON_HAVE &weapon) {
    if (ChannelElements(weapon)) {
        weapon.best_elem = WEAPON_ELEMENT_NONE;
    }
}

} // namespace

bool ChannelElements(const WEAPON_HAVE &weapon) {
    return CarriesWeapon(weapon, ITEM_WEAPON_MAGICAL_HAMMER);
}

int AttackElementFlags(int element) {
    if (NowWeaponHave != nullptr && ChannelElements(*NowWeaponHave)) {
        return kChannelFlags;
    }
    return GetWeaponElementAttr(element);
}

bool ChannelHit(int element_flags) {
    return element_flags == kChannelFlags;
}

float ChannelDamage(float damage, int magic, const s8 *elem, const s16 *resist) {
    float weakest = 0.0f;
    float magic_parts = 0.0f;
    for (int e = 0; e < kElements; e++) {
        float rate = 0.01f * static_cast<float>(resist[e]);
        weakest = std::max(weakest, rate);
        if (rate <= 0.0f) {
            continue; // immune: this element adds nothing
        }
        magic_parts += damage * (kMagicRate * static_cast<float>(magic) + kElementRate * static_cast<float>(elem[e])) * rate;
    }
    return damage * weakest + magic_parts;
}

int ChannelStrongest(const WEAPON_HAVE &weapon) {
    int strongest = 0;
    for (int e = 1; e < kElements; e++) {
        if (weapon.elem[e] > weapon.elem[strongest]) {
            strongest = e;
        }
    }
    return strongest;
}

bool ChannelChosen(const WEAPON_HAVE &weapon, int element) {
    return ChannelElements(weapon) && element >= 0 && element < kElements;
}

bool ChannelRefused(const WEAPON_HAVE &weapon) {
    return ChannelElements(weapon);
}

void HoldChannelElements(CUserStatus &status) {
    for (auto &member : status.chara_weapons) {
        for (WEAPON_HAVE &weapon : member) {
            Hold(weapon);
        }
    }
    if (NowWeaponHave != nullptr) {
        Hold(*NowWeaponHave);
    }
}

void ChannelElementsStep() {
    if (UserStatus != nullptr) {
        HoldChannelElements(*UserStatus);
    }
}

void ChannelElementsMenuStep() {
    if (BtlMenuStatusPt != nullptr) {
        HoldChannelElements(*BtlMenuStatusPt);
    }
}

void ChannelElementsBuiltUp(WEAPON_HAVE &weapon, int from_item) {
    if (from_item == ITEM_WEAPON_MAGICAL_HAMMER && WeaponAbilitiesOn()) {
        weapon.best_elem = WEAPON_ELEMENT_NONE;
    }
}

} // namespace expanded
