#include "expanded/data/data.hpp"

#include <chrono>
#include <format>
#include <stdexcept>
#include <vector>

#include "expanded/data/boss_model.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/fishing_poses.hpp"
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
    {"fishing pose", FishingPoseBuilds},
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

void BakeExpandedTextures(const std::filesystem::path &data, const TextureBaker &bake, std::FILE *log) {
    namespace fs = std::filesystem;
    const fs::path textures = data / "textures";
    if (!fs::is_directory(textures) || !fs::is_directory(data / "expanded")) {
        return;
    }
    const fs::path stage = fs::temp_directory_path() /
                           std::format("chronicle-expanded-textures-{}", std::chrono::steady_clock::now().time_since_epoch().count());

    struct Remove {
        fs::path path;

        ~Remove() {
            std::error_code error;
            fs::remove_all(path, error);
        }
    } remove{stage};

    fs::create_directories(stage / "raw");
    fs::copy(data / "expanded", stage / "raw" / "expanded", fs::copy_options::recursive);
    bake(stage / "raw", stage / "out");
    const fs::path baked = stage / "out" / "data" / "textures" / "expanded";
    fs::remove_all(textures / "expanded");
    int files = 0;
    if (fs::is_directory(baked)) {
        fs::copy(baked, textures / "expanded", fs::copy_options::recursive);
        for (const fs::directory_entry &entry : fs::recursive_directory_iterator(textures / "expanded")) {
            files += entry.is_regular_file();
        }
    }
    if (log != nullptr) {
        std::fputs(std::format("baked textures: textures/expanded ({} files)\n", files).c_str(), log);
    }
}

int RebuildExpandedData(const std::filesystem::path &data, const TextureBaker &bake, std::FILE *log) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(data / "data.hd2", error)) {
        throw std::runtime_error(std::format("{} is not an extracted data directory (it has no data.hd2)", data.string()));
    }
    int failures = BuildExpandedData(data, log);
    BakeExpandedTextures(data, bake, log);
    std::fputs(std::format("{}\n", failures ? std::format("{} failed", failures) : "done").c_str(), log);
    return failures ? 1 : 0;
}

} // namespace expanded
