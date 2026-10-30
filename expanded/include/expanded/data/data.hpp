#pragma once

// What Chronicle-Expanded derives from the extracted disc data, built into the data directory.
// Includes no game header, so dcdata can build with it.

#include <cstdio>
#include <filesystem>
#include <functional>

namespace expanded {

// Builds every derived file into data, at the end of an extraction. Returns the failures.
int BuildExpandedData(const std::filesystem::path &data, std::FILE *log);

// Converts the textures of the game files under raw into baked texture documents under out/data/textures, at the same
// paths as the files (dcdata's texture conversion).
using TextureBaker = std::function<void(const std::filesystem::path &raw, const std::filesystem::path &out)>;

// The data's baked textures of what is built (textures/expanded/) baked again from what is built (expanded/): the game
// reads a built file's textures from them, by the file's path. A data directory with no baked textures (the game decodes
// the raw ones) is left as it is.
void BakeExpandedTextures(const std::filesystem::path &data, const TextureBaker &bake, std::FILE *log);

// dcdata expanded: rebuilds them in a data directory extracted before, then their baked textures. Returns the exit
// status.
int RebuildExpandedData(const std::filesystem::path &data, const TextureBaker &bake, std::FILE *log);

} // namespace expanded
