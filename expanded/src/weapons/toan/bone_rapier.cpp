#include "expanded/weapons/toan/bone_rapier.hpp"

#include <optional>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/enemies/revive.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "localize.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "sysmes.hpp"

namespace expanded {

namespace {

// The line when the key opens a door, a system message of its own.
constexpr int kSkeletonKeyMessage = 1002;
constexpr int kSkeletonKeyFrames = 240;

std::optional<std::string> SkeletonKeyLine() {
    return LocalizeText("expanded.bone_rapier.door", "You can hear an ominous voice laughing 'Rattle me bones!'");
}

[[maybe_unused]] const bool kSkeletonKeyLine = (SetSystemMessage(kSkeletonKeyMessage, SkeletonKeyLine), true);

bool BoneWeapon(int weapon_no) {
    return weapon_no == ITEM_WEAPON_BONE_RAPIER || weapon_no == ITEM_WEAPON_BONE_SLINGSHOT;
}

bool InHand() {
    return WeaponWielded(ITEM_WEAPON_BONE_RAPIER) || WeaponWielded(ITEM_WEAPON_BONE_SLINGSHOT);
}

// The undead whose own death script revives them stay down.
bool Gravedigger(const CMonstorUnit &unit, int monster_no) {
    const ReviveSpecies *species = ReviveSpeciesOf(unit.model[unit.monster[monster_no].base_model].script_name);
    return species != nullptr && species->revival == Revival::kScript && GravediggerActive();
}

[[maybe_unused]] const bool kGravedigger = (AddStaysDownRule(Gravedigger), true);

} // namespace

bool SkeletonKeyActive() {
    return InHand() || BoneWeapon(SuperSteveSphere());
}

bool GravediggerActive() {
    int sphere = SuperSteveSphere();
    return InHand() || BoneWeapon(sphere) || sphere == ITEM_WEAPON_CROSS_HINDER;
}

bool SkeletonKeyAnswers(const int *items, RS_STACKDATA *result) {
    if (result == nullptr || items[0] != ITEM_BONE_KEY || items[1] != -1 || !SkeletonKeyActive()) {
        return false;
    }

    result->i = ITEM_BONE_KEY;
    BtEventInfo.item_select_result = 0;
    int args[4] = {-1, -1, -1, -1};
    SetSystemMes(kSkeletonKeyMessage, kSkeletonKeyFrames, MES_POS_BOTTOM, 0, args, nullptr);
    return true;
}

} // namespace expanded
