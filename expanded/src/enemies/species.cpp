#include "expanded/enemies/species.hpp"

#include <cstring>
#include <filesystem>
#include <iterator>
#include <optional>
#include <vector>

#include "expanded/enemies/enemy_events.hpp"
#include "monstorunit.hpp"
#include "platform/paths.hpp"

namespace expanded {

namespace {

// SpeciesHeight's table, by kind.
// clang-format off
constexpr float kSpeciesHeights[] = {
    16.0f, // 0 Master Jacket
    16.0f, // 1 Skeleton Soldier
    20.5f, // 2 Statue
    18.5f, // 3 Dasher
    17.0f, // 4 Werewolf
    16.5f, // 5 FliFli
    4.0f, // 6 Hornet
    19.0f, // 7 Halloween
    19.0f, // 8 Cannibal Plant
    16.0f, // 9 Earth Digger
    12.0f, // 10 Sunday
    12.0f, // 11 Monday
    12.0f, // 12 Tuesday
    12.0f, // 13 Wednesday
    12.0f, // 14 Thursday
    12.0f, // 15 Friday
    12.0f, // 16 Saturday
    16.0f, // 17 Witch Hellza
    16.0f, // 18 Witch Illza
    18.0f, // 19 Gunny
    20.0f, // 20 Gyon
    20.0f, // 21 Pirate's Chariot
    17.5f, // 22 Auntie Medu
    17.5f, // 23 Captain
    17.0f, // 24 Corcea
    27.0f, // 25 Golem
    19.0f, // 26 Mr. Blare
    16.0f, // 27 Dune
    23.5f, // 28 Titan
    27.0f, // 29 King Mimic (Divine Beast Cave)
    16.0f, // 30 Mimic (Divine Beast Cave)
    27.0f, // 31 King Mimic (Sun & Moon Temple)
    16.0f, // 32 Mimic (Sun & Moon Temple)
    27.0f, // 33 King Mimic (Moon Sea)
    16.0f, // 34 Mimic (Moon Sea)
    26.0f, // 35 Arthur
    13.5f, // 36 Ghost
    23.5f, // 37 Alexander
    14.5f, // 38 Heart
    14.5f, // 39 Club
    14.5f, // 40 Diamond
    14.5f, // 41 Spade
    14.5f, // 42 Joker
    15.0f, // 43 Bomber Head
    16.0f, // 44 Mummy
    13.5f, // 45 Lich
    16.0f, // 46 Curse Dancer
    20.5f, // 47 Living Armor
    17.0f, // 48 White Fang
    16.5f, // 49 Moon Bug
    4.0f, // 50 Phantom
    27.0f, // 51 Dragon
    6.0f, // 52 Cave Bat
    6.0f, // 53 Evil Bat
    12.0f, // 54 Hell Pockle
    18.5f, // 55 Rash Dasher
    27.0f, // 56 Steel Giant
    23.5f, // 57 Blizzard
    16.0f, // 58 Moon Digger
    22.5f, // 59 Dark Flower
    19.5f, // 60 Cursed Rose
    18.0f, // 61 Billy
    16.0f, // 62 Vulcan
    17.5f, // 63 Crabby Hermit
    20.0f, // 64 Space Gyon
    25.0f, // 65 Blue Dragon
    28.0f, // 66 Black Dragon
    18.5f, // 67 Mask of Prajna
    24.0f, // 68 Crescent Baron
    16.0f, // 69 Rockanoff
    27.0f, // 70 King Mimic (Wise Owl Forest)
    16.0f, // 71 Mimic (Wise Owl Forest)
    27.0f, // 72 King Mimic (Shipwreck)
    16.0f, // 73 Mimic (Shipwreck)
    27.0f, // 74 King Mimic (Gallery of Time)
    16.0f, // 75 Mimic (Gallery of Time)
    17.0f, // 76 Ice Arrow
    18.0f, // 77 Sam
    60.5f, // 78 Dran
    44.0f, // 79 Master Utan
    21.0f, // 80 Ice Queen
    36.0f, // 81 King's Curse Coffin
    40.5f, // 82 King's Curse
    46.0f, // 83 Minotaur Joe
    59.0f, // 84 Dark Genie
    54.0f, // 85 Dark Genie Right Hand
    54.0f, // 86 Dark Genie Left Hand
    17.0f, // 87 (DG effect c17_)
    17.0f, // 88 (DG companion c17_)
    17.0f, // 89 (DG companion c17_)
    17.0f, // 90 (DG companion c17_)
    6.0f, // 91 Wine Keg
    17.0f, // 92 Ice Aura
    17.0f, // 93 (DG companion c17_)
    27.0f, // 94 Gol
    27.0f, // 95 Sil
    16.0f, // 96 Yammich
    17.0f, // 97 Statue Dog
    43.0f, // 98 Opar
    12.0f, // 99 Haley Holey
    15.0f, // 100 King Prickly
    17.0f, // 101 Ice Barrier
    17.0f, // 102 Ice Prison
    17.0f, // 103 Ice Meteor
    17.0f, // 104 Ice Tornado
    22.5f, // 105 Gacious
    34.5f, // 106 Dark Genie (Final Form)
    17.0f, // 107 DG Final summon (last_mc)
    17.0f, // 108 DG Final ground wave (last_gw1)
    17.0f, // 109 DG Final beam
    17.0f, // 110 DG Final beam (small)
    19.0f, // 111 Gemron (Fire)
    32.0f, // 112 Nikapous
    17.0f, // 113 White Fang (Enhanced)
    26.0f, // 114 Arthur (Enhanced)
    27.0f, // 115 Sil (Enhanced)
    19.0f, // 116 Halloween (Enhanced)
    16.0f, // 117 Master Jacket (Enhanced)
    16.0f, // 118 Vulcan (Enhanced)
    16.0f, // 119 Mummy (Enhanced)
    14.5f, // 120 Diamond (Enhanced)
    19.0f, // 121 Gemron (Ice)
    16.5f, // 122 Horn Head
    17.5f, // 123 Auntie Medu (Enhanced)
    16.0f, // 124 Rockanoff (Enhanced)
    16.0f, // 125 Yammich (Enhanced)
    16.0f, // 126 Witch Hellza (Enhanced)
    27.0f, // 127 Steel Giant (Enhanced)
    14.5f, // 128 Club (Enhanced)
    17.0f, // 129 Corcea (Enhanced)
    16.0f, // 130 Mimic (Demon Shaft)
    27.0f, // 131 King Mimic (Demon Shaft)
    19.0f, // 132 Gemron (Thunder)
    16.0f, // 133 Bishop Q
    6.0f, // 134 Cave Bat (Enhanced)
    27.0f, // 135 Gol (Enhanced)
    18.5f, // 136 Mask of Prajna (Enhanced)
    20.0f, // 137 Gyon (Enhanced)
    14.5f, // 138 Spade (Enhanced)
    18.5f, // 139 Rash Dasher (Enhanced)
    17.5f, // 140 Captain (Enhanced)
    16.0f, // 141 Mimic (Demon Shaft) (Enhanced)
    27.0f, // 142 King Mimic (Demon Shaft) (Enhanced)
    19.0f, // 143 Gemron (Wind)
    17.5f, // 144 Silver Gear
    23.5f, // 145 Alexander (Enhanced)
    14.5f, // 146 Heart (Enhanced)
    15.0f, // 147 Bomber Head (Enhanced)
    17.5f, // 148 Crabby Hermit (Enhanced)
    19.5f, // 149 Cursed Rose (Enhanced)
    20.0f, // 150 Pirate's Chariot (Enhanced)
    20.0f, // 151 Space Gyon (Enhanced)
    16.0f, // 152 Mimic (Demon Shaft) (Enhanced x2)
    27.0f, // 153 King Mimic (Demon Shaft) (Enhanced x2)
    19.0f, // 154 Gemron (Holy)
    22.5f, // 155 Gacious (Enhanced)
    6.0f, // 156 Evil Bat (Enhanced)
    24.0f, // 157 Crescent Baron (Enhanced)
    17.0f, // 158 Statue Dog (Enhanced)
    14.5f, // 159 Joker (Enhanced)
    13.5f, // 160 Lich (Enhanced)
    23.5f, // 161 Titan (Enhanced)
    20.5f, // 162 Living Armor (Enhanced)
    16.0f, // 163 Mimic (Demon Shaft) (Enhanced x3)
    27.0f, // 164 King Mimic (Demon Shaft) (Enhanced x3)
    30.0f, // 165 Black Knight
    35.5f, // 166 Black Knight Mount
    19.0f, // 167 Bomb Gemron
    19.0f, // 168 Crystal Gemron
    19.0f, // 169 Atla Gemron
};
// clang-format on

struct Entry {
    ExpandedSpecies species;
    MONSTOR_MODEL   record;
};

std::vector<Entry> &Registry() {
    static std::vector<Entry> entries;
    return entries;
}

Entry *Find(int model_no) {
    for (Entry &entry : Registry()) {
        if (No(entry.species.kind) == model_no) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

bool AddSpecies(const ExpandedSpecies &species) {
    int model_no = No(species.kind);
    if (model_no < kRetailSpecies || model_no >= kSpeciesCount || Find(model_no) != nullptr) {
        return false;
    }
    Entry entry{species, MonstorTable[No(species.stand_in)]};
    MONSTOR_MODEL &record = entry.record;
    std::memset(record.model_name[0], 0, sizeof(record.model_name[0]));
    std::strncpy(record.model_name[0], species.model_name, sizeof(record.model_name[0]) - 1);
    for (std::size_t i = 0; i < species.elements.size(); i++) {
        record.attachment_weight[i] = static_cast<s16>(species.elements[i]);
    }
    record.shot_effect[0] = record.shot_effect[1] = -1;
    record.exp = static_cast<s16>(species.abs);
    record.name_no = static_cast<s16>(species.name_no);
    record.steal_item = static_cast<s16>(species.steal_item);
    record.rare_item = static_cast<s16>(species.rare_item);
    Registry().push_back(entry);
    return true;
}

const MONSTOR_MODEL *Species(int model_no) {
    if (model_no >= 0 && model_no < kRetailSpecies) {
        return &MonstorTable[model_no];
    }
    Entry *entry = Find(model_no);
    return entry != nullptr ? &entry->record : nullptr;
}

bool SpeciesBuilt(int model_no) {
    if (model_no >= 0 && model_no < kRetailSpecies) {
        return true;
    }
    Entry *entry = Find(model_no);
    if (entry == nullptr) {
        return false;
    }
    std::error_code error;
    for (const char *file : {entry->species.model_file, entry->species.script_file}) {
        if (file != nullptr && !std::filesystem::is_regular_file(PathsDataRoot() / file, error)) {
            return false;
        }
    }
    return true;
}

int SpeciesStandIn(int model_no) {
    Entry *entry = Find(model_no);
    return entry != nullptr ? No(entry->species.stand_in) : model_no;
}

MONSTOR_MODEL *SpeciesRecord(int model_no) {
    return const_cast<MONSTOR_MODEL *>(Species(model_no));
}

void SpeciesPackFile(char *filename, int model_no) {
    if (Entry *entry = Find(model_no)) {
        std::strcpy(filename, entry->species.model_file);
    }
}

void SpeciesScriptFile(char *filename, int model_no) {
    if (Entry *entry = Find(model_no); entry != nullptr && entry->species.script_file != nullptr) {
        std::strcpy(filename, entry->species.script_file);
    }
}

float SpeciesHeight(int model_no) {
    if (model_no >= 0 && model_no < static_cast<int>(std::size(kSpeciesHeights))) {
        return kSpeciesHeights[model_no];
    }
    int stand_in = SpeciesStandIn(model_no);
    return stand_in != model_no && stand_in >= 0 ? SpeciesHeight(stand_in) : kSpeciesHeightFallback;
}

float MonsterHeight(const CMonstorUnit &unit, int monster) {
    float scale = unit.chara[monster][0].scale[1];
    scale = scale > 0.05f && scale < 20.0f ? scale : 1.0f;
    return SpeciesHeight(ModelKind(unit.monster[monster].base_model)) * scale;
}

} // namespace expanded
