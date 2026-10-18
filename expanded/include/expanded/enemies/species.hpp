#pragma once

#include "expanded/enemies/monsters.hpp"

struct MONSTOR_MODEL;

namespace expanded {

// Monster kinds: retail's 167 (MonstorTable), then Expanded's own, each a record of its own built on a retail kind
// (its stand-in: where it counts as native, whose sounds it makes) and a model dcdata builds.
inline constexpr int kRetailSpecies = 167;
inline constexpr int kSpeciesCount = 169;

struct ExpandedSpecies {
    Monster     kind;
    Monster     stand_in;
    const char *model_file;  // under the data directory
    const char *script_file; // a script dcdata builds, or null for the record's own
    // The record, from the stand-in's.
    void (*build)(MONSTOR_MODEL &record);
};

// Registers an Expanded kind, from a static initialiser in its own file.
bool AddSpecies(const ExpandedSpecies &species);

// A kind's record: MonstorTable's, an Expanded kind's own, or null past them.
const MONSTOR_MODEL *Species(int model_no);

// Whether a kind can be placed: retail's always, an Expanded one once dcdata has built its model.
bool SpeciesBuilt(int model_no);

// The retail kind an Expanded one is built on; a retail kind itself.
int SpeciesStandIn(int model_no);

// Hooks: SetupBaseModel's record, and the model and script files it loads.
MONSTOR_MODEL *SpeciesRecord(int model_no);
void           SpeciesPackFile(char *filename, int model_no);
void           SpeciesScriptFile(char *filename, int model_no);

} // namespace expanded
