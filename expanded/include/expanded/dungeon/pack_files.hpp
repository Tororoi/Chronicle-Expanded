#pragma once

#include <string>
#include <vector>

namespace expanded {

// A disc file Expanded loads on a floor, read through read_buffer the first time and kept for the session (so a load
// mid-floor reads no disc), or null when it cannot be read.
const std::vector<unsigned char> *CachedPackFile(const std::string &path);

// Whether the images of a pack (as GetPackFile reads it) fit what is left of the texture table, past kTextureReserve, and
// of the VRAM above texture block block; says what was refused for what when they do not.
bool PackTexturesFit(unsigned int *pack, int block, const char *what);

// Free texture table entries kept back for what the dungeon enters after a load (menus, item models).
inline constexpr int kTextureReserve = 48;

} // namespace expanded
