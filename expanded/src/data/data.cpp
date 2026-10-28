#include "expanded/data/data.hpp"

#include <format>
#include <stdexcept>
#include <vector>

#include "expanded/data/boss_model.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/data/monster_scripts.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/data/queens_canal.hpp"
#include "expanded/data/script_damage.hpp"
#include "expanded/data/spirit_cat.hpp"
#include "expanded/data/town_model.hpp"

namespace expanded {

namespace {

// What is built, in order, by kind (the log's word for it): a kind's files, or a build of its own.
struct DataKind {
    const char *kind;
    std::vector<DataBuild> (*builds)();
    int (*run)(const std::filesystem::path &data, std::FILE *log) = nullptr;
};

const DataKind kDataKinds[] = {
    {"town model", TownModelBuilds},
    {"boss model", BossModelBuilds},
    {"gemron model", GemronModelBuilds},
    {"spirit cat", SpiritCatModelBuilds},
    {"monster script", MonsterScriptBuilds},
    {"script damage", nullptr, BuildScriptDamage}, // one file from every script in a folder
    {"prop model", PropModelBuilds},
    {"fishing spot", FishingSpotBuilds},
    {"queens canal", QueensCanalBuilds},
};

} // namespace

int BuildExpandedData(const std::filesystem::path &data, std::FILE *log) {
    int failures = 0;
    for (const DataKind &kind : kDataKinds) {
        failures += kind.run != nullptr ? kind.run(data, log) : RunDataBuilds(data, kind.kind, kind.builds(), log);
    }
    return failures;
}

int RebuildExpandedData(const std::filesystem::path &data, std::FILE *log) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(data / "data.hd2", error)) {
        throw std::runtime_error(std::format("{} is not an extracted data directory (it has no data.hd2)", data.string()));
    }
    int failures = BuildExpandedData(data, log);
    std::fputs(std::format("{}\n", failures ? std::format("{} failed", failures) : "done").c_str(), log);
    return failures ? 1 : 0;
}

} // namespace expanded
