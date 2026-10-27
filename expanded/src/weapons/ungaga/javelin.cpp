#include "expanded/weapons/ungaga/javelin.hpp"

#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

bool Active() {
    return WeaponActive(ITEM_WEAPON_JAVELIN);
}

bool Hunted(const CMonstorUnit &unit, int monster) {
    return Active() && unit.model[unit.monster[monster].base_model].attachment_kind == MONSTER_FAMILY_SEA;
}

} // namespace

int MarineHunterDefense(const CMonstorUnit &unit, int monster, int defense) {
    return Hunted(unit, monster) ? 0 : defense;
}

int MarineHunterHardness(const CMonstorUnit &unit, int monster, int hardness) {
    return Hunted(unit, monster) ? 0 : hardness;
}

int MarineHunterAbs(const CMonstorUnit &unit, int monster, int abs) {
    return Hunted(unit, monster) ? abs * kMarineHunterAbs : abs;
}

} // namespace expanded
