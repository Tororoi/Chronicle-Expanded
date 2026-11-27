#include "expanded/core/data_refresh.hpp"

#include <SDL3/SDL.h>

#include <cstdio>
#include <string>

#include "expanded/data/data.hpp"
#include "platform/data_archive.hpp"
#include "platform/paths.hpp"

namespace expanded {

namespace {

namespace fs = std::filesystem;

#ifdef _WIN32
constexpr const char *kDcdata = "dcdata.exe";
#else
constexpr const char *kDcdata = "dcdata";
#endif

// Runs `dcdata expanded <data>`; true when it succeeded.
bool RunDcdata(const fs::path &dcdata, const fs::path &data) {
    const std::string program = dcdata.string(), folder = data.string();
    const char       *args[] = {program.c_str(), "expanded", folder.c_str(), nullptr};
    SDL_Process      *process = SDL_CreateProcess(args, false);
    if (process == nullptr) {
        std::fprintf(stderr, "expanded data: cannot run %s: %s\n", program.c_str(), SDL_GetError());
        return false;
    }
    int status = -1;
    SDL_WaitProcess(process, true, &status);
    SDL_DestroyProcess(process);
    return status == 0;
}

} // namespace

void RefreshExpandedData(const fs::path &data) {
    std::error_code error;
    // A packed data.zip is read as it is; nothing is built into it.
    if (dc::platform::DataArchivePresent(data) || !fs::is_regular_file(data / "data.hd2", error) ||
        ExpandedDataCurrent(data)) {
        return;
    }
    if (!fs::is_directory(data / "textures", error)) {
        // The game decodes the raw textures, the built files' with them.
        const int failures = BuildExpandedData(data, stderr);
        std::fprintf(stderr, "expanded data: rebuilt for this build%s\n", failures ? " (some files failed)" : "");
        return;
    }
    // The built files' textures are baked too, which only dcdata does.
    const fs::path dcdata = PathsExecutable().parent_path() / kDcdata;
    if (fs::is_regular_file(dcdata, error) && RunDcdata(dcdata, data)) {
        std::fprintf(stderr, "expanded data: rebuilt for this build\n");
        return;
    }
    std::fprintf(stderr, "expanded data: %s was built by an older build; run `dcdata expanded %s`\n",
                 data.string().c_str(), data.string().c_str());
}

} // namespace expanded
