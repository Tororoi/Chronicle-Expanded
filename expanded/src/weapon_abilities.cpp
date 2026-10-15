#include "expanded/weapon_abilities.hpp"

#include <random>

#include "itemdata.hpp"

namespace expanded {

namespace {

// An ability a weapon gains, in chance percent of the weapons built.
struct WeaponAbility {
    int weapon;
    int ability;
    int chance;
};

constexpr WeaponAbility kAbilities[] = {
    // Toan
    {ITEM_WEAPON_EVILCISE, kAbilityPoor, 100},
    {ITEM_WEAPON_MACHO_SWORD, kAbilityAbsUp, 100},
    // Xiao
    {ITEM_WEAPON_BONE_SLINGSHOT, kAbilityFragile, 50},
    {ITEM_WEAPON_HARDSHOOTER, kAbilityFragile, 50},
    {ITEM_WEAPON_MATADOR, kAbilityCritical, 100},
    // Goro
    {ITEM_WEAPON_FROZEN_TUNA, kAbilityStop, 100},
    // Ruby
    {ITEM_WEAPON_GODDESS_RING, kAbilityHeal, 100},
    {ITEM_WEAPON_ATHENA_S_ARMLET, kAbilityAbsUp, 100},
    // Ungaga
    {ITEM_WEAPON_DE_SANGA, kAbilityDrain, 100},
    {ITEM_WEAPON_BABEL_S_SPEAR, kAbilityConfuse, 100},
};

} // namespace

int WeaponAbilities(int weapon_no, int flags) {
    // Its own generator, so the rolls leave the game's rand() sequence alone.
    static std::minstd_rand generator(std::random_device{}());

    return WeaponAbilities(weapon_no, flags, static_cast<int>(generator() % 100));
}

int WeaponAbilities(int weapon_no, int flags, int roll) {
    for (const WeaponAbility &gain : kAbilities) {
        if (gain.weapon == weapon_no && roll < gain.chance) {
            flags |= gain.ability;
        }
    }

    return flags;
}

} // namespace expanded
