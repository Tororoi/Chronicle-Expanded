#pragma once

#include <array>
#include <span>
#include <vector>

namespace expanded {

// The fishing fight's sight at the float: two thin rings standing the way to pull, the space between them filling with
// the tension bar, and a pointer (the Aquamarine's gem, dun/item/main_data/juelaqua.mds, drawn out long, no texture)
// lying the stick's way, so the pointer's tip through the inner ring pulls and through the outer holds. The rings are
// made in fishing_fight.cpp; the gem is read here.

// A point of the pointer: along it from its round end to its tip, up, and across.
using SightPoint = std::array<float, 3>;

// The pointer: its triangles (three points each).
struct FishingSight {
    std::vector<SightPoint> pointer;

    bool Complete() const { return !pointer.empty(); }
};

// The pointer's length.
inline constexpr float kSightPointerLength = 2.73f;

// The pointer from the Aquamarine's gem (juelaqua.mds), or none when it has no mesh.
std::vector<SightPoint> BuildSightPointer(std::span<const unsigned char> mds);

// The sight read from the disc the first time it is asked for (into memory of its own, not the game's read buffer,
// which a fish's close-up model may be reading into), or null when it cannot be read.
const FishingSight *LoadedFishingSight();

} // namespace expanded
