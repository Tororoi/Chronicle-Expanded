#pragma once

#include <deque>
#include <string>
#include <vector>

#include "dataalloc_fwd.hpp"

namespace expanded {

// A disc file Expanded loads on a floor, read through read_buffer the first time and kept for the session (so a load
// mid-floor reads no disc), or null when it cannot be read.
const std::vector<unsigned char> *CachedPackFile(const std::string &path);

// Whether the images of a pack (as GetPackFile reads it) fit what is left of the texture table, past kTextureReserve, and
// of the VRAM above texture block block; says what was refused for what when they do not.
bool PackTexturesFit(unsigned int *pack, int block, const char *what);

// Free texture table entries kept back for what the dungeon enters after a load (menus, item models).
inline constexpr int kTextureReserve = 48;

// The pack at path read whole into arena, which is sized for it: the pack, then copies times its size for what is set up
// from it (its model, motions, skins and images, and each shot's frame copy). Null when the file cannot be read.
unsigned int *ReadPackIntoArena(const std::string &path, CDataAlloc2<1> &arena, int copies);

// What Expanded loads into memory of its own on a floor: slots that only grow, so nothing in one moves, the first live of
// them in use. Iterating it visits the live ones.
template <typename Slot>
struct FloorSlots {
    std::deque<Slot> slots;
    int              live = 0;

    // The slot after the live ones, made when there is none.
    Slot &Next() {
        if (live == static_cast<int>(slots.size())) {
            slots.emplace_back();
        }
        return slots[live];
    }

    auto begin() { return slots.begin(); }

    auto end() { return slots.begin() + live; }
};

} // namespace expanded
