#include "expanded/weapon_abilities.hpp"

#include <random>

#include "itemdata.hpp"

namespace expanded {

namespace {

// The weapon ability bits.
constexpr int kPoor = 0x0004;
constexpr int kStop = 0x0040;
constexpr int kFragile = 0x0100;
constexpr int kDrain = 0x0400;
constexpr int kHeal = 0x0800;
constexpr int kCritical = 0x1000;
constexpr int kAbsUp = 0x2000;

// An ability a weapon gains, in chance percent of the weapons built.
struct WeaponAbility {
    int weapon;
    int ability;
    int chance;
};

constexpr WeaponAbility kAbilities[] = {
    // Toan
    {ITEM_WEAPON_EVILCISE, kPoor, 100},
    {ITEM_WEAPON_MACHO_SWORD, kAbsUp, 100},
    // Xiao
    {ITEM_WEAPON_BONE_SLINGSHOT, kFragile, 50},
    {ITEM_WEAPON_HARDSHOOTER, kFragile, 50},
    {ITEM_WEAPON_MATADOR, kCritical, 100},
    // Goro
    {ITEM_WEAPON_FROZEN_TUNA, kStop, 100},
    // Ruby
    {ITEM_WEAPON_GODDESS_RING, kHeal, 100},
    {ITEM_WEAPON_ATHENA_S_ARMLET, kAbsUp, 100},
    // Ungaga
    {ITEM_WEAPON_DE_SANGA, kDrain, 100},
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
