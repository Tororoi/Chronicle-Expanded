#pragma once

// What Chronicle-Expanded derives from the extracted disc data, built into the data directory.
// Includes no game header, so dcdata can build with it.

#include <cstdio>
#include <filesystem>

namespace expanded {

// Builds every derived file into data, at the end of an extraction. Returns the failures.
int BuildExpandedData(const std::filesystem::path &data, std::FILE *log);

// dcdata expanded: rebuilds them in a data directory extracted before. Returns the exit status.
int RebuildExpandedData(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
