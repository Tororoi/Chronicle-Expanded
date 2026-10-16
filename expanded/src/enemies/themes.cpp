#include "expanded/enemies/themes.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

#include "expanded/enemies/randomize.hpp"

namespace expanded {

namespace {

constexpr int    kDungeons = 7;
constexpr int    kDemonShaft = 6;
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
    {"Cards", {Monster::HEART, Monster::CLUB, Monster::DIAMOND, Monster::SPADE, Monster::JOKER}, true},
    {"Cards (Enhanced)", {Monster::HEART_ENHANCED, Monster::CLUB_ENHANCED, Monster::DIAMOND_ENHANCED, Monster::SPADE_ENHANCED, Monster::JOKER_ENHANCED}, true},
    {"Days of the Week", {Monster::SUNDAY, Monster::MONDAY, Monster::TUESDAY, Monster::WEDNESDAY, Monster::THURSDAY, Monster::FRIDAY, Monster::SATURDAY}, true},
    {"Dragons", {Monster::DRAGON, Monster::BLUE_DRAGON, Monster::BLACK_DRAGON}, false, -1, 2.0},
    {"Outlaws", {Monster::SAM, Monster::MR_BLARE, Monster::BILLY}},
    {"Pirates", {Monster::CAPTAIN, Monster::CORCEA, Monster::PIRATES_CHARIOT, Monster::SIL, Monster::GOL}, true, -1, 0.5, {Monster::CAPTAIN, Monster::SIL, Monster::GOL}, true},
    {"Gemron Elementals", {Monster::GEMRON_FIRE, Monster::GEMRON_ICE, Monster::GEMRON_THUNDER, Monster::GEMRON_WIND, Monster::GEMRON_HOLY}, true},
    {"Fire", {Monster::GEMRON_FIRE, Monster::VULCAN_ENHANCED, Monster::MR_BLARE, Monster::DRAGON}, true},
    {"Ice", {Monster::GEMRON_ICE, Monster::BLIZZARD, Monster::SAM, Monster::BLUE_DRAGON}, true},
    {"Thunder", {Monster::GEMRON_THUNDER, Monster::GOL_ENHANCED, Monster::EARTH_DIGGER, Monster::MOON_BUG, Monster::BILLY}, true},
    {"Wind", {Monster::GEMRON_WIND, Monster::DUNE, Monster::GOLEM, Monster::SILVER_GEAR, Monster::PHANTOM}, true},
    {"Holy", {Monster::GEMRON_HOLY, Monster::GACIOUS_ENHANCED, Monster::MASK_OF_PRAJNA_ENHANCED, Monster::HORN_HEAD, Monster::YAMMICH_ENHANCED}, true},
    {"Undead", {Monster::SKELETON_SOLDIER, Monster::MASTER_JACKET, Monster::GACIOUS_ENHANCED, Monster::HORN_HEAD, Monster::SILVER_GEAR, Monster::LICH}, false, -1, 2.0, {Monster::LICH}},
    {"Marine", {Monster::CRABBY_HERMIT, Monster::GUNNY, Monster::GYON, Monster::OPAR, Monster::SPACE_GYON}, false, -1, 2.0},
    {"Rock", {Monster::STATUE_DOG, Monster::DUNE, Monster::TITAN, Monster::ROCKANOFF}, false, -1, 2.0},
    {"Plants", {Monster::CANNIBAL_PLANT, Monster::DARK_FLOWER, Monster::CURSED_ROSE, Monster::FLI_FLI}, false, -1, 2.0},
    {"Beasts", {Monster::DASHER, Monster::WEREWOLF, Monster::WHITE_FANG_ENHANCED, Monster::RASH_DASHER}, false, -1, 2.0},
    {"Sky Dwellers", {Monster::CAVE_BAT, Monster::EVIL_BAT, Monster::CRESCENT_BARON}, false, -1, 2.0},
    {"Metal", {Monster::ARTHUR, Monster::ALEXANDER, Monster::STEEL_GIANT, Monster::PIRATES_CHARIOT, Monster::LIVING_ARMOR}, false, -1, 2.0},
    {"Mages", {Monster::CURSE_DANCER, Monster::WITCH_HELLZA, Monster::BISHOP_Q, Monster::NIKAPOUS}, false, -1, 2.0},
    {"Not the Bees", {Monster::HORNET, Monster::DARK_FLOWER}},
    {"Bombs Away", {Monster::BOMBER_HEAD, Monster::BOMBER_HEAD_ENHANCED}, false, -1, 2.0},
    {"Whack-a-Mole", {Monster::EARTH_DIGGER, Monster::HALEY_HOLEY}, false, -1, 0.5},
    {"Bait", {Monster::KING_PRICKLY, Monster::MOON_DIGGER, Monster::WITCH_ILLZA, Monster::HALEY_HOLEY}, false, -1, 3.0, {Monster::KING_PRICKLY}},
    {"Halloween", {Monster::HALLOWEEN, Monster::GHOST, Monster::SKELETON_SOLDIER, Monster::MUMMY_ENHANCED, Monster::WEREWOLF, Monster::EVIL_BAT}, false, -1, 2.0},
    {"Gorgon", {Monster::AUNTIE_MEDU, Monster::STATUE}},
    {"Precious", {Monster::SIL, Monster::GOL, Monster::HELL_POCKLE}, false, -1, 0.5},
    {"Graveyard Shift", {Monster::MASTER_JACKET_ENHANCED, Monster::SKELETON_SOLDIER, Monster::HORN_HEAD, Monster::SILVER_GEAR}, false, -1, 2.0},
    {"Demon Shaft: Fire", kDemonShaftFire, false, kDemonShaft},
    {"Demon Shaft: Ice", kDemonShaftIce, false, kDemonShaft},
    {"Demon Shaft: Thunder", kDemonShaftThunder, false, kDemonShaft},
    {"Demon Shaft: Wind", kDemonShaftWind, false, kDemonShaft},
    {"Demon Shaft: Holy", kDemonShaftHoly, false, kDemonShaft},
};

std::vector<Monster> Join(std::initializer_list<const std::vector<Monster> *> groups) {
    std::vector<Monster> kinds;
    for (const std::vector<Monster> *group : groups) {
        kinds.insert(kinds.end(), group->begin(), group->end());
    }
    return kinds;
}

const std::vector<Monster> kNatives[kDungeons] = {
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

// The repeatable monsters that backfill a theme's one-of-each: the mimics or the natives, by chance.
std::vector<Monster> Fillers(int dungeon, const DungeonMimics &mimics, std::mt19937 &random) {
    if (Chance(random, kThemeMimicFillChance)) {
        return {mimics.mimic, mimics.king};
    }
    std::vector<Monster> natives = DungeonNatives(dungeon);
    std::shuffle(natives.begin(), natives.end(), random);
    return natives;
}

// Adds a kind, or finds it there already; false once the roster is full.
bool Place(Roster &roster, Monster kind, bool once) {
    return roster.Contains(kind) || roster.Add(kind, once);
}

} // namespace

std::span<const Theme> Themes() {
    return kThemes;
}

const std::vector<Monster> &DungeonNatives(int dungeon) {
    static const std::vector<Monster> none;
    return dungeon >= 0 && dungeon < kDungeons ? kNatives[dungeon] : none;
}

double ThemeWeight(const Theme &theme, int dungeon) {
    if (theme.home < 0) {
        return theme.weight;
    }
    double near = 1.0 - std::abs(dungeon - theme.home) / static_cast<double>(kDungeons - 1);
    double pull = kFarWeight + (1.0 - kFarWeight) * near;
    return theme.weight * (dungeon == theme.home ? pull * kHomeBoost : pull);
}

Roster ThemedRoster(int dungeon, int floor, std::mt19937 &random, std::string_view *name) {
    std::optional<DungeonMimics> dungeon_mimics = MimicsOf(dungeon, floor);
    if (!dungeon_mimics) {
        return {};
    }
    const DungeonMimics &mimics = *dungeon_mimics;

    // The themes by weight, the dungeon's Mimics last at 1.
    double total = 1.0;
    for (const Theme &theme : kThemes) {
        total += ThemeWeight(theme, dungeon);
    }
    double       roll = std::uniform_real_distribution<double>(0.0, total)(random);
    const Theme *theme = nullptr;
    for (const Theme &candidate : kThemes) {
        roll -= ThemeWeight(candidate, dungeon);
        if (roll < 0.0) {
            theme = &candidate;
            break;
        }
    }

    std::vector<Monster> members = theme != nullptr ? theme->members : std::vector<Monster>{mimics.mimic, mimics.king};
    std::shuffle(members.begin(), members.end(), random);
    if (name != nullptr) {
        *name = theme != nullptr ? theme->name : "Mimics";
    }
    auto single = [&](Monster kind) {
        return theme != nullptr && std::find(theme->single.begin(), theme->single.end(), kind) != theme->single.end();
    };

    Roster roster;
    if (theme == nullptr || theme->whole_set) {
        // The whole set first; then the mimics it brings, or by chance one-of-each over fillers that fit.
        for (Monster kind : members) {
            Place(roster, kind, single(kind));
        }
        bool capped = false;
        if (theme != nullptr && theme->with_mimics) {
            Place(roster, mimics.mimic, false);
            Place(roster, mimics.king, false);
        } else if (theme != nullptr && Chance(random, kThemeCapOneChance)) {
            for (Monster kind : Fillers(dungeon, mimics, random)) {
                if (!roster.Contains(kind) && roster.Add(kind, false)) {
                    capped = true;
                }
            }
        }
        if (capped) {
            for (Monster kind : members) {
                roster.SetOnce(kind, true);
            }
        }
    } else if (!Chance(random, kThemeCapOneChance)) {
        // The group fills the floor.
        for (Monster kind : members) {
            Place(roster, kind, single(kind));
        }
    } else {
        // One of each, over a repeatable filler placed first so the floor fills.
        std::vector<Monster> fillers = Fillers(dungeon, mimics, random);
        roster.Add(fillers.front(), false);
        for (Monster kind : members) {
            Place(roster, kind, true);
        }
        for (Monster kind : fillers) {
            Place(roster, kind, false);
        }
    }
    // A king mimic spawns once, but for the dungeon's Mimics.
    if (theme != nullptr) {
        roster.SetOnce(mimics.king, true);
    }
    return roster;
}

} // namespace expanded
