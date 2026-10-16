#include "expanded/enemies/miniboss_loot.hpp"

#include <algorithm>
#include <random>
#include <vector>

#include "btitem.hpp"
#include "dun/gameloop.hpp"
#include "itemdata.hpp"
#include "randomitem.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

// An item a species drops in dungeons from..until-1, boosted or not.
struct Drop {
    int  item;
    int  from = DUNGEON_DIVINE_BEAST_CAVE;
    int  until = DUNGEON_COUNT;
    bool boosted = false;
};

constexpr Drop Boosted(int item, int from = DUNGEON_DIVINE_BEAST_CAVE, int until = DUNGEON_COUNT) {
    return {item, from, until, true};
}

// The rare and common drops of species that share them.
struct SpeciesLoot {
    std::vector<Monster> species;
    std::vector<Drop>    rare;
    std::vector<Drop>    common;
};

// A dungeon's weapons the fallback drops.
struct DungeonWeapons {
    int              first_half_floors; // floors that drop the first list; 0 for one list
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

std::vector<Drop> Drops(const std::vector<int> &items) {
    std::vector<Drop> drops;
    for (int item : items) {
        drops.push_back({item});
    }
    return drops;
}

const std::vector<Drop> kGemDrops = Drops(kGems);
const std::vector<Drop> kGoldOrGemDrops = Drops(kGoldOrGems);

// elem: fire, ice, thunder, wind, holy. anti: dragon, undead, sea, stone, plant, beast, sky, metal, mimic, mage.
const WeaponBoost kBoosts[] = {
    {.weapon = ITEM_WEAPON_GLADIUS, .attack = 15, .elem = {0, 0, 0, 0, 50}, .anti = {0, 50}},
    {.weapon = ITEM_WEAPON_BANDIT_SLINGSHOT, .attack = 35, .elem = {0, 0, 0, 40}, .anti = {0, 0, 0, 0, 0, 0, 0, 0, 40}},
    {.weapon = ITEM_WEAPON_LAMB_S_SWORD, .attack = 60, .magic = 30, .whp = 99},
    {.weapon = ITEM_WEAPON_DUSACK, .abilities_low = kSteal},
    {.weapon = ITEM_WEAPON_SMALL_SWORD, .attack = 40, .elem = {0, 0, 40}, .anti = {0, 0, 0, 0, 0, 0, 0, 0, 0, 40}},
    {.weapon = ITEM_WEAPON_PLATINUM_RING, .attack = 30, .elem = {0, 0, 0, 0, 40}, .anti = {0, 0, 0, 40}, .abilities_high = kDurable},
    {.weapon = ITEM_WEAPON_FROZEN_TUNA, .endurance = 99, .whp = 99, .elem = {0, 99}, .anti = {0, 0, 99}, .abilities_low = kBigBucks, .abilities_high = kFragile},
    {.weapon = ITEM_WEAPON_BLESSING_GUN, .attack = 50, .magic = 30, .elem = {40}, .anti = {40, 0, 0, 0, 20}},
    {.weapon = ITEM_WEAPON_POCKLEKUL, .abilities_low = kSteal},
    {.weapon = ITEM_WEAPON_DRAGON_S_Y, .elem = {0, 0, 40}, .anti = {0, 0, 0, 0, 0, 40, 40}},
    {.weapon = ITEM_WEAPON_CACTUS, .endurance = 99, .whp = 99, .elem = {0, 0, 0, 50}, .anti = {0, 0, 40, 0, 40}, .abilities_high = kCritical},
    {.weapon = ITEM_WEAPON_THORN_ARMLET, .abilities_high = kCritical},
    {.weapon = ITEM_WEAPON_BIG_BANG, .whp = 99, .abilities_high = kDurable},
    {.weapon = ITEM_WEAPON_ANGEL_SHOOTER, .whp = 99, .elem = {40, 0, 40, 0, 40}},
    {.weapon = ITEM_WEAPON_SUPER_STEVE, .attack = 80, .magic = 50, .whp = 99, .abilities_low = kSteal, .abilities_high = kAbsUp},
    {.weapon = ITEM_WEAPON_DARK_CLOUD, .whp = 99, .elem = {40, 0, 0, 0, 40}, .anti = {0, 0, 0, 0, 0, 0, 0, 0, 0, 30}, .abilities_low = kStop, .abilities_high = kDrain},
    {.weapon = ITEM_WEAPON_TRIAL_HAMMER, .elem = {0, 0, 50}},
    {.weapon = ITEM_WEAPON_CRYSTAL_RING, .elem = {0, 50}},
    {.weapon = ITEM_WEAPON_HEAVEN_S_CLOUD, .elem = {0, 0, 0, 50}},
    {.weapon = ITEM_WEAPON_FLAMINGO, .abilities_high = kAbsUp},
    {.weapon = ITEM_WEAPON_STEEL_SLINGSHOT, .abilities_high = kDurable},
    {.weapon = ITEM_WEAPON_5_FOOT_NAIL, .abilities_high = kDurable},
    {.weapon = ITEM_WEAPON_PLATE_HAMMER, .abilities_high = kDurable},
};

// The species' drops, by home dungeon, then the mimics'.
const SpeciesLoot kLoot[] = {
    // Divine Beast Cave
    {{Monster::CAVE_BAT, Monster::EVIL_BAT}, {{ITEM_WEAPON_DRAIN_SEEKER, DUNGEON_GALLERY_OF_TIME}}, {{ITEM_POISONOUS_APPLE}}},
    {{Monster::DASHER}, {{ITEM_WEAPON_BATTLE_AX, DUNGEON_WISE_OWL_FOREST}}, {{ITEM_ATTACH_TOPAZ}}},
    {{Monster::DRAGON}, {{ITEM_WEAPON_DRAGON_S_Y, DUNGEON_SUN_MOON_TEMPLE, DUNGEON_GALLERY_OF_TIME}, Boosted(ITEM_WEAPON_DRAGON_S_Y, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_ATTACH_RUBY}, {ITEM_ATTACH_GARNET}}},
    {{Monster::GHOST, Monster::LICH}, {{ITEM_WEAPON_CROSS_HINDER, DUNGEON_SUN_MOON_TEMPLE}}, {}},
    {{Monster::MASTER_JACKET}, {Boosted(ITEM_WEAPON_GLADIUS)}, {{ITEM_WEAPON_GLADIUS}}},
    {{Monster::OPAR}, {}, {{ITEM_ATTACH_OPAL}}},
    {{Monster::ROCKANOFF}, {}, {{ITEM_ATTACH_DIAMOND}}},
    {{Monster::SKELETON_SOLDIER}, {}, {{ITEM_WEAPON_BONE_RAPIER}, {ITEM_WEAPON_BONE_SLINGSHOT}}},
    {{Monster::STATUE, Monster::LIVING_ARMOR}, {Boosted(ITEM_WEAPON_PLATE_HAMMER, DUNGEON_SHIPWRECK)}, {{ITEM_WEAPON_STEEL_SLINGSHOT}}},
    {{Monster::STATUE_DOG}, {{ITEM_WEAPON_STEVE}}, {}},
    {{Monster::YAMMICH}, {{ITEM_WEAPON_EVILCISE}}, {}},
    // Wise Owl Forest
    {{Monster::CANNIBAL_PLANT, Monster::CURSED_ROSE}, {{ITEM_WEAPON_THORN_ARMLET, DUNGEON_SHIPWRECK, DUNGEON_GALLERY_OF_TIME}, Boosted(ITEM_WEAPON_THORN_ARMLET, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_SUN_DEW, DUNGEON_WISE_OWL_FOREST}}},
    {{Monster::EARTH_DIGGER, Monster::MOON_DIGGER}, {{ITEM_WEAPON_TRIAL_HAMMER, DUNGEON_WISE_OWL_FOREST}}, {{ITEM_WEAPON_PLATE_HAMMER, DUNGEON_WISE_OWL_FOREST, DUNGEON_MOON_SEA}, {ITEM_WEAPON_MAGICAL_HAMMER, DUNGEON_MOON_SEA}}},
    {{Monster::FLI_FLI}, {{ITEM_WEAPON_SNAIL, DUNGEON_MOON_SEA}}, {}},
    {{Monster::SUNDAY, Monster::MONDAY, Monster::TUESDAY, Monster::WEDNESDAY, Monster::THURSDAY, Monster::FRIDAY, Monster::SATURDAY}, {Boosted(ITEM_WEAPON_BANDIT_SLINGSHOT, DUNGEON_WISE_OWL_FOREST, DUNGEON_MOON_SEA), Boosted(ITEM_WEAPON_POCKLEKUL, DUNGEON_MOON_SEA)}, {{ITEM_AUTO_REPAIR_POWDER}}},
    {{Monster::HALEY_HOLEY}, {{ITEM_WEAPON_FLAMINGO, DUNGEON_WISE_OWL_FOREST}}, {{ITEM_ATTACH_SAPPHIRE}}},
    {{Monster::HALLOWEEN}, {{ITEM_WEAPON_STEVE}}, {}},
    {{Monster::HORNET, Monster::PHANTOM}, {{ITEM_WEAPON_SCORPION, DUNGEON_SUN_MOON_TEMPLE}}, {}},
    {{Monster::KING_PRICKLY}, {{ITEM_WEAPON_CACTUS, DUNGEON_SUN_MOON_TEMPLE}}, {{ITEM_WEAPON_STEEL_HAMMER, DUNGEON_WISE_OWL_FOREST}}},
    {{Monster::WEREWOLF}, {Boosted(ITEM_WEAPON_LAMB_S_SWORD, DUNGEON_WISE_OWL_FOREST)}, {}},
    {{Monster::WITCH_ILLZA, Monster::WITCH_HELLZA}, {{ITEM_FRUIT_OF_EDEN, DUNGEON_SHIPWRECK}}, {}},
    // Shipwreck
    {{Monster::AUNTIE_MEDU}, {{ITEM_WEAPON_SERPENT_SWORD, DUNGEON_SHIPWRECK}}, {{ITEM_ATTACH_TURQUOISE}, {ITEM_ATTACH_GARNET}, {ITEM_ATTACH_EMERALD}}},
    {{Monster::CAPTAIN}, {{ITEM_WEAPON_FLAMINGO, DUNGEON_SHIPWRECK}}, {{ITEM_GOLD_BULLION}, {ITEM_BRAVERY_LAUNCH, DUNGEON_MOON_SEA}}},
    {{Monster::CORCEA}, {Boosted(ITEM_WEAPON_DUSACK, DUNGEON_SHIPWRECK)}, {{ITEM_WEAPON_CHOPPER, DUNGEON_SHIPWRECK}, {ITEM_BRAVERY_LAUNCH, DUNGEON_MOON_SEA}}},
    {{Monster::GUNNY}, {}, {{ITEM_ATTACH_PEARL}}},
    {{Monster::GYON}, {{ITEM_WEAPON_FROZEN_TUNA, DUNGEON_SHIPWRECK, DUNGEON_MOON_SEA}, Boosted(ITEM_WEAPON_FROZEN_TUNA, DUNGEON_MOON_SEA)}, {{ITEM_ATTACH_AQUAMARINE}, {ITEM_WEAPON_JAVELIN, DUNGEON_SUN_MOON_TEMPLE}}},
    {{Monster::MASK_OF_PRAJNA}, {Boosted(ITEM_WEAPON_SMALL_SWORD, DUNGEON_SHIPWRECK)}, {{ITEM_ATTACH_AMETHYST}}},
    {{Monster::PIRATES_CHARIOT}, {}, {{ITEM_WEAPON_STEEL_HAMMER, DUNGEON_SHIPWRECK}, {ITEM_POWERUP_POWDER}}},
    {{Monster::SAM}, {}, {{ITEM_WEAPON_CRYSTAL_RING, DUNGEON_SHIPWRECK}}},
    // Sun and Moon Temple
    {{Monster::BLUE_DRAGON}, {{ITEM_WEAPON_DRAGON_S_Y, DUNGEON_SUN_MOON_TEMPLE, DUNGEON_GALLERY_OF_TIME}, Boosted(ITEM_WEAPON_DRAGON_S_Y, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_ATTACH_AMETHYST}, {ITEM_ATTACH_AQUAMARINE}, {ITEM_ATTACH_TURQUOISE}}},
    {{Monster::BOMBER_HEAD}, {}, {{ITEM_POWERUP_POWDER}}},
    {{Monster::CRABBY_HERMIT}, {{ITEM_WEAPON_SNAIL, DUNGEON_MOON_SEA}}, {}},
    {{Monster::DUNE}, {{ITEM_WEAPON_CACTUS, DUNGEON_SUN_MOON_TEMPLE}}, {{ITEM_POWERUP_POWDER}}},
    {{Monster::GOL}, {}, {{ITEM_GOLD_BULLION}}},
    {{Monster::GOLEM}, {{ITEM_WEAPON_SUN_SWORD, DUNGEON_SUN_MOON_TEMPLE}, {ITEM_WEAPON_TSUKIKAGE, DUNGEON_SUN_MOON_TEMPLE}}, {{ITEM_AUTO_REPAIR_POWDER}}},
    {{Monster::MR_BLARE}, {}, {{ITEM_WEAPON_BLESSING_GUN, DUNGEON_SUN_MOON_TEMPLE}}},
    {{Monster::MUMMY}, {{ITEM_WEAPON_CLAYMORE, DUNGEON_SUN_MOON_TEMPLE}}, {{ITEM_REVIVAL_POWDER}, {ITEM_ATTACH_PERIDOT}}},
    {{Monster::SIL}, {Boosted(ITEM_WEAPON_PLATINUM_RING, DUNGEON_SHIPWRECK)}, {}},
    {{Monster::STEEL_GIANT}, {Boosted(ITEM_WEAPON_STEEL_SLINGSHOT, DUNGEON_WISE_OWL_FOREST), Boosted(ITEM_WEAPON_5_FOOT_NAIL, DUNGEON_SUN_MOON_TEMPLE)}, {{ITEM_ATTACH_OPAL}, {ITEM_ATTACH_DIAMOND}}},
    {{Monster::TITAN}, {{ITEM_WEAPON_GAIA_HAMMER, DUNGEON_MOON_SEA}}, kGemDrops},
    // Moon Sea
    {{Monster::ARTHUR}, {Boosted(ITEM_WEAPON_5_FOOT_NAIL, DUNGEON_SUN_MOON_TEMPLE)}, {{ITEM_WEAPON_STEEL_SLINGSHOT, DUNGEON_WISE_OWL_FOREST}, {ITEM_WEAPON_SWALLOW, DUNGEON_MOON_SEA}}},
    {{Monster::CRESCENT_BARON}, {{ITEM_WEAPON_TSUKIKAGE, DUNGEON_WISE_OWL_FOREST}, {ITEM_WEAPON_MIRAGE, DUNGEON_SUN_MOON_TEMPLE}}, {{ITEM_ATTACH_PEARL}}},
    {{Monster::HELL_POCKLE}, {{ITEM_WEAPON_SATAN_S_RING, DUNGEON_MOON_SEA}, Boosted(ITEM_WEAPON_BANDIT_SLINGSHOT, DUNGEON_WISE_OWL_FOREST, DUNGEON_MOON_SEA), Boosted(ITEM_WEAPON_POCKLEKUL, DUNGEON_MOON_SEA)}, {}},
    {{Monster::MOON_BUG}, {}, {{ITEM_POWERUP_POWDER}}},
    {{Monster::SPACE_GYON}, {{ITEM_WEAPON_FROZEN_TUNA, DUNGEON_SHIPWRECK, DUNGEON_MOON_SEA}, Boosted(ITEM_WEAPON_FROZEN_TUNA, DUNGEON_MOON_SEA)}, {{ITEM_WEAPON_JAVELIN, DUNGEON_SUN_MOON_TEMPLE}}},
    {{Monster::VULCAN}, {Boosted(ITEM_WEAPON_BLESSING_GUN, DUNGEON_MOON_SEA)}, {{ITEM_ATTACH_PERIDOT}, {ITEM_ATTACH_GARNET}, {ITEM_ATTACH_TOPAZ}, {ITEM_ATTACH_DIAMOND}, {ITEM_ATTACH_RUBY}}},
    {{Monster::WHITE_FANG}, {{ITEM_WEAPON_DE_SANGA, DUNGEON_MOON_SEA}}, {{ITEM_ATTACH_TOPAZ}}},
    // Gallery of Time
    {{Monster::ALEXANDER}, {}, {{ITEM_WEAPON_DRAIN_SEEKER, DUNGEON_GALLERY_OF_TIME}, {ITEM_ATTACH_GARNET}}},
    {{Monster::BILLY}, {}, {Boosted(ITEM_WEAPON_TRIAL_HAMMER, DUNGEON_GALLERY_OF_TIME)}},
    {{Monster::BLACK_DRAGON}, {{ITEM_WEAPON_DRAGON_S_Y, DUNGEON_SUN_MOON_TEMPLE, DUNGEON_GALLERY_OF_TIME}, Boosted(ITEM_WEAPON_DRAGON_S_Y, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_ATTACH_PERIDOT}}},
    {{Monster::BLIZZARD}, {{ITEM_WEAPON_CRYSTAL_RING, DUNGEON_SHIPWRECK}}, {{ITEM_ATTACH_AQUAMARINE}, {ITEM_ATTACH_TURQUOISE}}},
    {{Monster::CLUB}, {Boosted(ITEM_WEAPON_CACTUS, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_WEAPON_STEEL_HAMMER, DUNGEON_WISE_OWL_FOREST}}},
    {{Monster::CURSE_DANCER}, {{ITEM_WEAPON_MANEATER, DUNGEON_SHIPWRECK}}, {{ITEM_WEAPON_EVILCISE, DUNGEON_WISE_OWL_FOREST}}},
    {{Monster::DARK_FLOWER}, {{ITEM_WEAPON_THORN_ARMLET, DUNGEON_SHIPWRECK, DUNGEON_GALLERY_OF_TIME}, Boosted(ITEM_WEAPON_THORN_ARMLET, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_WEAPON_7BRANCH_SWORD, DUNGEON_GALLERY_OF_TIME}, {ITEM_SUN_DEW, DUNGEON_WISE_OWL_FOREST}}},
    {{Monster::DIAMOND}, {Boosted(ITEM_WEAPON_BIG_BANG, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_ATTACH_DIAMOND}}},
    {{Monster::HEART}, {Boosted(ITEM_WEAPON_ANGEL_SHOOTER, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_WEAPON_GODDESS_RING, DUNGEON_GALLERY_OF_TIME}}},
    {{Monster::JOKER}, {Boosted(ITEM_WEAPON_SUPER_STEVE, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_WEAPON_STEVE}}},
    {{Monster::RASH_DASHER}, {{ITEM_WEAPON_BATTLE_AX, DUNGEON_WISE_OWL_FOREST}}, {}},
    {{Monster::SPADE}, {Boosted(ITEM_WEAPON_DARK_CLOUD, DUNGEON_GALLERY_OF_TIME)}, {{ITEM_WEAPON_BRAVE_ARK, DUNGEON_GALLERY_OF_TIME}}},
    // Demon Shaft
    {{Monster::BISHOP_Q}, {{ITEM_WEAPON_FLAMINGO, DUNGEON_WISE_OWL_FOREST, DUNGEON_GALLERY_OF_TIME}, Boosted(ITEM_WEAPON_FLAMINGO, DUNGEON_GALLERY_OF_TIME)}, {}},
    {{Monster::GACIOUS_ENHANCED}, {{ITEM_WEAPON_SATAN_S_AX, DUNGEON_GALLERY_OF_TIME}}, {{ITEM_WEAPON_BONE_RAPIER}}},
    {{Monster::GEMRON_FIRE}, {{ITEM_WEAPON_SATAN_S_RING, DUNGEON_DEMON_SHAFT}}, {{ITEM_ATTACH_RUBY}, {ITEM_ATTACH_SUN, DUNGEON_MOON_SEA}}},
    {{Monster::GEMRON_HOLY}, {{ITEM_WEAPON_GODDESS_RING, DUNGEON_DEMON_SHAFT}}, {{ITEM_ATTACH_PERIDOT}, {ITEM_ATTACH_SUN, DUNGEON_MOON_SEA}}},
    {{Monster::GEMRON_ICE}, {Boosted(ITEM_WEAPON_CRYSTAL_RING, DUNGEON_DEMON_SHAFT)}, {{ITEM_ATTACH_AQUAMARINE}, {ITEM_ATTACH_SAPPHIRE}, {ITEM_ATTACH_SUN, DUNGEON_MOON_SEA}}},
    {{Monster::GEMRON_THUNDER}, {Boosted(ITEM_WEAPON_TRIAL_HAMMER, DUNGEON_DEMON_SHAFT)}, {{ITEM_ATTACH_PEARL}, {ITEM_ATTACH_SUN, DUNGEON_MOON_SEA}}},
    {{Monster::GEMRON_WIND}, {Boosted(ITEM_WEAPON_HEAVEN_S_CLOUD, DUNGEON_DEMON_SHAFT)}, {{ITEM_ATTACH_EMERALD}, {ITEM_ATTACH_SUN, DUNGEON_MOON_SEA}}},
    {{Monster::HORN_HEAD}, {{ITEM_WEAPON_SATAN_S_AX, DUNGEON_GALLERY_OF_TIME}}, {{ITEM_WEAPON_MATADOR, DUNGEON_MOON_SEA}, {ITEM_WEAPON_STEEL_HAMMER, DUNGEON_WISE_OWL_FOREST}}},
    {{Monster::NIKAPOUS}, {{ITEM_WEAPON_MACHO_SWORD, DUNGEON_DEMON_SHAFT}}, {{ITEM_ATTACH_GARNET}}},
    {{Monster::SILVER_GEAR}, {{ITEM_WEAPON_DESTRUCTION_RING, DUNGEON_GALLERY_OF_TIME}}, {{ITEM_WEAPON_BONE_SLINGSHOT}}},    // Every dungeon
    {{Monster::KING_MIMIC_DBC, Monster::KING_MIMIC_WOF, Monster::KING_MIMIC_SW, Monster::KING_MIMIC_SMT, Monster::KING_MIMIC_MS, Monster::KING_MIMIC_GOT, Monster::KING_MIMIC_DS, Monster::KING_MIMIC_DS_ENHANCED, Monster::KING_MIMIC_DS_ENHANCED_TWICE, Monster::KING_MIMIC_DS_ENHANCED_THRICE}, {}, kGoldOrGemDrops},
    {{Monster::MIMIC_DBC, Monster::MIMIC_WOF, Monster::MIMIC_SW, Monster::MIMIC_SMT, Monster::MIMIC_MS, Monster::MIMIC_GOT, Monster::MIMIC_DS, Monster::MIMIC_DS_ENHANCED, Monster::MIMIC_DS_ENHANCED_TWICE, Monster::MIMIC_DS_ENHANCED_THRICE}, {}, kGemDrops},
};

// Enhanced species that take their base's loot.
constexpr Monster kTakesLootOf[][2] = {
    {Monster::ALEXANDER_ENHANCED, Monster::ALEXANDER},
    {Monster::ARTHUR_ENHANCED, Monster::ARTHUR},
    {Monster::AUNTIE_MEDU_ENHANCED, Monster::AUNTIE_MEDU},
    {Monster::BOMBER_HEAD_ENHANCED, Monster::BOMBER_HEAD},
    {Monster::CAPTAIN_ENHANCED, Monster::CAPTAIN},
    {Monster::CAVE_BAT_ENHANCED, Monster::CAVE_BAT},
    {Monster::CLUB_ENHANCED, Monster::CLUB},
    {Monster::CORCEA_ENHANCED, Monster::CORCEA},
    {Monster::CRABBY_HERMIT_ENHANCED, Monster::CRABBY_HERMIT},
    {Monster::CRESCENT_BARON_ENHANCED, Monster::CRESCENT_BARON},
    {Monster::CURSED_ROSE_ENHANCED, Monster::CURSED_ROSE},
    {Monster::DIAMOND_ENHANCED, Monster::DIAMOND},
    {Monster::EVIL_BAT_ENHANCED, Monster::EVIL_BAT},
    {Monster::GOL_ENHANCED, Monster::GOL},
    {Monster::GYON_ENHANCED, Monster::GYON},
    {Monster::HALLOWEEN_ENHANCED, Monster::HALLOWEEN},
    {Monster::HEART_ENHANCED, Monster::HEART},
    {Monster::JOKER_ENHANCED, Monster::JOKER},
    {Monster::LICH_ENHANCED, Monster::LICH},
    {Monster::LIVING_ARMOR_ENHANCED, Monster::LIVING_ARMOR},
    {Monster::MASK_OF_PRAJNA_ENHANCED, Monster::MASK_OF_PRAJNA},
    {Monster::MASTER_JACKET_ENHANCED, Monster::MASTER_JACKET},
    {Monster::MUMMY_ENHANCED, Monster::MUMMY},
    {Monster::PIRATES_CHARIOT_ENHANCED, Monster::PIRATES_CHARIOT},
    {Monster::RASH_DASHER_ENHANCED, Monster::RASH_DASHER},
    {Monster::ROCKANOFF_ENHANCED, Monster::ROCKANOFF},
    {Monster::SIL_ENHANCED, Monster::SIL},
    {Monster::SPACE_GYON_ENHANCED, Monster::SPACE_GYON},
    {Monster::SPADE_ENHANCED, Monster::SPADE},
    {Monster::STATUE_DOG_ENHANCED, Monster::STATUE_DOG},
    {Monster::STEEL_GIANT_ENHANCED, Monster::STEEL_GIANT},
    {Monster::TITAN_ENHANCED, Monster::TITAN},
    {Monster::VULCAN_ENHANCED, Monster::VULCAN},
    {Monster::WHITE_FANG_ENHANCED, Monster::WHITE_FANG},
    {Monster::WITCH_HELLZA_ENHANCED, Monster::WITCH_HELLZA},
    {Monster::YAMMICH_ENHANCED, Monster::YAMMICH},
};

const DungeonWeapons kWeapons[DUNGEON_COUNT] = {
    // Divine Beast Cave
    {9, {{ITEM_WEAPON_BASELARD, ITEM_WEAPON_GLADIUS, ITEM_WEAPON_CRYSKNIFE, ITEM_WEAPON_BONE_RAPIER},
          {ITEM_WEAPON_BASELARD, ITEM_WEAPON_GLADIUS, ITEM_WEAPON_CRYSKNIFE, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_BONE_RAPIER, ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_BONE_SLINGSHOT}}},
    // Wise Owl Forest
    {10, {{ITEM_WEAPON_BUSTER_SWORD, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_SMALL_SWORD, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER, ITEM_WEAPON_SAX, ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_STEVE, ITEM_WEAPON_BONE_SLINGSHOT},
          {ITEM_WEAPON_BUSTER_SWORD, ITEM_WEAPON_SHAMSHIR, ITEM_WEAPON_SMALL_SWORD, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER, ITEM_WEAPON_SAX, ITEM_WEAPON_STEEL_SLINGSHOT, ITEM_WEAPON_STEVE, ITEM_WEAPON_BONE_SLINGSHOT, ITEM_WEAPON_STEEL_HAMMER, ITEM_WEAPON_FROZEN_TUNA, ITEM_WEAPON_TRIAL_HAMMER}}},
    // Shipwreck
    {10, {{ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_KITCHEN_KNIFE, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER, ITEM_WEAPON_BANDIT_SLINGSHOT, ITEM_WEAPON_HARDSHOOTER, ITEM_WEAPON_STEEL_HAMMER, ITEM_WEAPON_TURTLE_SHELL, ITEM_WEAPON_PLATE_HAMMER, ITEM_WEAPON_TRIAL_HAMMER},
          {ITEM_WEAPON_WISE_OWL_SWORD, ITEM_WEAPON_KITCHEN_KNIFE, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_SAND_BREAKER, ITEM_WEAPON_CHOPPER, ITEM_WEAPON_BANDIT_SLINGSHOT, ITEM_WEAPON_HARDSHOOTER, ITEM_WEAPON_TURTLE_SHELL, ITEM_WEAPON_PLATE_HAMMER, ITEM_WEAPON_TRIAL_HAMMER, ITEM_WEAPON_BANDIT_S_RING, ITEM_WEAPON_PLATINUM_RING, ITEM_WEAPON_POCKLEKUL}}},
    // Sun and Moon Temple
    {10, {{ITEM_WEAPON_ANTIQUE_SWORD, ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_DUSACK, ITEM_WEAPON_DOUBLE_IMPACT, ITEM_WEAPON_BATTLE_AX, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_THORN_ARMLET},
          {ITEM_WEAPON_ANTIQUE_SWORD, ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_EVILCISE, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_DUSACK, ITEM_WEAPON_DOUBLE_IMPACT, ITEM_WEAPON_BATTLE_AX, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_FAIRY_S_RING, ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_JAVELIN, ITEM_WEAPON_HALBERT, ITEM_WEAPON_5_FOOT_NAIL}}},
    // Moon Sea
    {9, {{ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_GAIA_HAMMER, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_DE_SANGA, ITEM_WEAPON_SCORPION, ITEM_WEAPON_PARTISAN},
          {ITEM_WEAPON_LAMB_S_SWORD, ITEM_WEAPON_CLAYMORE, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAGON_S_Y, ITEM_WEAPON_GAIA_HAMMER, ITEM_WEAPON_CRYSTAL_RING, ITEM_WEAPON_THORN_ARMLET, ITEM_WEAPON_DE_SANGA, ITEM_WEAPON_SCORPION, ITEM_WEAPON_PARTISAN, ITEM_WEAPON_JACKAL, ITEM_WEAPON_SNAIL}}},
    // Gallery of Time
    {0, {{ITEM_WEAPON_BRAVE_ARK, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAIN_SEEKER, ITEM_WEAPON_7BRANCH_SWORD, ITEM_WEAPON_CROSS_HINDER, ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_GAIA_HAMMER, ITEM_WEAPON_GODDESS_RING, ITEM_WEAPON_DESTRUCTION_RING, ITEM_WEAPON_SATAN_S_RING, ITEM_WEAPON_MIRAGE, ITEM_WEAPON_CACTUS, ITEM_WEAPON_BLESSING_GUN, ITEM_WEAPON_SWALLOW}}},
    // Demon Shaft
    {0, {{ITEM_WEAPON_ATLAMILLIA_SWORD, ITEM_WEAPON_DARK_CLOUD, ITEM_WEAPON_BIG_BANG, ITEM_WEAPON_AGA_S_SWORD, ITEM_WEAPON_DRAIN_SEEKER, ITEM_WEAPON_ANGEL_SHOOTER, ITEM_WEAPON_MATADOR, ITEM_WEAPON_LAST_JUDGEMENT, ITEM_WEAPON_SATAN_S_AX, ITEM_WEAPON_ATHENA_S_ARMLET, ITEM_WEAPON_DESTRUCTION_RING, ITEM_WEAPON_TERRA_SWORD, ITEM_WEAPON_5_FOOT_NAIL, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_HEXA_BLASTER}}},
};

// A boosted drop lying on the floor: its slot among the items, and the item and spot that tell it from a later one there.
struct Lying {
    const CRandomItem *items;
    int                slot;
    int                item;
    float              position[3];
    const WeaponBoost *boost;
};

std::vector<Lying> lying;

// The boost of the item being picked up, if it is a boosted drop.
const WeaponBoost *picked = nullptr;

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

const SpeciesLoot *LootOf(Monster species) {
    for (const auto &alias : kTakesLootOf) {
        if (alias[0] == species) {
            species = alias[1];
        }
    }
    for (const SpeciesLoot &loot : kLoot) {
        if (std::find(loot.species.begin(), loot.species.end(), species) != loot.species.end()) {
            return &loot;
        }
    }
    return nullptr;
}

const WeaponBoost *BoostOf(int weapon) {
    for (const WeaponBoost &boost : kBoosts) {
        if (boost.weapon == weapon) {
            return &boost;
        }
    }
    return nullptr;
}

} // namespace

std::vector<MinibossDrop> MinibossLoot(Monster species, int dungeon, MinibossTier tier) {
    std::vector<MinibossDrop> drops;
    const SpeciesLoot        *loot = LootOf(species);
    if (loot == nullptr) {
        return drops;
    }
    for (const Drop &drop : tier == MinibossTier::RARE ? loot->rare : loot->common) {
        if (dungeon >= drop.from && dungeon < drop.until) {
            drops.push_back({drop.item, drop.boosted ? BoostOf(drop.item) : nullptr});
        }
    }
    return drops;
}

MinibossDrop RollMinibossDrop(int dungeon, int floor, Monster species) {
    MinibossDrop drop;
    if (dungeon < 0 || dungeon >= DUNGEON_COUNT) {
        return drop;
    }
    std::vector<MinibossDrop> tier = MinibossLoot(species, dungeon, MinibossTier::RARE);
    if (tier.empty() || !(Roll(100) < kRarePercent)) {
        tier.clear();
    }
    if (tier.empty() && Roll(100) < kCommonPercent) {
        tier = MinibossLoot(species, dungeon, MinibossTier::COMMON);
    }
    if (!tier.empty()) {
        return tier[Roll(static_cast<int>(tier.size()))];
    }
    const DungeonWeapons &weapons = kWeapons[dungeon];
    if (Roll(100) < 35) {
        drop.item = kBackFloorKeys[dungeon];
    } else if (Roll(100) < 15) {
        drop.item = Pick(weapons.weapons[weapons.first_half_floors != 0 && floor >= weapons.first_half_floors]);
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

void MinibossDropLanding(const MinibossDrop &drop, CRandomItem &items, const float *position) {
    int slot = items.CheckID();
    if (drop.boost == nullptr || slot == -1) {
        return;
    }
    std::erase_if(lying, [&](const Lying &l) { return l.items == &items && l.slot == slot; });
    lying.push_back({&items, slot, drop.item, {position[0], position[1] + 1.75f, position[2]}, drop.boost});
}

int MinibossItemEvent(CRandomItem &items) {
    picked = nullptr;
    for (int slot = 0; slot < 32; slot++) {
        if (items.pickup_event[slot] == -1) {
            continue;
        }
        auto found = std::find_if(lying.begin(), lying.end(), [&](const Lying &l) { return l.items == &items && l.slot == slot; });
        if (found != lying.end()) {
            bool same = found->item == items.item_no[slot] && found->position[0] == items.position[slot][0] &&
                        found->position[1] == items.position[slot][1] && found->position[2] == items.position[slot][2];
            picked = same ? found->boost : nullptr;
            lying.erase(found);
        }
        break;
    }
    return items.checkEvent();
}

void MinibossDropsCleared() {
    lying.clear();
    picked = nullptr;
}

void MinibossDropPickUp(int dungeon, int item_no) {
    const WeaponBoost *boost = picked != nullptr && picked->weapon == item_no ? picked : nullptr;
    picked = nullptr;
    if (boost == nullptr) {
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
                BoostWeapon(weapon, *boost);
                return;
            }
        }
    }
}

void SetMinibossLootRoll(MinibossLootRoll roll) {
    roll_override = roll;
}

} // namespace expanded
