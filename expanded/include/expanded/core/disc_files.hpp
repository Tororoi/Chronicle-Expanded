#pragma once

#include <string>
#include <string_view>

namespace expanded {

// The file the game reads for a disc file: the copy dcdata rebuilt for Expanded (under expanded/disc/) where there is
// one.
std::string DiscFile(std::string_view path);

} // namespace expanded
