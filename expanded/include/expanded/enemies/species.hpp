#pragma once

#include <array>
#include <string>

#include "expanded/enemies/monsters.hpp"

namespace dc::game {
class CMonstorUnit;
struct MONSTOR_MODEL;
} // namespace dc::game

namespace expanded {

// Monster kinds: retail's (MonstorTable, through the Black Knight's mount), then Expanded's own, each a record of its
// own built on a retail kind (its stand-in: where it counts as native, whose sounds it makes) and a model dcdata builds.
inline constexpr int kRetailSpecies = No(Monster::BLACK_KNIGHT_MOUNT) + 1;
inline constexpr int kSpeciesCount = No(Monster::ATLA_GEMRON) + 1;

// Whether a number is a kind's, retail's or Expanded's.
constexpr bool ValidKind(int model_no) {
    return model_no >= 0 && model_no < kSpeciesCount;
}

struct ExpandedSpecies {
    Monster     kind;
    Monster     stand_in;
    const char *model_file;  // under the data directory
    const char *script_file; // a script dcdata builds, or null for the record's own
    // The record, the stand-in's with these: the model's name, the weight of each element (fire, ice, thunder, wind,
    // holy; 100 is a normal hit, 0 immune), its ABS, name and steal and rare items. Its shots are its own, entered as
    // it is set up.
    const char        *model_name;
    std::array<int, 5> elements;
    int                abs;
    int                name_no;
    int                steal_item;
    int                rare_item;
};

// Registers an Expanded kind, from a static initialiser in its own file.
bool AddSpecies(const ExpandedSpecies &species);

// A kind's record: MonstorTable's, an Expanded kind's own, or null past them.
const MONSTOR_MODEL *Species(int model_no);

// Whether a kind can be placed: retail's always, an Expanded one once dcdata has built its model.
bool SpeciesBuilt(int model_no);

// The retail kind an Expanded one is built on; a retail kind itself.
int SpeciesStandIn(int model_no);

// A kind's height above its root, authored kind by kind (begun as the top of its highest hurt sphere in the bind pose,
// then corrected by eye): what a judgement blade or a hanging bomb clears. kSpeciesHeightFallback past the table.
inline constexpr float kSpeciesHeightFallback = 20.0f;
float                  SpeciesHeight(int model_no);
// A floor monster's height above its root: its kind's, grown with a grown monster.
float MonsterHeight(const CMonstorUnit &unit, int monster);

// Hooks: SetupBaseModel's record, and the model and script files it loads (a script the mod's bake rebuilt, its
// rebuilt copy: data/monster_scripts.hpp).
MONSTOR_MODEL *SpeciesRecord(int model_no);
void           SpeciesPackFile(std::string &filename, int model_no);
void           SpeciesScriptFile(std::string &filename, int model_no);

} // namespace expanded
