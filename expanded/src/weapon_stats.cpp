#include "expanded/weapon_stats.hpp"

#include "itemdata.hpp"

extern WEAPON_DATA WeaponList[120];

namespace expanded {

namespace {

// A stat left at kKeep keeps its retail value.
constexpr s16 kKeep = -1;

// One weapon's new stats; slots is how many attachment slots it has.
struct WeaponStats {
    int weapon;
    s16 whp = kKeep, attack = kKeep, endurance = kKeep, speed = kKeep, magic = kKeep;
    s16 fire = kKeep, ice = kKeep, thunder = kKeep, wind = kKeep, holy = kKeep;
    s16 dino_slayer = kKeep, undead_buster = kKeep, sea_killer = kKeep, stone_breaker = kKeep, plant_buster = kKeep;
    s16 beast_buster = kKeep, sky_hunter = kKeep, metal_breaker = kKeep, mimic_breaker = kKeep, mage_slayer = kKeep;
    s16 max_attack = kKeep, max_magic = kKeep;
    int slots = kKeep;
};

// A bonus added to every weapon from first to last, but the one excepted.
struct WeaponStatsBonus {
    int first, last, except;
    s16 attack, max_attack, endurance;
};

// The bonuses, added before the per-weapon stats are set.
constexpr WeaponStatsBonus kBonuses[] = {
    // Ungaga
    {.first = ITEM_WEAPON_FIGHTING_STICK, .last = ITEM_WEAPON_CACTUS, .except = ITEM_WEAPON_BABEL_S_SPEAR, .attack = 10, .max_attack = 10, .endurance = 15},
    // Osmond
    {.first = ITEM_WEAPON_MACHINE_GUN, .last = ITEM_WEAPON_SWALLOW, .except = 0, .attack = 15, .max_attack = 15, .endurance = 0},
};

constexpr WeaponStats kStats[] = {
    // Toan
    {.weapon = ITEM_WEAPON_BASELARD, .endurance = 30},
    {.weapon = ITEM_WEAPON_ANTIQUE_SWORD, .speed = 70, .fire = 15},
    {.weapon = ITEM_WEAPON_KITCHEN_KNIFE, .whp = 50, .attack = 25, .endurance = 30, .ice = 0, .thunder = 8, .sea_killer = 90},
    {.weapon = ITEM_WEAPON_TSUKIKAGE, .endurance = 33, .speed = 80},
    {.weapon = ITEM_WEAPON_HEAVEN_S_CLOUD, .max_attack = 180, .max_magic = 180, .slots = 3},
    {.weapon = ITEM_WEAPON_LAMB_S_SWORD, .slots = 3},
    {.weapon = ITEM_WEAPON_BRAVE_ARK, .slots = 3},
    {.weapon = ITEM_WEAPON_BIG_BANG, .speed = 70},
    {.weapon = ITEM_WEAPON_AGA_S_SWORD, .max_attack = 190},
    {.weapon = ITEM_WEAPON_SMALL_SWORD, .whp = 35, .magic = 17, .sea_killer = 0, .metal_breaker = 10},
    {.weapon = ITEM_WEAPON_SAND_BREAKER, .whp = 45, .endurance = 25, .slots = 3},
    {.weapon = ITEM_WEAPON_DRAIN_SEEKER, .whp = 60},
    {.weapon = ITEM_WEAPON_CHOPPER, .speed = 60},
    {.weapon = ITEM_WEAPON_CHOORA, .whp = 57, .attack = 45, .speed = 70, .ice = 10, .thunder = 15, .undead_buster = 15, .beast_buster = 15, .metal_breaker = 15, .slots = 3},
    {.weapon = ITEM_WEAPON_CLAYMORE, .undead_buster = 10, .beast_buster = 10, .mage_slayer = 10},
    {.weapon = ITEM_WEAPON_MANEATER, .endurance = 44, .speed = 70, .magic = 45, .ice = 15, .thunder = 15, .holy = 15, .undead_buster = 15, .beast_buster = 15, .metal_breaker = 15, .mimic_breaker = 10},
    {.weapon = ITEM_WEAPON_BONE_RAPIER, .whp = 38, .magic = 26},
    {.weapon = ITEM_WEAPON_SAX, .speed = 60, .fire = 6, .sky_hunter = 10},
    {.weapon = ITEM_WEAPON_7BRANCH_SWORD, .whp = 47, .endurance = 47, .magic = 37, .dino_slayer = 7, .undead_buster = 7, .sea_killer = 7, .stone_breaker = 7, .plant_buster = 7, .beast_buster = 8, .sky_hunter = 7, .metal_breaker = 10, .mimic_breaker = 7, .mage_slayer = 8},
    {.weapon = ITEM_WEAPON_CROSS_HINDER, .endurance = 50, .speed = 70, .magic = 32},
    {.weapon = ITEM_WEAPON_CHRONICLE2, .max_attack = 999},
    // Xiao
    {.weapon = ITEM_WEAPON_WOODEN_SLINGSHOT, .attack = 6, .magic = 2, .fire = 4},
    {.weapon = ITEM_WEAPON_BONE_SLINGSHOT, .attack = 11, .endurance = 30},
    {.weapon = ITEM_WEAPON_HARDSHOOTER, .speed = 60},
    // Goro
    {.weapon = ITEM_WEAPON_TURTLE_SHELL, .magic = 10},
    {.weapon = ITEM_WEAPON_FROZEN_TUNA, .whp = 65, .max_attack = 100, .max_magic = 678, .slots = 3},
    {.weapon = ITEM_WEAPON_GAIA_HAMMER, .endurance = 25},
    {.weapon = ITEM_WEAPON_TRIAL_HAMMER, .attack = 30, .endurance = 25},
    // Ruby
    {.weapon = ITEM_WEAPON_GOLD_RING, .attack = 15, .magic = 30},
    {.weapon = ITEM_WEAPON_PLATINUM_RING, .ice = 0, .holy = 20},
    {.weapon = ITEM_WEAPON_CRYSTAL_RING, .ice = 20, .holy = 0},
    {.weapon = ITEM_WEAPON_POCKLEKUL, .attack = 28, .magic = 28, .holy = 0},
    {.weapon = ITEM_WEAPON_THORN_ARMLET, .stone_breaker = 20, .beast_buster = 20, .max_attack = 70, .max_magic = 65},
    {.weapon = ITEM_WEAPON_DESTRUCTION_RING, .attack = 60, .metal_breaker = 40, .max_magic = 140},
    // Ungaga
    {.weapon = ITEM_WEAPON_BABEL_S_SPEAR, .slots = 4},
    // Osmond
    {.weapon = ITEM_WEAPON_BLESSING_GUN, .max_attack = 87, .max_magic = 80},
    {.weapon = ITEM_WEAPON_SKUNK, .max_attack = 143, .max_magic = 105},
};

void Set(s16 &field, s16 value) {
    if (value != kKeep) {
        field = value;
    }
}

} // namespace

void WeaponStatsApply() {
    static bool applied = false;

    if (!applied) {
        WeaponStatsApply(WeaponList);
        applied = true;
    }
}

void WeaponStatsApply(WEAPON_DATA *list) {
    for (const WeaponStatsBonus &bonus : kBonuses) {
        for (int weapon = bonus.first; weapon <= bonus.last; weapon++) {
            if (weapon != bonus.except) {
                WEAPON_DATA &data = list[weapon - ITEM_WEAPON_START];
                data.attack += bonus.attack;
                data.attack_max += bonus.max_attack;
                data.endurance += bonus.endurance;
            }
        }
    }

    for (const WeaponStats &stats : kStats) {
        WEAPON_DATA &data = list[stats.weapon - ITEM_WEAPON_START];
        const s16    elem[5] = {stats.fire, stats.ice, stats.thunder, stats.wind, stats.holy};
        const s16    vs_monster[10] = {stats.dino_slayer, stats.undead_buster, stats.sea_killer, stats.stone_breaker, stats.plant_buster,
                                       stats.beast_buster,  stats.sky_hunter,    stats.metal_breaker, stats.mimic_breaker, stats.mage_slayer};

        Set(data.durability, stats.whp);
        Set(data.attack, stats.attack);
        Set(data.endurance, stats.endurance);
        Set(data.speed, stats.speed);
        Set(data.magic, stats.magic);
        Set(data.attack_max, stats.max_attack);
        Set(data.magic_max, stats.max_magic);

        for (int i = 0; i < 5; i++) {
            Set(data.elem[i], elem[i]);
        }

        for (int i = 0; i < 10; i++) {
            Set(data.vs_monster[i], vs_monster[i]);
        }

        // A slot the weapon lacks opens as a regular one.
        for (int i = 0; i < stats.slots; i++) {
            if (data.hole[i] <= 0) {
                data.hole[i] = 1;
            }
        }
    }
}

} // namespace expanded
