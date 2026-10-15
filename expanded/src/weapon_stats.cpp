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

constexpr WeaponStats kStats[] = {
    // Toan
    {.weapon = ITEM_WEAPON_HEAVEN_S_CLOUD, .max_attack = 180, .max_magic = 180, .slots = 3},
    {.weapon = ITEM_WEAPON_LAMB_S_SWORD, .slots = 3},
    {.weapon = ITEM_WEAPON_BRAVE_ARK, .slots = 3},
    {.weapon = ITEM_WEAPON_SAND_BREAKER, .slots = 3},
    {.weapon = ITEM_WEAPON_CHOORA, .slots = 3},
    {.weapon = ITEM_WEAPON_7BRANCH_SWORD, .whp = 47, .attack = 67, .endurance = 47, .speed = 77, .magic = 37, .fire = 17, .ice = 17, .thunder = 17, .wind = 17, .holy = 17, .dino_slayer = 7, .undead_buster = 7, .sea_killer = 7, .stone_breaker = 7, .plant_buster = 7, .beast_buster = 7, .sky_hunter = 7, .metal_breaker = 7, .mimic_breaker = 7, .mage_slayer = 7},
    {.weapon = ITEM_WEAPON_CHRONICLE2, .max_attack = 999},
    // Goro
    {.weapon = ITEM_WEAPON_FROZEN_TUNA, .max_attack = 100, .max_magic = 678, .slots = 3},
    // Ruby
    {.weapon = ITEM_WEAPON_PLATINUM_RING, .ice = 0, .holy = 20},
    {.weapon = ITEM_WEAPON_CRYSTAL_RING, .ice = 20, .holy = 0},
    {.weapon = ITEM_WEAPON_POCKLEKUL, .attack = 28, .magic = 28},
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
