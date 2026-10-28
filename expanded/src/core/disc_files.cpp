#include "expanded/core/disc_files.hpp"

#include "data/data_read.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

std::string DiscFile(std::string_view path) {
    std::string rebuilt = std::string(kRebuiltDiscFolder) + std::string(path);
    return DataFileSize(rebuilt) > 0 ? rebuilt : std::string(path);
}

} // namespace expanded
