#include "expanded/data/data.hpp"

#include <format>
#include <stdexcept>

#include "expanded/data/boss_model.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/data/monster_scripts.hpp"
#include "expanded/data/prop_models.hpp"
#include "expanded/data/script_damage.hpp"
#include "expanded/data/spirit_cat.hpp"
#include "expanded/data/town_model.hpp"

namespace expanded {

int BuildExpandedData(const std::filesystem::path &data, std::FILE *log) {
    return BuildTownModels(data, log) + BuildBossModels(data, log) + BuildGemronModels(data, log) + BuildSpiritCatModels(data, log) +
           BuildMonsterScripts(data, log) + BuildScriptDamage(data, log) + BuildPropModels(data, log);
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
