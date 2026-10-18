#include "expanded/enemies/species.hpp"

#include <cstring>
#include <filesystem>
#include <optional>
#include <vector>

#include "monstorunit.hpp"
#include "platform/paths.hpp"

namespace expanded {

namespace {

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
    species.build(entry.record);
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
    Entry          *entry = Find(model_no);
    std::error_code error;
    return entry != nullptr && std::filesystem::is_regular_file(PathsDataRoot() / entry->species.model_file, error);
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

} // namespace expanded
