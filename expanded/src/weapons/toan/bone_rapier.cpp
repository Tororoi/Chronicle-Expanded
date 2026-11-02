#include "expanded/weapons/toan/bone_rapier.hpp"

#include <optional>
#include <string>

#include "expanded/core/menu_text.hpp"
#include "expanded/dungeon/door_keys.hpp"
#include "expanded/enemies/revive.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

// The line when the key opens a door, a system message of its own.
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

[[maybe_unused]] const bool kSkeletonKey = (AddDoorKey({ITEM_BONE_KEY, SkeletonKeyActive, kSkeletonKeyMessage}), true);

// Every revival stays down: the revive layer asks only about monsters that can get back up.
bool Gravedigger(const CMonstorUnit &, int) {
    return GravediggerActive();
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

} // namespace expanded
