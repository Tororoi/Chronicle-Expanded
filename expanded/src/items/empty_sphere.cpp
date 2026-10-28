#include "expanded/items/empty_sphere.hpp"

#include <optional>
#include <string>

#include "battlemenu.hpp"
#include "dngstatusdata.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/core/party.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/seven_branch_sword.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"

namespace expanded {

namespace {

// The level the game asks for a Status Break (WeaponStatusBreakEnable).
constexpr int kBreakLevel = 5;

// The weapon the weapon menu has selected, or nullptr.
const WEAPON_HAVE *Selected() {
    if (BtlMenuStatusPt == nullptr || WepMenu.chara < 0 || WepMenu.chara >= kPartyMembers || WepMenu.weapon_slot < 0 ||
        WepMenu.weapon_slot >= kWeaponSlots) {
        return nullptr;
    }
    return &BtlMenuStatusPt->chara_weapons[WepMenu.chara][WepMenu.weapon_slot];
}

std::optional<std::string> SlotWarning() {
    return LocalizeText("expanded.empty_sphere.slot_one", "An empty SynthSphere\nonly fits the\nfirst slot.");
}

// A break only the empty sphere allows spends it: the game's confirmation asks for no attachment.
std::optional<std::string> Confirm() {
    const WEAPON_HAVE *weapon = Selected();
    if (weapon == nullptr || weapon->level >= kBreakLevel || !EmptySphereBreaks(*weapon) || SevenfoldRefuses(*weapon)) {
        return std::nullopt;
    }
    return LocalizeText("expanded.empty_sphere.break_confirm",
                        "{alert}Seal 60% weapon\nability to sphere.\n(Uses empty sphere.)\n Do status break?");
}

[[maybe_unused]] const bool kRegistered =
    (SetMenuMessage(kEmptySphereSlotMessage, SlotWarning), SetMenuMessage(kStatusBreakConfirmMessage, Confirm), true);

} // namespace

bool EmptySphere(const ATTACH_LIST &attach) {
    return attach.item_no == ITEM_ATTACH_SYNTHESIS_SPHERE && attach.sphere_weapon_no == 0;
}

bool EmptySphereBreaks(const WEAPON_HAVE &weapon) {
    return WeaponAbilitiesOn() && EmptySphere(weapon.attach[0]);
}

bool EmptySphereWarns(const ATTACH_LIST &held) {
    return WeaponAbilitiesOn() && EmptySphere(held);
}

} // namespace expanded
