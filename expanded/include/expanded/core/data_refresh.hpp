#pragma once

#include <filesystem>

namespace expanded {

// At launch: rebuilds the data's derived files (expanded/data/data.hpp) when older builders built them, so a data
// directory extracted before a builder changed still gets the new files. Data with baked textures is rebuilt by the
// dcdata beside the game (`dcdata expanded`), which bakes the rebuilt files' textures too; without dcdata it is left
// as it is, with a message saying what to run.
void RefreshExpandedData(const std::filesystem::path &data);

} // namespace expanded
