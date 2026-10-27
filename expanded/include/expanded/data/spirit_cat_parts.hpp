#pragma once

// The spirit cat bake's parts (spirit_cat.hpp builds the pack from them), each the PS2 mod's: the wings grafted from Dran
// (CatWingGraft), the Super Steve cape's rest lattice and collision bounds (CatCapeBake) and the domino mask (CatMaskBake).
// Includes no game header, so dcdata can build with it.

#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "expanded/data/cat_rig.hpp"

namespace expanded::spirit_cat {

using Log = std::function<void(const std::string &)>;

// The cat's keys as the mod lists them (start, end, speed); the pack's ids are kKeyStart + index.
struct CatKeyWindow {
    int         start, end;
    double      speed;
    const char *comment;
};

inline constexpr int          kKeyStart = 64;
inline constexpr CatKeyWindow kCatKeys[] = {
    {10,  20,  0.1,  "cat stand"                           },
    {95,  105, 0.4,  "cat ready"                           },
    {120, 136, 0.5,  "cat run"                             },
    {190, 204, 0.5,  "cat take-off"                        },
    {205, 214, 0.5,  "cat leap"                            },
    {215, 227, 0.36, "cat land"                            },
    {60,  80,  1.0,  "cat walk (s86 KEY 2, brisk)"         },
    {285, 294, 0.6,  "cat float-up (e04c04cat #5 160..169)"},
    {30,  40,  0.1,  "cat sit (s86 KEY 1)"                 },
};

// The wings on the cat (its rig named catroot / cat_*, posed as the bake's pack has it): the cat's nodes then Dran's 8
// wing bones, the two wing meshes then the cat's skin, the cat's tracks then the wings', and what the encoder reads back.
struct WingData {
    struct WingSet {
        std::vector<rig::Tri>        tris;
        std::vector<int>             used;
        std::unordered_map<int, int> remap;
    };

    struct BindOpen {
        rig::M3 r;
        rig::V3 t;
        rig::M4 world;
    };

    std::vector<rig::RigNode>      nodes, cat_nodes;
    std::vector<rig::SkinMesh>     meshes;
    std::vector<rig::Track>        tracks;
    int                            base = 0, parent = 0;
    std::map<std::string, WingSet> wing_sets;
    MdtMesh                        om;
    std::map<int, rig::M4>         wref;
    std::vector<rig::M4>           wcat;
    std::map<int, BindOpen>        bind_open;
    Bytes                          mds;
    ChrPack                        pack;
};

// The graft on the wingless cat: Dran's pack, the cat rig's .mds and a pack holding its "cat.mot" and "cat.wgt".
WingData WingGraft(std::span<const unsigned char> dran, const Bytes &cat_mds, const ChrPack &cat_pack, const Log &log);

// The cape.
inline constexpr const char *kCapeAnchor = "cat_sebone2";
inline constexpr int         kCapeRows = 16, kCapeCols = 12;

struct CapePhysics {
    double k, gravity, follow0, follow1, follow2, wind, normal;
};

inline constexpr CapePhysics kCapePhysics = {0.12, 0.0, 0.40, 0.30, 0.40, 0.25, -1.0};

struct CapeBound {
    const char *bone;
    rig::V3     centre, axis, radii;
    double      damp;
};

inline constexpr CapeBound kCapeBounds[] = {
    {"cat_sebone2",  {0.0, 3.05, 2.00},  {0.0, 0.0, 1.0}, {1.25, 0.70, 2.50}, 0.7},
    {"cat_sebone1",  {0.0, 3.16, -0.30}, {0.0, 0.0, 1.0}, {1.25, 0.70, 2.30}, 0.7},
    {"cat_kosibone", {0.0, 2.75, -1.00}, {0.0, 0.0, 1.0}, {1.35, 1.10, 2.20}, 0.7},
    {"cat_kao",      {0.0, 3.15, 3.55},  {0.0, 0.0, 1.0}, {1.10, 1.00, 0.90}, 0.7},
};

// A bound as a .clo BOUND line has it, in its bone's bind space: the capsule's ends, the up direction, its radii.
struct BoundLocal {
    std::string bone;
    rig::V3     a, b, up, radii;
    double      damp;
};

std::vector<BoundLocal> CapeBoundsLocal(const std::vector<rig::RigNode> &cat_nodes);
// The rest lattice row-major from the collar row, in the anchor's bind space.
std::vector<rig::V3> CapeRestLocal(const std::vector<rig::RigNode> &cat_nodes, const rig::SkinMesh &skin);

// The mask, rigid to its anchor (positions in the head's bind space).
inline constexpr const char *kMaskAnchor = "cat_kao";
rig::SkinMesh                BuildMaskMesh(const std::vector<rig::RigNode> &cat_nodes, const rig::SkinMesh &skin, const Log &log);

} // namespace expanded::spirit_cat
