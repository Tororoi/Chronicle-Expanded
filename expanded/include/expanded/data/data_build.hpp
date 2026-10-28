#pragma once

// The files dcdata builds into a data directory: each from the disc's own files there, written only when it changes.
// Includes no game header, so dcdata can build with it.

#include <cstdio>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "expanded/data/bytes.hpp"

namespace expanded {

// Where a disc file a build rebuilds goes (its path after this): the game reads it in place of the disc's.
inline constexpr std::string_view kRebuiltDiscFolder = "expanded/disc/";

// Reads a disc file by its path in the data directory.
using DiscReader = std::function<Bytes(std::string_view path)>;

// A file read whole, throwing when it cannot be.
Bytes ReadDataFile(const std::filesystem::path &file);

// Writes bytes to target unless it holds them already (true then); throws when it cannot.
bool WriteIfChanged(const std::filesystem::path &target, std::span<const unsigned char> bytes);

// One built file: its name in the log, its path in the data directory, the disc files it is skipped without, and its
// build, reading disc files through read.
struct DataBuild {
    std::string                              name;
    std::string                              target;
    std::vector<std::string>                 sources;
    std::function<Bytes(const DiscReader &)> build;

    // A build of its sources alone, given them read in their order.
    static DataBuild FromSources(std::string name, std::string target, std::vector<std::string> sources,
                                 std::function<Bytes(const std::vector<Bytes> &)> build);
};

// Builds each, logging "<kind> <name>: <target> (<size> bytes)" (", unchanged" when it was), "skipped, the data has
// no <source>", or "not built: <why>"; the number not built.
int RunDataBuilds(const std::filesystem::path &data, std::string_view kind, const std::vector<DataBuild> &builds, std::FILE *log);

} // namespace expanded
