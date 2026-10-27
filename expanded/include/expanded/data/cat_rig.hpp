#pragma once

// The rig and model reading the spirit cat's bake (spirit_cat.hpp) is built on, operation for operation as the PS2 mod's
// (its RigMath, ExactMath and ModelCodec, which mirror the Python the cat was authored in), so the built bytes are the
// same: the shared maths (rig_math.hpp) with every dot product a Python sum, the tracks, skins and fits over them, and
// plane geometry. Includes no game header, so dcdata can build with it.

#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/rig_math.hpp"

namespace expanded::rig {

// The shared maths' types under the short names the cat's code uses, and a triangle's three vertex indices.
using V2 = Vec2;
using V3 = Vec3;
using V4 = Vec4;
using M3 = Mat3;
using M4 = Mat4;
using Tri = std::array<int, 3>;

// Keys (frame, value) held outside, smoothstepped between.
double KeyedVal(std::span<const std::pair<double, double>> keys, double f);

// A motion track as frames and values (rotations, mot::kRotation, slerped; others lerped).
struct Track {
    int                 node = 0, chan = 0;
    std::vector<double> frames;
    std::vector<V4>     vals;
};

V4 Sample(const Track &track, double f);

// Small solves: Gaussian elimination with partial pivoting and a 4x4 inverse; nullopt when singular.
std::optional<std::vector<double>> Solve(std::vector<std::vector<double>> a, const std::vector<double> &b);
std::optional<M4>                  Inv4(const M4 &m);

// A bone's posed world in column form (world = r * p + t), a two-bone linear blend and the bone-local positions that land
// a two-bone vertex on targets in two poses (nullopt when it explodes).
struct ColumnPose {
    M3 r;
    V3 t;
};

ColumnPose                       Col(const M4 &m);
V3                               Blend(double w, const ColumnPose &m0, const ColumnPose &m1, const V3 &p0, const V3 &p1);
std::optional<std::pair<V3, V3>> TwoPoseFit(double w, const std::pair<ColumnPose, ColumnPose> &ma, const std::pair<ColumnPose, ColumnPose> &mb,
                                            const V3 &target_a, const V3 &target_b, double guard = 50.0);
V3                               PcaNormal(const std::vector<V3> &points, const V3 &centre);

// An .mds frame as a rig node: its record's rotation rows and translation, and its bind world.
struct RigNode {
    int         i = 0, mesh_off = 0, parent = -1;
    std::string name;
    M3          r{};
    V3          t{};
    V4          quat{};
    M4          world{}, inv_world{};
    V3          world_pos{};

    void Bind(const std::vector<RigNode> &nodes);
};

std::vector<RigNode> ReadSkeleton(std::span<const unsigned char> mds);

// The node named name, throwing when the rig has none.
const RigNode &NodeNamed(const std::vector<RigNode> &nodes, std::string_view name);

// A skinned mesh as two bone influences a vertex (b0/p0 weighted w0, b1/p1 the rest).
struct SkinMesh {
    int                 node = 0, nv = 0;
    bool                skin = false;
    std::vector<Tri>    tris;
    std::vector<int>    b0, b1;
    std::vector<V3>     p0, p1;
    std::vector<double> w0;
};

V3 Skinned(const SkinMesh &skin, int i, const std::function<const M4 &(int)> &world);

// A mesh node's .wgt weights: each vertex's (bone, weight 0..1), the vertices in the order they first appear.
struct VertexWeights {
    std::vector<int>                                             order;
    std::unordered_map<int, std::vector<std::pair<int, double>>> of;
    const std::vector<std::pair<int, double>>                   *Find(int vertex) const;
};

// Every mesh node's, from the pack's .wgt record name (empty when it has none).
std::unordered_map<int, VertexWeights> LoadWeights(const ChrPack &pack, std::string_view wgt);

// Position-index triangles of an MDT (strips with their degenerates dropped).
std::vector<Tri> MdtTriangles(const MdtMesh &m);
// A node's mesh skinned to the two nearest joints when it is a large one, else rigid to the node.
std::optional<SkinMesh> BuildMesh(std::span<const unsigned char> mds, const RigNode &node, const std::vector<RigNode> &nodes);
// A node's mesh with its .wgt weights (the top two, renormalised).
std::optional<SkinMesh> BuildMeshWeighted(std::span<const unsigned char> mds, const RigNode &node, const std::vector<RigNode> &nodes,
                                          const VertexWeights &weights);
// The rotation and translation tracks of a .mot for nodes below count.
std::vector<Track> BuildTracks(const ChrPack &pack, std::string_view mot, int count);

// A pack's first cfg, and a cfg's MODEL and first MOTION .mot names.
const ChrRecord                    *FindCfg(const ChrPack &pack);
std::pair<std::string, std::string> CfgNames(std::span<const unsigned char> cfg);

// Plane geometry: Bowyer-Watson Delaunay triangles (counter-clockwise, in the Python's order) and point in polygon.
std::vector<Tri> Delaunay(const std::vector<V2> &points);
bool             PtInRing(const V2 &p, const std::vector<V2> &ring);

} // namespace expanded::rig
