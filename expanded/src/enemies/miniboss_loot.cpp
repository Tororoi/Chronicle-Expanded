#include "expanded/enemies/miniboss_loot.hpp"

#include <algorithm>
#include <random>
#include <vector>

#include "btitem.hpp"
#include "dun/gameloop.hpp"
#include "itemdata.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// A monster (MONSTOR::name_no) and the items it may drop, one picked at random.
struct Drops {
    int              name_no;
    std::vector<int> items;
};

// A dungeon's rare or common drops, and the boosts of the weapons among them.
struct Tier {
    std::vector<Drops>       drops;
    std::vector<WeaponBoost> boosts;
};

struct DungeonLoot {
    Tier             rare;
    Tier             common;
    int              first_half_floors; // floors that drop the first weapon list; 0 for one list
    std::vector<int> weapons[2];
};

constexpr int kRarePercent = 5;
constexpr int kCommonPercent = 30;

// Ability bits in the low and high bytes of the flags.
constexpr int kBigBucks = 0x02, kStop = 0x40, kSteal = 0x80;
constexpr int kFragile = 0x01, kDurable = 0x02, kDrain = 0x04, kCritical = 0x10, kAbsUp = 0x20;

const std::vector<int> kGems = {ITEM_ATTACH_GARNET, ITEM_ATTACH_AMETHYST, ITEM_ATTACH_AQUAMARINE, ITEM_ATTACH_DIAMOND,
                                ITEM_ATTACH_EMERALD, ITEM_ATTACH_PEARL, ITEM_ATTACH_RUBY, ITEM_ATTACH_PERIDOT,
                                ITEM_ATTACH_SAPPHIRE, ITEM_ATTACH_OPAL, ITEM_ATTACH_TOPAZ, ITEM_ATTACH_TURQUOISE};

const std::vector<int> kGoldOrGems = [] {
    std::vector<int> items = {ITEM_GOLD_BULLION};
    items.insert(items.end(), kGems.begin(), kGems.end());
    return items;
}();

const std::vector<int> kAttachments = {
    ITEM_ATTACH_FIRE,
    ITEM_ATTACH_ICE,
    ITEM_ATTACH_THUNDER,
    ITEM_ATTACH_WIND,
    ITEM_ATTACH_HOLY,
    ITEM_ATTACH_ATTACK,
    ITEM_ATTACH_ENDURANCE,
    ITEM_ATTACH_SPEED,
    ITEM_ATTACH_MAGICAL_POWER,
    ITEM_ATTACH_DINOSLAYER,
    ITEM_ATTACH_UNDEAD_BUSTER,
    ITEM_ATTACH_SEA_KILLER,
    ITEM_ATTACH_STONE_BREAKER,
    ITEM_ATTACH_PLANT_BUSTER,
    ITEM_ATTACH_BEAST_BUSTER,
    ITEM_ATTACH_SKY_HUNTER,
    ITEM_ATTACH_METALBREAKER,
    ITEM_ATTACH_MIMIC_BREAKER,
    ITEM_ATTACH_MAGE_SLAYER,
};

const std::vector<int> kLuckyItems = {ITEM_POWERUP_POWDER, ITEM_AUTO_REPAIR_POWDER};
const std::vector<int> kItems = {ITEM_STAMINA_DRINK, ITEM_DRAN_S_FEATHER};

constexpr int kBackFloorKeys[DUNGEON_COUNT] = {ITEM_TRAM_OIL, ITEM_SUN_DEW, ITEM_FLAPPING_FISH, ITEM_SECRET_PATH_KEY,
                                               ITEM_BRAVERY_LAUNCH, ITEM_FLAPPING_DUSTER, ITEM_CRYSTAL_EYEBALL};

// elem: fire, ice, thunder, wind, holy. anti: dragon, undead, sea, stone, plant, beast, sky, metal, mimic, mage.
const DungeonLoot kLoot[DUNGEON_COUNT] = {
    // Divine Beast Cave
    {
     .rare = {.drops = {{1, {ITEM_WEAPON_GLADIUS}}, {303, {ITEM_WEAPON_STEVE}}, {301, {ITEM_WEAPON_EVILCISE}}},
                 .boosts = {{.weapon = ITEM_WEAPON_GLADIUS, .attack = 15, .elem = {0, 0, 0, 0, 50}, .anti = {0, 50}}}},
     .common = {.drops = {{77, {ITEM_ATTACH_DIAMOND}},
                             {59, {ITEM_ATTACH_RUBY, ITEM_ATTACH_GARNET}},
                             {3, {ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_BONE_SLINGSHOT}},
                             {1, {ITEM_WEAPON_GLADIUS}},
                             {5, {ITEM_WEAPON_STEEL_SLINGSHOT}},
                             {303, {ITEM_ATTACH_TURQUOISE}},
                             {6, {ITEM_ATTACH_TOPAZ}},
                             {304, {ITEM_ATTACH_OPAL}},
                             {301, {ITEM_ATTACH_AMETHYST}},
                             {34, kGoldOrGems},
                             {35, kGems}}},
     .first_half_floors = 9,
     .weapons = {{ITEM_WEAPON_BASELARD, ITEM_WEAPON_GLADIUS, ITEM_WEAPON_CRYSKNIFE, ITEM_WEAPON_BONE_RAPIER},
                    {ITEM_WEAPON_BASELARD, ITEM_WEAPON_GLADIUS, ITEM_WEAPON_CRYSKNIFE, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_BONE_RAPIER,
                     ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_BONE_SLINGSHOT}},
     },
    // Wise Owl Forest
    {
     .rare = {.drops = {{12, {ITEM_WEAPON_TRIAL_HAMMER}},
                           {19, {ITEM_WEAPON_BANDIT_SLINGSHOT}},
                           {10, {ITEM_WEAPON_STEVE}},
                           {15, {ITEM_WEAPON_BANDIT_SLINGSHOT}},
                           {20, {ITEM_WEAPON_BANDIT_SLINGSHOT}},
                           {14, {ITEM_WEAPON_BANDIT_SLINGSHOT}},
                           {18, {ITEM_WEAPON_BANDIT_SLINGSHOT}},
                           {16, {ITEM_WEAPON_BANDIT_SLINGSHOT}},
                           {17, {ITEM_WEAPON_BANDIT_SLINGSHOT}},
                           {7, {ITEM_WEAPON_LAMB_S_SWORD}}},
                 .boosts = {{.weapon = ITEM_WEAPON_BANDIT_SLINGSHOT, .attack = 35, .elem = {0, 0, 0, 40}, .anti = {0, 0, 0, 0, 0, 0, 0, 0, 40}},
                            {.weapon = ITEM_WEAPON_LAMB_S_SWORD, .attack = 60, .magic = 30, .whp = 99}}},
     .common = {.drops = {{12, {ITEM_WEAPON_PLATE_HAMMER}},
                             {19, {ITEM_AUTO_REPAIR_POWDER}},
                             {305, {ITEM_ATTACH_SAPPHIRE, ITEM_WEAPON_FLAMINGO}},
                             {306, {ITEM_WEAPON_STEEL_HAMMER}},
                             {15, {ITEM_AUTO_REPAIR_POWDER}},
                             {20, {ITEM_AUTO_REPAIR_POWDER}},
                             {14, {ITEM_AUTO_REPAIR_POWDER}},
                             {18, {ITEM_AUTO_REPAIR_POWDER}},
                             {16, {ITEM_AUTO_REPAIR_POWDER}},
                             {17, {ITEM_AUTO_REPAIR_POWDER}},
                             {78, kGoldOrGems},
                             {79, kGems}}},
     .first_half_floors = 10,
     .weapons = {{ITEM_WEAPON_BUSTER_SWORD, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_SMALL_SWORD, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER,
                     ITEM_WEAPON_SAX, ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_STEVE, ITEM_WEAPON_BONE_SLINGSHOT},
                    {ITEM_WEAPON_BUSTER_SWORD, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_SMALL_SWORD, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER,
                     ITEM_WEAPON_SAX, ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_STEVE, ITEM_WEAPON_BONE_SLINGSHOT, ITEM_WEAPON_STEEL_HAMMER,
                     ITEM_WEAPON_FROZEN_TUNA, ITEM_WEAPON_TRIAL_HAMMER}},
     },
    // Shipwreck
    {
     .rare = {.drops = {{26, {ITEM_WEAPON_SERPENT_SWORD}},
                           {27, {ITEM_GOLD_BULLION}},
                           {28, {ITEM_WEAPON_DUSACK}},
                           {75, {ITEM_WEAPON_SMALL_SWORD}}},
                 .boosts = {{.weapon = ITEM_WEAPON_DUSACK, .abilities_low = kSteal},
                            {.weapon = ITEM_WEAPON_SMALL_SWORD, .attack = 40, .elem = {0, 0, 40}, .anti = {0, 0, 0, 0, 0, 0, 0, 0, 0, 40}}}},
     .common = {.drops = {{26, {ITEM_ATTACH_TURQUOISE, ITEM_ATTACH_GARNET}},
                             {27, {ITEM_WEAPON_FLAMINGO}},
                             {28, {ITEM_WEAPON_CHOPPER}},
                             {68, {ITEM_WEAPON_THORN_ARMLET}},
                             {23, {ITEM_ATTACH_PEARL}},
                             {24, {ITEM_WEAPON_FROZEN_TUNA, ITEM_ATTACH_AQUAMARINE}},
                             {75, {ITEM_ATTACH_AMETHYST}},
                             {25, {ITEM_WEAPON_STEEL_HAMMER, ITEM_POWERUP_POWDER}},
                             {85, {ITEM_WEAPON_CRYSTAL_RING}},
                             {80, kGoldOrGems},
                             {81, kGems}}},
     .first_half_floors = 10,
     .weapons = {{ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_KITCHEN_KNIFE, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER,
                     ITEM_WEAPON_BANDIT_SLINGSHOT, ITEM_WEAPON_HARDSHOOTER, ITEM_WEAPON_STEEL_HAMMER, ITEM_WEAPON_TURTLE_SHELL,
                     ITEM_WEAPON_PLATE_HAMMER, ITEM_WEAPON_TRIAL_HAMMER},
                    {ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_KITCHEN_KNIFE, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER,
                     ITEM_WEAPON_BANDIT_SLINGSHOT, ITEM_WEAPON_HARDSHOOTER, ITEM_WEAPON_TURTLE_SHELL, ITEM_WEAPON_PLATE_HAMMER,
                     ITEM_WEAPON_TRIAL_HAMMER, ITEM_WEAPON_BANDIT_S_RING, ITEM_WEAPON_PLATINUM_RING, ITEM_WEAPON_POCKLEKUL}},
     },
    // Sun and Moon Temple
    {
     .rare = {.drops = {{73, {ITEM_WEAPON_DRAGON_S_Y}},
                           {32, {ITEM_WEAPON_CACTUS}},
                           {30, {ITEM_WEAPON_SUN_SWORD, ITEM_WEAPON_TSUKIKAGE}},
                           {50, {ITEM_WEAPON_CLAYMORE}},
                           {64, {ITEM_WEAPON_PLATINUM_RING}}},
                 .boosts = {{.weapon = ITEM_WEAPON_PLATINUM_RING, .attack = 30, .elem = {0, 0, 0, 0, 40}, .anti = {0, 0, 0, 40}, .abilities_high = kDurable}}},
     .common = {.drops = {{73, {ITEM_ATTACH_AMETHYST, ITEM_ATTACH_AQUAMARINE, ITEM_ATTACH_TURQUOISE}},
                             {49, {ITEM_POWERUP_POWDER}},
                             {71, {ITEM_ATTACH_OPAL}},
                             {32, {ITEM_POWERUP_POWDER}},
                             {30, {ITEM_AUTO_REPAIR_POWDER}},
                             {31, {ITEM_WEAPON_BLESSING_GUN}},
                             {50, {ITEM_REVIVAL_POWDER, ITEM_ATTACH_PERIDOT}},
                             {64, {ITEM_ATTACH_OPAL, ITEM_ATTACH_DIAMOND}},
                             {36, kGoldOrGems},
                             {37, kGems}}},
     .first_half_floors = 10,
     .weapons = {{ITEM_WEAPON_ANTIQUE_SWORD, ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_DUSACK,
                     ITEM_WEAPON_DOUBLE_IMPACT, ITEM_WEAPON_BATTLE_AX, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_THORN_ARMLET},
                    {ITEM_WEAPON_ANTIQUE_SWORD, ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_DUSACK,
                     ITEM_WEAPON_DOUBLE_IMPACT, ITEM_WEAPON_BATTLE_AX, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_THORN_ARMLET,
                     ITEM_WEAPON_JAVELIN, ITEM_WEAPON_HALBERT, ITEM_WEAPON_5_FOOT_NAIL}},
     },
    // Moon Sea
    {
     .rare = {.drops = {{76, {ITEM_WEAPON_TSUKIKAGE, ITEM_WEAPON_MIRAGE}},
                           {62, {ITEM_WEAPON_SATAN_S_RING}},
                           {66, {ITEM_WEAPON_TRIAL_HAMMER}},
                           {72, {ITEM_WEAPON_FROZEN_TUNA}},
                           {33, {ITEM_WEAPON_GAIA_HAMMER}},
                           {70, {ITEM_WEAPON_BLESSING_GUN}},
                           {56, {ITEM_WEAPON_DE_SANGA}}},
                 .boosts = {{.weapon = ITEM_WEAPON_FROZEN_TUNA, .endurance = 99, .whp = 99, .elem = {0, 99}, .anti = {0, 0, 99}, .abilities_low = kBigBucks, .abilities_high = kFragile},
                            {.weapon = ITEM_WEAPON_BLESSING_GUN, .attack = 50, .magic = 30, .elem = {40}, .anti = {40, 0, 0, 0, 20}},
                            {.weapon = ITEM_WEAPON_DE_SANGA, .abilities_high = kDrain}}},
     .common = {.drops = {{40, {ITEM_WEAPON_SWALLOW, ITEM_WEAPON_5_FOOT_NAIL}},
                             {76, {ITEM_ATTACH_PEARL}},
                             {62, {ITEM_WEAPON_POCKLEKUL}},
                             {57, {ITEM_POWERUP_POWDER}},
                             {66, {ITEM_WEAPON_MAGICAL_HAMMER}},
                             {72, {ITEM_WEAPON_JAVELIN}},
                             {33, kGems},
                             {70, {ITEM_ATTACH_PERIDOT, ITEM_ATTACH_GARNET, ITEM_ATTACH_TOPAZ, ITEM_ATTACH_DIAMOND}},
                             {56, {ITEM_ATTACH_TOPAZ}},
                             {38, kGoldOrGems},
                             {39, kGems}},
                   .boosts = {{.weapon = ITEM_WEAPON_POCKLEKUL, .abilities_low = kSteal}}},
     .first_half_floors = 9,
     .weapons = {{ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_GAIA_HAMMER,
                     ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_DE_SANGA, ITEM_WEAPON_SCORPION, ITEM_WEAPON_PARTISAN},
                    {ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_GAIA_HAMMER,
                     ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_DE_SANGA, ITEM_WEAPON_SCORPION, ITEM_WEAPON_PARTISAN,
                     ITEM_WEAPON_JACKAL, ITEM_WEAPON_SNAIL}},
     },
    // Gallery of Time
    {
     .rare = {.drops = {{74, {ITEM_WEAPON_DRAGON_S_Y}},
                           {45, {ITEM_WEAPON_CACTUS}},
                           {67, {ITEM_WEAPON_THORN_ARMLET}},
                           {46, {ITEM_WEAPON_BIG_BANG}},
                           {44, {ITEM_WEAPON_ANGEL_SHOOTER}},
                           {48, {ITEM_WEAPON_SUPER_STEVE}},
                           {47, {ITEM_WEAPON_DARK_CLOUD}}},
                 .boosts = {{.weapon = ITEM_WEAPON_DRAGON_S_Y, .elem = {0, 0, 40}, .anti = {0, 0, 0, 0, 0, 40, 40}},
                            {.weapon = ITEM_WEAPON_CACTUS, .endurance = 99, .whp = 99, .elem = {0, 0, 0, 50}, .anti = {0, 0, 40, 0, 40}, .abilities_high = kCritical},
                            {.weapon = ITEM_WEAPON_THORN_ARMLET, .abilities_high = kAbsUp},
                            {.weapon = ITEM_WEAPON_BIG_BANG, .whp = 99, .abilities_high = kDurable},
                            {.weapon = ITEM_WEAPON_ANGEL_SHOOTER, .whp = 99, .elem = {40, 0, 40, 0, 40}},
                            {.weapon = ITEM_WEAPON_SUPER_STEVE, .attack = 80, .magic = 50, .whp = 99, .abilities_low = kSteal, .abilities_high = kAbsUp},
                            {.weapon = ITEM_WEAPON_DARK_CLOUD, .whp = 99, .elem = {40, 0, 0, 0, 40}, .anti = {0, 0, 0, 0, 0, 0, 0, 0, 0, 30}, .abilities_low = kStop, .abilities_high = kDrain}}},
     .common = {.drops = {{43, {ITEM_WEAPON_DRAIN_SEEKER}},
                             {69, {ITEM_WEAPON_TRIAL_HAMMER}},
                             {74, {ITEM_ATTACH_PERIDOT}},
                             {65, {ITEM_ATTACH_AQUAMARINE, ITEM_ATTACH_AMETHYST, ITEM_ATTACH_TURQUOISE}},
                             {45, {ITEM_WEAPON_STEEL_HAMMER}},
                             {52, {ITEM_WEAPON_MANEATER, ITEM_WEAPON_CROSS_HINDER}},
                             {67, {ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_7BRANCH_SWORD}},
                             {46, {ITEM_ATTACH_DIAMOND}},
                             {44, {ITEM_WEAPON_GODDESS_RING}},
                             {48, {ITEM_WEAPON_SUPER_STEVE}},
                             {47, {ITEM_WEAPON_BRAVE_ARK}},
                             {82, kGoldOrGems},
                             {83, kGems}},
                   .boosts = {{.weapon = ITEM_WEAPON_TRIAL_HAMMER, .elem = {0, 0, 50}}}},
     .weapons = {{ITEM_WEAPON_BRAVE_ARK, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAIN_SEEKER, ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_CROSS_HINDER,
                     ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_GAIA_HAMMER, ITEM_WEAPON_GODDESS_RING, ITEM_WEAPON_DESTRUCTION_RING,
                     ITEM_WEAPON_SATAN_S_RING, ITEM_WEAPON_MIRAGE, ITEM_WEAPON_CACTUS, ITEM_WEAPON_BLESSING_GUN, ITEM_WEAPON_SWALLOW}},
     },
    // Demon Shaft
    {
     .rare = {.drops = {{311, {ITEM_WEAPON_SATAN_S_RING}},
                           {308, {ITEM_WEAPON_MACHO_SWORD}},
                           {319, {ITEM_WEAPON_SATAN_S_AX}},
                           {312, {ITEM_WEAPON_CRYSTAL_RING}},
                           {313, {ITEM_WEAPON_TRIAL_HAMMER}},
                           {318, {ITEM_WEAPON_DESTRUCTION_RING}},
                           {314, {ITEM_WEAPON_HEAVEN_S_CLOUD}},
                           {315, {ITEM_WEAPON_GODDESS_RING}}},
                 .boosts = {{.weapon = ITEM_WEAPON_CRYSTAL_RING, .elem = {0, 50}},
                            {.weapon = ITEM_WEAPON_TRIAL_HAMMER, .elem = {0, 0, 50}},
                            {.weapon = ITEM_WEAPON_HEAVEN_S_CLOUD, .elem = {0, 0, 0, 50}}}},
     .common = {.drops = {{311, {ITEM_ATTACH_SUN, ITEM_ATTACH_RUBY}},
                             {319, {ITEM_WEAPON_MATADOR, ITEM_WEAPON_BONE_RAPIER}},
                             {312, {ITEM_ATTACH_SUN, ITEM_ATTACH_AQUAMARINE}},
                             {316, {ITEM_WEAPON_FLAMINGO}},
                             {313, {ITEM_ATTACH_SUN, ITEM_ATTACH_PEARL}},
                             {318, {ITEM_WEAPON_BONE_SLINGSHOT}},
                             {314, {ITEM_ATTACH_SUN, ITEM_ATTACH_SAPPHIRE}},
                             {315, {ITEM_ATTACH_SUN, ITEM_ATTACH_PERIDOT}},
                             {310, kGoldOrGems},
                             {309, kGems}},
                   .boosts = {{.weapon = ITEM_WEAPON_FLAMINGO, .abilities_high = kAbsUp}}},
     .weapons = {{ITEM_WEAPON_ATLAMILLIA_SWORD, ITEM_WEAPON_DARK_CLOUD, ITEM_WEAPON_BIG_BANG, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAIN_SEEKER,
                     ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_MATADOR, ITEM_WEAPON_LAST_JUDGEMENT, ITEM_WEAPON_SATAN_S_AX,
                     ITEM_WEAPON_ATHENA_S_ARMLET, ITEM_WEAPON_DESTRUCTION_RING, ITEM_WEAPON_TERRA_SWORD, ITEM_WEAPON_5_FOOT_NAIL,
                     ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_HEXA_BLASTER}},
     },
};

// Boosted weapons lying on the floor, dropped by its minibosses.
std::vector<const WeaponBoost *> lying;

MinibossLootRoll roll_override = nullptr;

int Roll(int n) {
    if (roll_override != nullptr) {
        return roll_override(n);
    }
    static std::minstd_rand rng{std::random_device{}()};
    return std::uniform_int_distribution<int>(0, n - 1)(rng);
}

int Pick(const std::vector<int> &items) {
    return items[Roll(static_cast<int>(items.size()))];
}

// The tier's drop for the monster, rolled at its percentage, with its boost.
bool RollTier(const Tier &tier, int percent, int name_no, MinibossDrop &drop) {
    auto drops = std::find_if(tier.drops.begin(), tier.drops.end(), [&](const Drops &d) { return d.name_no == name_no; });
    if (drops == tier.drops.end() || !(Roll(100) < percent)) {
        return false;
    }
    drop.item = Pick(drops->items);
    auto boost = std::find_if(tier.boosts.begin(), tier.boosts.end(), [&](const WeaponBoost &b) { return b.weapon == drop.item; });
    drop.boost = boost != tier.boosts.end() ? &*boost : nullptr;
    return true;
}

} // namespace

MinibossDrop RollMinibossDrop(int dungeon, int floor, int name_no) {
    MinibossDrop drop;
    if (dungeon < 0 || dungeon >= DUNGEON_COUNT) {
        return drop;
    }
    const DungeonLoot &loot = kLoot[dungeon];
    if (RollTier(loot.rare, kRarePercent, name_no, drop) || RollTier(loot.common, kCommonPercent, name_no, drop)) {
        return drop;
    }
    if (Roll(100) < 35) {
        drop.item = kBackFloorKeys[dungeon];
    } else if (Roll(100) < 15) {
        drop.item = Pick(loot.weapons[loot.first_half_floors != 0 && floor >= loot.first_half_floors]);
    } else if (Roll(100) < 80) {
        drop.item = Roll(100) < 60 ? Pick(kGems) : Pick(kAttachments);
    } else {
        drop.item = Roll(100) < 60 ? Pick(kLuckyItems) : Pick(kItems);
    }
    return drop;
}

void BoostWeapon(WEAPON_HAVE &weapon, const WeaponBoost &boost) {
    auto set = [](auto &field, int value) {
        if (value > 0) {
            field = value;
        }
    };
    set(weapon.attack, boost.attack);
    set(weapon.endurance, boost.endurance);
    set(weapon.speed, boost.speed);
    set(weapon.magic, boost.magic);
    set(weapon.durability, boost.whp);
    if (boost.whp > 0) {
        weapon.durability_f = boost.whp;
    }
    int best = 5;
    for (int i = 0; i < 5; i++) {
        set(weapon.elem[i], boost.elem[i]);
        if (weapon.elem[i] > (best == 5 ? 0 : weapon.elem[best])) {
            best = i;
        }
    }
    weapon.best_elem = best;
    for (int i = 0; i < 10; i++) {
        set(weapon.vs_monster[i], boost.anti[i]);
    }
    if (boost.abilities_low >= 0) {
        weapon.flags = (weapon.flags & 0xFF00) | boost.abilities_low;
    }
    if (boost.abilities_high >= 0) {
        weapon.flags = (weapon.flags & 0x00FF) | (boost.abilities_high << 8);
    }
}

void MinibossDropDropped(const MinibossDrop &drop) {
    if (drop.boost != nullptr) {
        lying.push_back(drop.boost);
    }
}

void MinibossDropsCleared() {
    lying.clear();
}

void MinibossDropPickUp(int dungeon, int item_no) {
    auto boost = std::find_if(lying.begin(), lying.end(), [&](const WeaponBoost *b) { return b->weapon == item_no; });
    if (boost == lying.end()) {
        BtGetAttach_Init(dungeon, item_no);
        return;
    }
    bool had[6][11];
    for (int chara = 0; chara < 6; chara++) {
        for (int slot = 0; slot < 11; slot++) {
            had[chara][slot] = UserStatus->chara_weapons[chara][slot].item_no == item_no;
        }
    }
    BtGetAttach_Init(dungeon, item_no);
    for (int chara = 0; chara < 6; chara++) {
        for (int slot = 0; slot < 11; slot++) {
            WEAPON_HAVE &weapon = UserStatus->chara_weapons[chara][slot];
            if (weapon.item_no == item_no && !had[chara][slot]) {
                BoostWeapon(weapon, **boost);
                lying.erase(boost);
                return;
            }
        }
    }
}

void SetMinibossLootRoll(MinibossLootRoll roll) {
    roll_override = roll;
}

} // namespace expanded
