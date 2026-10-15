#include "expanded/weapons/buildups.hpp"

#include "itemdata.hpp"

namespace expanded {

namespace {

// The weapons a weapon builds up into; none makes it a final form.
struct WeaponBuildUp {
    int weapon;
    int into[3];
};

constexpr WeaponBuildUp kBuildUps[] = {
    // Toan
    {ITEM_WEAPON_HEAVEN_S_CLOUD, {}},
    {ITEM_WEAPON_CHOORA, {ITEM_WEAPON_MANEATER}},
    // Xiao
    {ITEM_WEAPON_HARDSHOOTER, {ITEM_WEAPON_DOUBLE_IMPACT}},
    {ITEM_WEAPON_DOUBLE_IMPACT, {ITEM_WEAPON_MATADOR}},
    // Goro
    {ITEM_WEAPON_BIG_BUCKS_HAMMER, {ITEM_WEAPON_MAGICAL_HAMMER}},
    {ITEM_WEAPON_FROZEN_TUNA, {}},
    // Ruby
    {ITEM_WEAPON_PLATINUM_RING, {ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_THORN_ARMLET}},
    {ITEM_WEAPON_THORN_ARMLET, {ITEM_WEAPON_DESTRUCTION_RING}},
    {ITEM_WEAPON_POCKLEKUL, {ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_GODDESS_RING, ITEM_WEAPON_SATAN_S_RING}},
    // Osmond
    {ITEM_WEAPON_JACKAL, {ITEM_WEAPON_SWALLOW}},
    {ITEM_WEAPON_SNAIL, {ITEM_WEAPON_HEXA_BLASTER}},
    {ITEM_WEAPON_SKUNK, {}},
};

// The changed build-ups of a weapon, or null for retail's.
const WeaponBuildUp *Changed(int weapon_no) {
    for (const WeaponBuildUp &buildup : kBuildUps) {
        if (buildup.weapon == weapon_no) {
            return &buildup;
        }
    }

    return nullptr;
}

} // namespace

bool BuildsUp(int weapon_no, const WEAPON_DATA *data) {
    const WeaponBuildUp *buildup = Changed(weapon_no);

    if (buildup == nullptr) {
        return data->buildup_mask0 != 0 || data->buildup_mask1 != 0;
    }

    return buildup->into[0] != 0;
}

bool BuildsUpInto(int weapon_no, const WEAPON_DATA *data, int position) {
    const WeaponBuildUp *buildup = Changed(weapon_no);

    if (buildup == nullptr) {
        return (position > 20 || (data->buildup_mask0 & (1 << position))) && (position <= 20 || (data->buildup_mask1 & (1 << (position - 20))));
    }

    // The chain counts from the owner's broken default weapon, position 0.
    for (int into : buildup->into) {
        if (into != 0 && into - (weapon_no - data->chain_pos) == position) {
            return true;
        }
    }

    return false;
}

} // namespace expanded
