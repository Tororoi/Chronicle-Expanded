#include "dungeon.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/enemies/atla_gemron_loot.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// The rolls under which the drop is the Atlamillia Sword, its SynthSphere and the featured weapon's (atla_gemron_loot.hpp).
constexpr int kSwordPercent = 5, kAtlamilliaPercent = 20, kFeaturedPercent = 30;
constexpr int kFeatured[DUNGEON_COUNT] = {ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_DUSACK,
                                          ITEM_WEAPON_MANEATER, ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_DARK_CLOUD, 0};
// The SynthSphere tiers, by the mod's rebalanced attack, magic, elements and antis: a dungeon draws from its own and
// every tier below (Demon Shaft all). Not listed: the Atlamillia (its own drops).
// clang-format off
const std::vector<int> kTiers[DUNGEON_COUNT] = {
    {ITEM_WEAPON_DAGGER, ITEM_WEAPON_WOODEN_SLINGSHOT, ITEM_WEAPON_BASELARD, ITEM_WEAPON_BONE_SLINGSHOT, ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_MALLET, ITEM_WEAPON_GLADIUS, ITEM_WEAPON_MACHINE_GUN, ITEM_WEAPON_CRYSKNIFE, ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_FIGHTING_STICK, ITEM_WEAPON_STEEL_HAMMER, ITEM_WEAPON_GOLD_RING},
    {ITEM_WEAPON_BUSTER_SWORD, ITEM_WEAPON_BANDIT_SLINGSHOT, ITEM_WEAPON_POCKLEKUL, ITEM_WEAPON_BANDIT_S_RING, ITEM_WEAPON_BIG_BUCKS_HAMMER, ITEM_WEAPON_FROZEN_TUNA, ITEM_WEAPON_PLATE_HAMMER, ITEM_WEAPON_DOUBLE_IMPACT, ITEM_WEAPON_SMALL_SWORD, ITEM_WEAPON_STEVE, ITEM_WEAPON_JAVELIN, ITEM_WEAPON_FLAMINGO, ITEM_WEAPON_PLATINUM_RING, ITEM_WEAPON_JACKAL, ITEM_WEAPON_SNAIL, ITEM_WEAPON_SAX, ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_TURTLE_SHELL, ITEM_WEAPON_CHOPPER, ITEM_WEAPON_TRIAL_HAMMER, ITEM_WEAPON_HARDSHOOTER},
    {ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_HALBERT, ITEM_WEAPON_5_FOOT_NAIL, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_MACHO_SWORD, ITEM_WEAPON_KITCHEN_KNIFE, ITEM_WEAPON_PARTISAN, ITEM_WEAPON_DE_SANGA, ITEM_WEAPON_BLESSING_GUN, ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_BATTLE_AX, ITEM_WEAPON_ANTIQUE_SWORD, ITEM_WEAPON_DUSACK, ITEM_WEAPON_SCORPION, ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_SWALLOW, ITEM_WEAPON_SERPENT_SWORD, ITEM_WEAPON_MATADOR, ITEM_WEAPON_CHOORA, ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_GAIA_HAMMER, ITEM_WEAPON_CACTUS, ITEM_WEAPON_LAMB_S_SWORD},
    {ITEM_WEAPON_SKUNK, ITEM_WEAPON_MAGICAL_HAMMER, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_MIRAGE, ITEM_WEAPON_MANEATER},
    {ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_SUN_SWORD, ITEM_WEAPON_CROSS_HINDER, ITEM_WEAPON_GODDESS_RING, ITEM_WEAPON_SATAN_S_RING, ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_BRAVE_ARK, ITEM_WEAPON_AGA_S_SWORD},
    {ITEM_WEAPON_DARK_CLOUD, ITEM_WEAPON_DESTRUCTION_RING, ITEM_WEAPON_LAST_JUDGEMENT, ITEM_WEAPON_HEAVEN_S_CLOUD, ITEM_WEAPON_DRAIN_SEEKER, ITEM_WEAPON_TERRA_SWORD, ITEM_WEAPON_HEXA_BLASTER},
    {ITEM_WEAPON_MARDAN_EINS, ITEM_WEAPON_MARDAN_TWEI, ITEM_WEAPON_ARISE_MARDAN, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_ATHENA_S_ARMLET, ITEM_WEAPON_TALL_HAMMER, ITEM_WEAPON_SATAN_S_AX, ITEM_WEAPON_STAR_BREAKER, ITEM_WEAPON_HERCULES_WRATH, ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_MOBIUS_RING, ITEM_WEAPON_SUPERNOVA, ITEM_WEAPON_7THHEAVEN, ITEM_WEAPON_BIG_BANG, ITEM_WEAPON_BABEL_S_SPEAR, ITEM_WEAPON_ANGE_GEAR, ITEM_WEAPON_INFERNO, ITEM_WEAPON_SECRET_ARMLET, ITEM_WEAPON_SWORD_OF_ZEUS, ITEM_WEAPON_CHRONICLE2, ITEM_WEAPON_CHRONICLE_SWORD},
};
// clang-format on

int RandomWeapon(int dungeon) {
    std::vector<int> pool;
    int              tiers = std::clamp(dungeon + 1, 1, int(DUNGEON_COUNT));
    for (int t = 0; t < tiers; t++) {
        for (int weapon : kTiers[t]) {
            WEAPON_DATA *data = GetWeaponData(weapon);
            if (data != nullptr && (data->owner == 0 || data->owner < UserStatus->party_size)) { // its owner has joined
                pool.push_back(weapon);
            }
        }
    }
    return pool.empty() ? ITEM_WEAPON_DAGGER : pool[static_cast<std::size_t>(ModRoll(static_cast<int>(pool.size())))];
}

} // namespace

AtlaDrop RollAtlaGemronDrop(int dungeon) {
    dungeon = std::clamp(dungeon, 0, DUNGEON_COUNT - 1);
    int roll = ModRoll(100);
    if (roll < kSwordPercent && dungeon >= DUNGEON_SUN_MOON_TEMPLE) {
        return {ITEM_WEAPON_ATLAMILLIA_SWORD, false};
    }
    if (roll >= kSwordPercent && roll < kAtlamilliaPercent) {
        return {ITEM_WEAPON_ATLAMILLIA_SWORD, true};
    }
    if (roll >= kAtlamilliaPercent && roll < kFeaturedPercent && kFeatured[dungeon] != 0) {
        return {kFeatured[dungeon], true};
    }
    return {RandomWeapon(dungeon), true};
}

} // namespace expanded
