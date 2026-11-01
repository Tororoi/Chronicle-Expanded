#pragma once

// The allies' town models, their motion slots grafted from the disc's other models of them
// (expanded/docs/town-models.md). Includes no game header, so dcdata can build with it.

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"

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

// A motion of Toan's played by the ally: frames [src_lo, src_hi] of src's src_mot written into the ally's body and shadow
// motions at dest_lo on. Each joint pair turns the ally's joint from its idle (its frame ref) the way Toan's turns in the
// world from his frame src_ref; the other joints keep their idle under their parent. The hips move by Toan's hip travel
// times hip_scale.
struct TownRetarget {
    // Toan's model and his motion.
    std::string_view rig;
    std::string_view rig_mds;
    std::string_view src;
    std::string_view src_mot;
    int              src_lo = 0;
    int              src_hi = 0;
    int              src_ref = 0;
    int              dest_lo = 0;
    int              ref = 0;
    double           hip_scale = 1.0;
    std::string_view src_hip;
    std::string_view hip;
    // Toan's joint, then the ally's.
    std::vector<std::pair<std::string_view, std::string_view>> joints;
};

// Joints borrowed from one of the ally's own clips across a retarget: their idle, eased into the loop [lo, hi] at speed over
// Toan's frames ease, the loop on, then eased back to idle over rest. queue: the KEYs (lo, hi, speed) the game plays Toan's
// frames through, which set how fast the loop runs.
struct TownBorrowedJoints {
    std::vector<std::string_view>             nodes;
    int                                       lo = 0;
    int                                       hi = 0;
    double                                    speed = 0;
    std::pair<int, int>                       ease{};
    std::pair<int, int>                       rest{};
    std::vector<std::tuple<int, int, double>> queue;
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
    // Motions of Toan's written in after the slots, each with joints borrowed back from the ally's own clips.
    std::optional<TownRetarget>       retarget;
    std::optional<TownBorrowedJoints> borrowed;
};

// The allies built, in order.
const std::vector<TownAlly> &TownAllies();

// The ally's model: its base and every source read through read.
Bytes BuildTownModel(const TownAlly &ally, const DiscReader &read);

// The KEY_START block's KEYs replaced with one per slot, in idx order.
Bytes RewriteTownKeys(std::span<const unsigned char> cfg, const std::vector<TownSlot> &slots);

// Grafts src's frames [slo, shi] into base's body and shadow motions at [dlo, dhi] by joint name, keying every
// grafted track at the window's edges; src_cfg names src's cfg where its pack bundles two characters.
void GraftMotionWindow(ChrPack &base, const ChrPack &src, std::string_view src_cfg, int slo, int shi, int dlo, int dhi);

// Every ally's model, each skipped when its base is missing (its slots' sources are read as it builds).
std::vector<DataBuild> TownModelBuilds();

} // namespace expanded
