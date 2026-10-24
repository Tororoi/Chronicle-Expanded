#include "expanded/weapons/toan/seven_branch_sword.hpp"

#include <optional>
#include <string>

#include "battlemenu.hpp"
#include "dngstatusdata.hpp"
#include "expanded/core/menu_text.hpp"
#include "itemdata.hpp"
#include "localize.hpp"

namespace expanded {

namespace {

// The weapon the weapon menu has selected, or nullptr.
const WEAPON_HAVE *Selected() {
    if (BtlMenuStatusPt == nullptr || WepMenu.chara < 0 || WepMenu.chara >= 6 || WepMenu.weapon_slot < 0 ||
        WepMenu.weapon_slot >= 11) {
        return nullptr;
    }
    return &BtlMenuStatusPt->chara_weapons[WepMenu.chara][WepMenu.weapon_slot];
}

bool SevenBranchSelected() {
    const WEAPON_HAVE *weapon = Selected();
    return weapon != nullptr && weapon->item_no == ITEM_WEAPON_7BRANCH_SWORD;
}

std::optional<std::string> Hint() {
    if (!SevenBranchSelected()) {
        return std::nullopt;
    }
    return LocalizeText("expanded.seven_branch.break_hint", "\"Status break\"\nFrom +7, keeps\n77% in \"SynthSphere\".");
}

std::optional<std::string> Confirm() {
    if (!SevenBranchSelected()) {
        return std::nullopt;
    }
    return LocalizeText("expanded.seven_branch.break_confirm",
                        "{alert}Seal 77% weapon\nability to sphere.\n(Do w/o attachment.)\n Do status break?");
}

std::optional<std::string> Refused() {
    return LocalizeText("expanded.seven_branch.break_refused", "The 7 Branch Sword\nrefuses a status break\nbelow +7.");
}

[[maybe_unused]] const bool kRegistered = (SetMenuMessage(kStatusBreakHintMessage, Hint),
                                           SetMenuMessage(kStatusBreakConfirmMessage, Confirm),
                                           SetMenuMessage(kSevenfoldRefusedMessage, Refused), true);

} // namespace

bool SevenfoldRefuses(const WEAPON_HAVE &weapon) {
    return weapon.item_no == ITEM_WEAPON_7BRANCH_SWORD && weapon.level < kSevenfoldLevel;
}

float StatusBreakShare(const WEAPON_HAVE &weapon) {
    return weapon.item_no == ITEM_WEAPON_7BRANCH_SWORD ? kSevenfoldShare : kStatusBreakShare;
}

} // namespace expanded
