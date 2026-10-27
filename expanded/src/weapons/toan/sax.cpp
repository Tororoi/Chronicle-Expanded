#include "expanded/weapons/toan/sax.hpp"

#include <cstdint>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

enum Tier {
    kNone = -1,
    kSax = 0,
    kDusack = 1,
    kSevenBranch = 2,
};

struct Upgrade {
    int  from;
    int  to;
    Tier tier;
    bool roll; // one chest in kFineFareRepairOneIn
};

constexpr Upgrade kUpgrades[] = {
    {ITEM_REGULAR_WATER, ITEM_PREMIUM_WATER,      kSax,         false},
    {ITEM_TASTY_WATER,   ITEM_PREMIUM_WATER,      kSax,         false},
    {ITEM_BREAD,         ITEM_PREMIUM_CHICKEN,    kSax,         false},
    {ITEM_CHEESE,        ITEM_PREMIUM_CHICKEN,    kSax,         false},
    {ITEM_TREASURE_KEY,  ITEM_GOLD_BULLION,       kDusack,      false},
    {ITEM_REPAIR_POWDER, ITEM_AUTO_REPAIR_POWDER, kSevenBranch, true },
};

Tier TierOf(int weapon_no) {
    switch (weapon_no) {
        case ITEM_WEAPON_SAX:
            return kSax;
        case ITEM_WEAPON_DUSACK:
            return kDusack;
        case ITEM_WEAPON_7BRANCH_SWORD:
        case ITEM_WEAPON_ATLAMILLIA_SWORD:
        case ITEM_WEAPON_CHRONICLE_SWORD:
            return kSevenBranch;
        default:
            return kNone;
    }
}

// The floor's roll for a chest: the same each time it is opened.
bool RollWins(const CDungeonMap &map, int box_no) {
    std::uint32_t hash = static_cast<std::uint32_t>(map.map_seed) * 2654435761u ^ static_cast<std::uint32_t>(box_no + 1) * 40503u;
    hash ^= hash >> 15;
    return hash % kFineFareRepairOneIn == 0;
}

} // namespace

int FineFareTier() {
    if (NowWeaponHave == nullptr) {
        return kNone;
    }

    Tier tier = TierOf(HeldWeapon());
    return tier != kNone ? tier : TierOf(SuperSteveSphere());
}

int FineFareItem(const CDungeonMap &map, int box_no) {
    int item_no = map.boxes[box_no].item_no;
    int tier = FineFareTier();

    for (const Upgrade &upgrade : kUpgrades) {
        if (upgrade.from == item_no && upgrade.tier <= tier && (!upgrade.roll || RollWins(map, box_no))) {
            return upgrade.to;
        }
    }

    return item_no;
}

} // namespace expanded
