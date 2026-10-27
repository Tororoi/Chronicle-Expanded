#pragma once

#include <span>
#include <vector>

#include "options/rows.hpp"

namespace expanded {

// The Options screen's pages with the Expanded page last, a row per toggle; without toggles, the pages as they are.
std::vector<dc::options::Page> OptionsPages(std::span<const dc::options::Page> pages);

} // namespace expanded
