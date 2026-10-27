#include "dungeon.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "expanded/core/options.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/boss_record.hpp"
#include "expanded/enemies/randomize.hpp"
#include "expanded/enemies/themes.hpp"

namespace expanded {

namespace {

constexpr double kFarWeight = 0.10;
constexpr double kHomeBoost = 4.0;

// Demon Shaft's five bands, each led by its Gemron.
const std::vector<Monster> kDemonShaftFire = {Monster::GEMRON_FIRE, Monster::MUMMY_ENHANCED, Monster::SIL_ENHANCED,
                                              Monster::NIKAPOUS, Monster::HALLOWEEN_ENHANCED,
                                              Monster::MASTER_JACKET_ENHANCED, Monster::VULCAN_ENHANCED,
                                              Monster::DIAMOND_ENHANCED, Monster::WHITE_FANG_ENHANCED,
                                              Monster::ARTHUR_ENHANCED};
const std::vector<Monster> kDemonShaftIce = {Monster::GEMRON_ICE, Monster::CORCEA_ENHANCED, Monster::CLUB_ENHANCED,
                                             Monster::ROCKANOFF_ENHANCED, Monster::HORN_HEAD,
                                             Monster::AUNTIE_MEDU_ENHANCED, Monster::YAMMICH_ENHANCED,
                                             Monster::WITCH_HELLZA_ENHANCED, Monster::STEEL_GIANT_ENHANCED};
const std::vector<Monster> kDemonShaftThunder = {Monster::GEMRON_THUNDER, Monster::CAPTAIN_ENHANCED,
                                                 Monster::SPADE_ENHANCED, Monster::GOL_ENHANCED,
                                                 Monster::CAVE_BAT_ENHANCED, Monster::BISHOP_Q, Monster::GYON_ENHANCED,
                                                 Monster::RASH_DASHER_ENHANCED, Monster::MASK_OF_PRAJNA_ENHANCED};
const std::vector<Monster> kDemonShaftWind = {Monster::GEMRON_WIND, Monster::BOMBER_HEAD_ENHANCED,
                                              Monster::SILVER_GEAR, Monster::PIRATES_CHARIOT_ENHANCED,
                                              Monster::CRABBY_HERMIT_ENHANCED, Monster::SPACE_GYON_ENHANCED,
                                              Monster::CURSED_ROSE_ENHANCED, Monster::HEART_ENHANCED,
                                              Monster::ALEXANDER_ENHANCED};
const std::vector<Monster> kDemonShaftHoly = {Monster::GEMRON_HOLY, Monster::LIVING_ARMOR_ENHANCED,
                                              Monster::STATUE_DOG_ENHANCED, Monster::JOKER_ENHANCED,
                                              Monster::EVIL_BAT_ENHANCED, Monster::GACIOUS_ENHANCED,
                                              Monster::LICH_ENHANCED, Monster::TITAN_ENHANCED,
                                              Monster::CRESCENT_BARON_ENHANCED};

const std::vector<Theme> kThemes = {
    {.name = "Cards", .members = {Monster::HEART, Monster::CLUB, Monster::DIAMOND, Monster::SPADE, Monster::JOKER}, .whole_set = true},
    {.name = "Cards (Enhanced)", .members = {Monster::HEART_ENHANCED, Monster::CLUB_ENHANCED, Monster::DIAMOND_ENHANCED, Monster::SPADE_ENHANCED, Monster::JOKER_ENHANCED}, .whole_set = true},
    {.name = "Days of the Week", .members = {Monster::SUNDAY, Monster::MONDAY, Monster::TUESDAY, Monster::WEDNESDAY, Monster::THURSDAY, Monster::FRIDAY, Monster::SATURDAY}, .whole_set = true},
    {.name = "Dragons", .members = {Monster::DRAGON, Monster::BLUE_DRAGON, Monster::BLACK_DRAGON}, .weight = 2.0},
    {.name = "Outlaws", .members = {Monster::SAM, Monster::MR_BLARE, Monster::BILLY}},
    {.name = "Pirates", .members = {Monster::CAPTAIN, Monster::CORCEA, Monster::PIRATES_CHARIOT, Monster::SIL, Monster::GOL}, .whole_set = true, .weight = 0.5, .single = {Monster::CAPTAIN, Monster::SIL, Monster::GOL}, .with_mimics = true},
    {.name = "Gemron Elementals", .members = {Monster::GEMRON_FIRE, Monster::GEMRON_ICE, Monster::GEMRON_THUNDER, Monster::GEMRON_WIND, Monster::GEMRON_HOLY}, .whole_set = true},
    {.name = "Fire", .members = {Monster::GEMRON_FIRE, Monster::VULCAN_ENHANCED, Monster::MR_BLARE, Monster::DRAGON}, .whole_set = true},
    {.name = "Ice", .members = {Monster::GEMRON_ICE, Monster::BLIZZARD, Monster::SAM, Monster::BLUE_DRAGON}, .whole_set = true},
    {.name = "Thunder", .members = {Monster::GEMRON_THUNDER, Monster::GOL_ENHANCED, Monster::EARTH_DIGGER, Monster::MOON_BUG, Monster::BILLY}, .whole_set = true},
    {.name = "Wind", .members = {Monster::GEMRON_WIND, Monster::DUNE, Monster::GOLEM, Monster::SILVER_GEAR, Monster::PHANTOM}, .whole_set = true},
    {.name = "Holy", .members = {Monster::GEMRON_HOLY, Monster::GACIOUS_ENHANCED, Monster::MASK_OF_PRAJNA_ENHANCED, Monster::HORN_HEAD, Monster::YAMMICH_ENHANCED}, .whole_set = true},
    {.name = "Undead", .members = {Monster::SKELETON_SOLDIER, Monster::MASTER_JACKET, Monster::GACIOUS_ENHANCED, Monster::HORN_HEAD, Monster::SILVER_GEAR, Monster::LICH}, .weight = 2.0, .single = {Monster::LICH}},
    {.name = "Marine", .members = {Monster::CRABBY_HERMIT, Monster::GUNNY, Monster::GYON, Monster::OPAR, Monster::SPACE_GYON}, .weight = 2.0},
    {.name = "Rock", .members = {Monster::STATUE_DOG, Monster::DUNE, Monster::TITAN, Monster::ROCKANOFF}, .weight = 2.0},
    {.name = "Plants", .members = {Monster::CANNIBAL_PLANT, Monster::DARK_FLOWER, Monster::CURSED_ROSE, Monster::FLI_FLI}, .weight = 2.0},
    {.name = "Beasts", .members = {Monster::DASHER, Monster::WEREWOLF, Monster::WHITE_FANG_ENHANCED, Monster::RASH_DASHER}, .weight = 2.0},
    {.name = "Sky Dwellers", .members = {Monster::CAVE_BAT, Monster::EVIL_BAT, Monster::CRESCENT_BARON}, .weight = 2.0},
    {.name = "Metal", .members = {Monster::ARTHUR, Monster::ALEXANDER, Monster::STEEL_GIANT, Monster::PIRATES_CHARIOT, Monster::LIVING_ARMOR}, .weight = 2.0},
    {.name = "Mages", .members = {Monster::CURSE_DANCER, Monster::WITCH_HELLZA, Monster::BISHOP_Q, Monster::NIKAPOUS}, .weight = 2.0},
    {.name = "Not the Bees", .members = {Monster::HORNET, Monster::DARK_FLOWER}},
    {.name = "Bombs Away", .members = {Monster::BOMBER_HEAD, Monster::BOMBER_HEAD_ENHANCED, Monster::BOMB_GEMRON}, .weight = 2.0},
    {.name = "Whack-a-Mole", .members = {Monster::EARTH_DIGGER, Monster::HALEY_HOLEY}, .weight = 0.5},
    {.name = "Bait", .members = {Monster::KING_PRICKLY, Monster::MOON_DIGGER, Monster::WITCH_ILLZA, Monster::HALEY_HOLEY}, .weight = 3.0, .single = {Monster::KING_PRICKLY}},
    {.name = "Halloween", .members = {Monster::HALLOWEEN, Monster::GHOST, Monster::SKELETON_SOLDIER, Monster::MUMMY_ENHANCED, Monster::WEREWOLF, Monster::EVIL_BAT}, .weight = 2.0},
    {.name = "Gorgon", .members = {Monster::AUNTIE_MEDU, Monster::STATUE}},
    {.name = "Precious", .members = {Monster::SIL, Monster::GOL, Monster::HELL_POCKLE}, .weight = 0.5},
    {.name = "Graveyard Shift", .members = {Monster::MASTER_JACKET_ENHANCED, Monster::SKELETON_SOLDIER, Monster::HORN_HEAD, Monster::SILVER_GEAR}, .weight = 2.0},
    {.name = "Demon Shaft: Fire", .members = kDemonShaftFire, .home = DUNGEON_DEMON_SHAFT},
    {.name = "Demon Shaft: Ice", .members = kDemonShaftIce, .home = DUNGEON_DEMON_SHAFT},
    {.name = "Demon Shaft: Thunder", .members = kDemonShaftThunder, .home = DUNGEON_DEMON_SHAFT},
    {.name = "Demon Shaft: Wind", .members = kDemonShaftWind, .home = DUNGEON_DEMON_SHAFT},
    {.name = "Demon Shaft: Holy", .members = kDemonShaftHoly, .home = DUNGEON_DEMON_SHAFT},
};

// The bosses' themes: '!' members of the user's lists spawn once.
const std::vector<Theme> kBossThemes = {
    {.name = "Dran",
     .members = {Monster::DRAN, Monster::MASTER_JACKET, Monster::CAVE_BAT, Monster::SKELETON_SOLDIER, Monster::ROCKANOFF, Monster::OPAR, Monster::DASHER},
     .whole_set = true,
     .single = {Monster::DRAN, Monster::ROCKANOFF, Monster::OPAR, Monster::DASHER},
     .with_mimics = true,
     .boss = Monster::DRAN              },
    {.name = "Master Utan",
     .members = {Monster::MASTER_UTAN, Monster::SUNDAY, Monster::TUESDAY, Monster::FRIDAY, Monster::SATURDAY, Monster::HORNET, Monster::CANNIBAL_PLANT},
     .whole_set = true,
     .single = {Monster::MASTER_UTAN, Monster::SUNDAY, Monster::TUESDAY, Monster::FRIDAY, Monster::SATURDAY},
     .with_mimics = true,
     .boss = Monster::MASTER_UTAN       },
    {.name = "Ice Queen",
     .members = {Monster::ICE_QUEEN, Monster::ICE_BARRIER, Monster::SAM, Monster::GUNNY, Monster::CAPTAIN, Monster::CORCEA, Monster::PIRATES_CHARIOT},
     .whole_set = true,
     .single = {Monster::ICE_QUEEN, Monster::ICE_BARRIER, Monster::CAPTAIN, Monster::CORCEA, Monster::PIRATES_CHARIOT},
     .with_mimics = true,
     .boss = Monster::ICE_QUEEN         },
    {.name = "King's Curse",
     .members = {Monster::KINGS_CURSE, Monster::KINGS_CURSE_COFFIN, Monster::DUNE, Monster::MUMMY, Monster::GOLEM, Monster::GOL, Monster::SIL},
     .whole_set = true,
     .single = {Monster::KINGS_CURSE, Monster::KINGS_CURSE_COFFIN, Monster::GOL, Monster::SIL},
     .with_mimics = true,
     .boss = Monster::KINGS_CURSE_COFFIN},
    {.name = "Minotaur Joe",
     .members = {Monster::MINOTAUR_JOE, Monster::WINE_KEG, Monster::HELL_POCKLE, Monster::MOON_BUG, Monster::CRESCENT_BARON, Monster::WEDNESDAY, Monster::MOON_DIGGER},
     .whole_set = true,
     .single = {Monster::MINOTAUR_JOE, Monster::CRESCENT_BARON, Monster::WEDNESDAY, Monster::MOON_DIGGER},
     .with_mimics = true,
     .boss = Monster::MINOTAUR_JOE      },
    {.name = "Black Knight",
     .members = {Monster::BLACK_KNIGHT, Monster::BLACK_KNIGHT_MOUNT, Monster::GEMRON_HOLY, Monster::GEMRON_WIND, Monster::GEMRON_THUNDER, Monster::GEMRON_ICE, Monster::GEMRON_FIRE},
     .whole_set = true,
     .single = {Monster::BLACK_KNIGHT, Monster::BLACK_KNIGHT_MOUNT},
     .with_mimics = true,
     .boss = Monster::BLACK_KNIGHT      },
};

[[maybe_unused]] const bool kRegistered =
    AddToggle({.key = kRandomizerBosses,
               .label = "Randomizer Bosses",
               .help = "\"Randomizer Bosses\"\nBosses with their group\non randomized floors:\nnever, once beaten,\nor always.",
               .choices = "Off|Defeated|Any"});

std::vector<Monster> Join(std::initializer_list<const std::vector<Monster> *> groups) {
    std::vector<Monster> kinds;
    for (const std::vector<Monster> *group : groups) {
        kinds.insert(kinds.end(), group->begin(), group->end());
    }
    return kinds;
}

const std::vector<Monster> kNatives[DUNGEON_COUNT] = {
    {Monster::SKELETON_SOLDIER, Monster::MASTER_JACKET, Monster::STATUE, Monster::DASHER, Monster::CAVE_BAT,
     Monster::GHOST, Monster::DRAGON, Monster::ROCKANOFF, Monster::STATUE_DOG, Monster::YAMMICH, Monster::OPAR},
    {Monster::CANNIBAL_PLANT, Monster::EARTH_DIGGER, Monster::FLI_FLI, Monster::HORNET, Monster::HALLOWEEN,
     Monster::WEREWOLF, Monster::WITCH_ILLZA, Monster::KING_PRICKLY, Monster::HALEY_HOLEY, Monster::SUNDAY,
     Monster::MONDAY, Monster::TUESDAY, Monster::WEDNESDAY, Monster::THURSDAY, Monster::FRIDAY, Monster::SATURDAY},
    {Monster::CAPTAIN, Monster::CORCEA, Monster::GUNNY, Monster::GYON, Monster::PIRATES_CHARIOT, Monster::AUNTIE_MEDU,
     Monster::MASK_OF_PRAJNA, Monster::CURSED_ROSE, Monster::SAM},
    {Monster::GOLEM, Monster::MUMMY, Monster::PHANTOM, Monster::BOMBER_HEAD, Monster::CRABBY_HERMIT, Monster::DUNE,
     Monster::TITAN, Monster::STEEL_GIANT, Monster::BLUE_DRAGON, Monster::GOL, Monster::SIL, Monster::MR_BLARE},
    {Monster::ARTHUR, Monster::WITCH_HELLZA, Monster::MOON_BUG, Monster::MOON_DIGGER, Monster::SPACE_GYON,
     Monster::VULCAN, Monster::WHITE_FANG, Monster::TITAN, Monster::CRESCENT_BARON, Monster::HELL_POCKLE},
    {Monster::ALEXANDER, Monster::BILLY, Monster::BLACK_DRAGON, Monster::BLIZZARD, Monster::CLUB,
     Monster::CURSE_DANCER, Monster::DARK_FLOWER, Monster::DIAMOND, Monster::EVIL_BAT, Monster::HEART, Monster::JOKER,
     Monster::LICH, Monster::LIVING_ARMOR, Monster::RASH_DASHER, Monster::SPADE},
    Join({&kDemonShaftFire, &kDemonShaftIce, &kDemonShaftThunder, &kDemonShaftWind, &kDemonShaftHoly}
    ),
};

bool Chance(std::mt19937 &random, double chance) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(random) < chance;
}

// The repeatable monsters that may backfill a theme's one-of-each: the mimics alone, or the mimics then the
// dungeon's natives shuffled, by chance.
std::vector<Monster> Fillers(int dungeon, const DungeonMimics &mimics, std::mt19937 &random, ThemeOutcome::Fill &fill) {
    std::vector<Monster> fillers = {mimics.mimic, mimics.king};
    if (Chance(random, kThemeMimicFillChance)) {
        fill = ThemeOutcome::Fill::Mimics;
        return fillers;
    }
    fill = ThemeOutcome::Fill::MimicsAndNatives;
    std::vector<Monster> natives = DungeonNatives(dungeon);
    std::shuffle(natives.begin(), natives.end(), random);
    fillers.insert(fillers.end(), natives.begin(), natives.end());
    return fillers;
}

// Adds a kind, or finds it there already; false once the roster is full.
bool Place(Roster &roster, Monster kind, bool once) {
    return roster.Contains(kind) || roster.Add(kind, once);
}

// Whether a member of a theme spawns once even when the group fills the floor; none of the dungeon's Mimics does.
bool SpawnsOnceIn(const Theme *theme, Monster kind) {
    return theme != nullptr && std::find(theme->single.begin(), theme->single.end(), kind) != theme->single.end();
}

// A theme drawn by weight among the themes and the available bosses' after them, or null for the dungeon's Mimics,
// last at 1.
const Theme *DrawTheme(int dungeon, std::mt19937 &random) {
    std::vector<const Theme *> candidates;
    for (const Theme &candidate : kThemes) {
        candidates.push_back(&candidate);
    }
    for (const Theme &candidate : kBossThemes) {
        if (BossThemeAvailable(candidate)) {
            candidates.push_back(&candidate);
        }
    }
    double total = 1.0;
    for (const Theme *candidate : candidates) {
        total += ThemeWeight(*candidate, dungeon);
    }
    double roll = std::uniform_real_distribution<double>(0.0, total)(random);
    for (const Theme *candidate : candidates) {
        roll -= ThemeWeight(*candidate, dungeon);
        if (roll < 0.0) {
            return candidate;
        }
    }
    return nullptr;
}

// Each member, in order.
void PlaceAll(Roster &roster, const Theme *theme, const std::vector<Monster> &members) {
    for (Monster kind : members) {
        Place(roster, kind, SpawnsOnceIn(theme, kind));
    }
}

// A whole set (or the dungeon's Mimics) first; then the mimics a theme brings, or by chance fillers that fit, the set
// then spawning once each.
void PlaceWholeSet(Roster &roster, const Theme *theme, const std::vector<Monster> &members, int dungeon,
                   const DungeonMimics &mimics, std::mt19937 &random) {
    PlaceAll(roster, theme, members);
    if (theme == nullptr) {
        return;
    }
    if (theme->with_mimics) {
        Place(roster, mimics.mimic, false);
        Place(roster, mimics.king, false);
        return;
    }
    if (!Chance(random, kThemeCapOneChance)) {
        return;
    }
    bool               capped = false;
    ThemeOutcome::Fill fill;
    for (Monster kind : Fillers(dungeon, mimics, random, fill)) {
        if (!roster.Contains(kind) && roster.Add(kind, false)) {
            capped = true;
        }
    }
    if (capped) {
        for (Monster kind : members) {
            roster.SetOnce(kind, true);
        }
    }
}

// One of each member over a rolled number of repeatable fillers, the mimic placed first so the floor fills.
void PlaceOneOfEach(Roster &roster, const std::vector<Monster> &members, int dungeon, const DungeonMimics &mimics,
                    std::mt19937 &random, ThemeOutcome &result) {
    std::vector<Monster> fillers = Fillers(dungeon, mimics, random, result.fill);
    std::erase_if(fillers, [&](Monster kind) {
        return kind != mimics.mimic && std::find(members.begin(), members.end(), kind) != members.end();
    });
    int placed = std::min(static_cast<int>(members.size()), kRosterSize - 1);
    int room = std::min(kRosterSize - placed, static_cast<int>(fillers.size()));
    result.fillers = RollKindCount(room, random);
    roster.Add(fillers.front(), false);
    for (int i = 0; i < placed; i++) {
        Place(roster, members[i], true);
    }
    for (int i = 1; i < result.fillers; i++) {
        roster.Add(fillers[i], false);
    }
}

} // namespace

std::span<const Theme> Themes() {
    return kThemes;
}

RandomizerBosses RandomizerBossesSetting() {
    return static_cast<RandomizerBosses>(OptionChoice(kRandomizerBosses));
}

std::span<const Theme> BossThemes() {
    return kBossThemes;
}

bool BossThemeAvailable(const Theme &theme) {
    if (!theme.boss) {
        return true;
    }
    RandomizerBosses setting = RandomizerBossesSetting();
    if (setting == RandomizerBosses::Off || (setting == RandomizerBosses::Defeated && !BossDefeated(*theme.boss))) {
        return false;
    }
    for (Monster kind : theme.members) {
        if (!BossCastAllowed(kind)) {
            return false;
        }
    }
    return true;
}

const std::vector<Monster> &DungeonNatives(int dungeon) {
    static const std::vector<Monster> none;
    return dungeon >= 0 && dungeon < DUNGEON_COUNT ? kNatives[dungeon] : none;
}

double ThemeWeight(const Theme &theme, int dungeon) {
    if (theme.home < 0) {
        return theme.weight;
    }
    double near = 1.0 - std::abs(dungeon - theme.home) / static_cast<double>(DUNGEON_COUNT - 1);
    double pull = kFarWeight + (1.0 - kFarWeight) * near;
    return theme.weight * (dungeon == theme.home ? pull * kHomeBoost : pull);
}

Roster ThemedRoster(int dungeon, int floor, std::mt19937 &random, ThemeOutcome *outcome) {
    std::optional<DungeonMimics> dungeon_mimics = MimicsOf(dungeon, floor);
    if (!dungeon_mimics) {
        return {};
    }
    const DungeonMimics &mimics = *dungeon_mimics;
    const Theme         *theme = DrawTheme(dungeon, random);
    std::vector<Monster> members = theme != nullptr ? theme->members : std::vector<Monster>{mimics.mimic, mimics.king};
    std::shuffle(members.begin(), members.end(), random);
    ThemeOutcome result;
    result.name = theme != nullptr ? theme->name : "Mimics";

    Roster roster;
    if (theme == nullptr || theme->whole_set) {
        PlaceWholeSet(roster, theme, members, dungeon, mimics, random);
    } else if (!Chance(random, kThemeCapOneChance)) {
        // The group fills the floor.
        PlaceAll(roster, theme, members);
    } else {
        PlaceOneOfEach(roster, members, dungeon, mimics, random, result);
    }
    // A king mimic spawns once, but for the dungeon's Mimics.
    if (theme != nullptr) {
        roster.SetOnce(mimics.king, true);
    }
    if (outcome != nullptr) {
        *outcome = result;
    }
    return roster;
}

} // namespace expanded
