#pragma once

// The allies' town models, their motion slots grafted from the disc's other models of them
// (expanded/docs/town-models.md). Includes no game header, so dcdata can build with it.

#include <array>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "expanded/data/chr_pack.hpp"

namespace expanded {

// One KEY, frames [lo, hi] at speed: the base's own, or src's frames win grafted in; then the
// post-ops below, in order.
struct TownSlot {
    int                 idx = 0;
    int                 lo = 0;
    int                 hi = 0;
    double              speed = 0;
    std::string_view    name;
    std::string_view    src;
    std::pair<int, int> win{};
    // The source's cfg, where a pack bundles two characters.
    std::string_view src_cfg;
    // Added to the root's translation in the window.
    std::optional<std::array<double, 3>> root_offset;
    // Plays the window backwards.
    bool reverse = false;
    // Holds the pose of this dest frame across the window.
    std::optional<int> hold;
    // Sets every translation in the window to its value at this dest frame.
    std::optional<int> root_freeze;
    // Holds these nodes at their pose of dest frame pin_frame across the window.
    std::vector<std::string_view> pin_nodes;
    int                           pin_frame = 0;
};

// A grafted hub spun about axis at rate degrees a frame, easing in and out.
struct TownSpin {
    std::string_view      node;
    std::array<double, 3> axis{};
    double                rate = 0;
    double                handoff_advance = 0;
    std::pair<int, int>   ramp_up{};
    std::pair<int, int>   loop{};
    std::pair<int, int>   ramp_down{};
};

// Nodes the base lacks, appended from src and held at its pose_frame outside src's windows.
struct TownMeshGraft {
    std::string_view              src;
    int                           pose_frame = 0;
    std::vector<std::string_view> nodes;
    std::optional<TownSpin>       spin;
};

// A shadow for a base without one: these records of src copied in and declared in the cfg.
struct TownShadowInject {
    std::string_view src;
    std::string_view model;
    std::string_view mot;
    std::string_view bbp;
    std::string_view wgt;
};

struct TownAlly {
    std::string_view name;
    // The built model's file under the data directory.
    std::string_view file;
    std::string_view base;
    // Grafts root and hip channels the base's motion lacks, aliasing a differently named root.
    bool                            root_motion = false;
    std::optional<TownMeshGraft>    mesh_graft;
    std::optional<TownShadowInject> shadow_inject;
    std::vector<TownSlot>           slots;
};

// The allies built, in order.
const std::vector<TownAlly> &TownAllies();

// Reads a disc file by its path in the data directory.
using DiscReader = std::function<Bytes(std::string_view path)>;

// The ally's model: its base and every source read through read.
Bytes BuildTownModel(const TownAlly &ally, const DiscReader &read);

// The KEY_START block's KEYs replaced with one per slot, in idx order.
Bytes RewriteTownKeys(std::span<const unsigned char> cfg, const std::vector<TownSlot> &slots);

// Grafts src's frames [slo, shi] into base's body and shadow motions at [dlo, dhi] by joint name, keying every
// grafted track at the window's edges; src_cfg names src's cfg where its pack bundles two characters.
void GraftMotionWindow(ChrPack &base, const ChrPack &src, std::string_view src_cfg, int slo, int shi, int dlo, int dhi);

// Builds every ally's model into data, skipping one whose base is missing. Returns the failures.
int BuildTownModels(const std::filesystem::path &data, std::FILE *log);

} // namespace expanded
