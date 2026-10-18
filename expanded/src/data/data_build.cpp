#include "expanded/data/data_build.hpp"

#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace expanded {

namespace fs = std::filesystem;

Bytes ReadDataFile(const fs::path &file) {
    std::ifstream stream(file, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(std::format("cannot read {}", file.string()));
    }
    return Bytes(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

bool WriteIfChanged(const fs::path &target, std::span<const unsigned char> bytes) {
    std::error_code error;
    if (fs::is_regular_file(target, error) && fs::file_size(target, error) == bytes.size()) {
        Bytes current = ReadDataFile(target);
        if (std::equal(current.begin(), current.end(), bytes.begin(), bytes.end())) {
            return true;
        }
    }
    fs::create_directories(target.parent_path());
    std::ofstream stream(target, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    if (!stream) {
        throw std::runtime_error(std::format("cannot write {}", target.string()));
    }
    return false;
}

int RunDataBuilds(const fs::path &data, std::string_view kind, const std::vector<DataBuild> &builds, std::FILE *log) {
    auto say = [&](const std::string &line) {
        if (log != nullptr) {
            std::fputs(line.c_str(), log);
        }
    };
    DiscReader read = [&](std::string_view path) { return ReadDataFile(data / fs::path(path)); };
    int        failures = 0;
    for (const DataBuild &build : builds) {
        std::error_code error;
        auto            missing = std::find_if(build.sources.begin(), build.sources.end(),
                                               [&](const std::string &source) { return !fs::is_regular_file(data / fs::path(source), error); });
        if (missing != build.sources.end()) {
            say(std::format("{} {}: skipped, the data has no {}\n", kind, build.name, *missing));
            continue;
        }
        try {
            Bytes model = build.build(read);
            bool  unchanged = WriteIfChanged(data / fs::path(build.target), model);
            say(std::format("{} {}: {} ({} bytes{})\n", kind, build.name, build.target, model.size(), unchanged ? ", unchanged" : ""));
        } catch (const std::exception &failure) {
            say(std::format("{} {}: not built: {}\n", kind, build.name, failure.what()));
            failures++;
        }
    }
    return failures;
}

} // namespace expanded
