#pragma once

// The rig maths and model reading the spirit cat's bake (spirit_cat.hpp) is built on, operation for operation as the PS2
// mod's (its RigMath, ExactMath and ModelCodec, which mirror the Python the cat was authored in), so the built bytes are
// the same: row-vector 4x4 matrices (v' = v * M), rotations as 3x3 rows, quaternions scalar first, every dot product a
// Python sum. Includes no game header, so dcdata can build with it.

#include <array>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"

namespace expanded::rig {

using V2 = std::array<double, 2>;
using V3 = std::array<double, 3>;
using V4 = std::array<double, 4>;
using M3 = std::array<V3, 3>;
using M4 = std::array<V4, 4>;
using Tri = std::array<int, 3>;

// Python's numerics: sum() (Neumaier-compensated), round(x, ndigits) (half to even on the exact value), a % b and a // b
// for floats, math.dist and math.hypot (the double-length sum with its correction), and an int % that is never negative.
double Sum(std::initializer_list<double> values);
double Sum(const std::vector<double> &values);
double Round(double x, int ndigits);
double FMod(double a, double b);
int    Mod(int a, int m);
double Dist(std::span<const double> p, std::span<const double> q);
double Hypot(std::initializer_list<double> values);

// Matrices: a * b (Python sums), from rotation and translation, a rigid inverse, a point through one.
M4 MatMul(const M4 &a, const M4 &b);
M4 MatFromRT(const M3 &r, const V3 &t);
M4 RigidInv(const M4 &m);
V3 XformPt(const M4 &m, const V3 &v);
M3 RotOf(const M4 &m);
V3 TransOf(const M4 &m);

V4 MatToQuat(const M3 &r);
M3 QuatToMat(const V4 &q);
M3 Mul3(const M3 &a, const M3 &b);
M3 T3(const M3 &a);

V3     Pitch(const V3 &v, double p);
V3     RotY(const V3 &v, double yaw);
double Sum3(const V3 &a, const V3 &b);
V3     Cross(const V3 &a, const V3 &b);
V3     Unit(const V3 &v);
double Smooth(double t);
double Radians(double deg);
V4     Slerp(const V4 &q0, V4 q1, double t);
// Rotation of +deg about axis, as row vectors.
M3 RotAbout(const V3 &axis, double deg);
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

// A pack's cfg (the first, or the first whose name holds prefer) and its MODEL and first MOTION .mot names.
const ChrRecord                    *FindCfg(const ChrPack &pack);
std::pair<std::string, std::string> CfgNames(std::span<const unsigned char> cfg);

// Plane geometry: Bowyer-Watson Delaunay triangles (counter-clockwise, in the Python's order) and point in polygon.
std::vector<Tri> Delaunay(const std::vector<V2> &points);
bool             PtInRing(const V2 &p, const std::vector<V2> &ring);

} // namespace expanded::rig
