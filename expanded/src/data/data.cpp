#include "expanded/data/data.hpp"

#include <format>
#include <stdexcept>

#include "expanded/data/boss_model.hpp"
#include "expanded/data/script_damage.hpp"
#include "expanded/data/town_model.hpp"

namespace expanded {

int BuildExpandedData(const std::filesystem::path &data, std::FILE *log) {
    return BuildTownModels(data, log) + BuildBossModels(data, log) + BuildScriptDamage(data, log);
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
