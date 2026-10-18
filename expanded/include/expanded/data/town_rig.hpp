#pragma once

// The town model builder's parts (town_model.cpp, town_rig.cpp, town_mesh_graft.cpp, town_motion_graft.cpp): cfg
// text, rig math and the grafts. The mod's ISO bake's math, in doubles rounded to float on store, so the bytes match
// its.

#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/town_model.hpp"

namespace expanded::town {

using Vec4 = std::array<double, 4>;
using Span = std::pair<int, int>;

// Throws what went wrong; whether text ends with tail (case folded); bytes as text and back; where what is in text
// from from, throwing when the cfg lacks it.
[[noreturn]] void Bad(const std::string &what);
bool              EndsFolded(std::string_view text, std::string_view tail);
std::string       Text(std::span<const unsigned char> bytes);
Bytes             ToBytes(std::string_view text);
std::size_t       IndexOf(std::string_view text, std::string_view what, std::size_t from = 0);

// ------------------------------------------------------------------ cfg text

// The pack's main cfg and its model and motion names; prefer names it in a two-character pack.
struct CfgMotions {
    std::size_t                cfg = 0;
    std::string                body_mot;
    std::string                body_mds;
    std::optional<std::string> shadow_mot;
    std::optional<std::string> shadow_mds;
};

CfgMotions CfgMotionsOf(const ChrPack &pack, std::string_view prefer = {});

// The body MOTION 0 line's .bbp, its second quoted name.
std::string CfgBbp(const ChrRecord &cfg);

// ------------------------------------------------------------------ math

// A .mds node's parent, bind rotation rows and bind translation.
struct Bind {
    int                   parent;
    Mat3                  rotation;
    std::array<double, 3> translation;
};

// A node as these grafts read one: a record from its name on, its index and size words trailing the node before it.
constexpr std::size_t kNodeAt = mds::kTable + mds::kName, kNodeSize = mds::kRecord - mds::kName;
constexpr std::size_t kNodeMesh = mds::kMesh - mds::kName, kNodeParent = mds::kParent - mds::kName;

// Hamilton product, w first as .mot rotations store it.
Vec4 QuatMul(const Vec4 &a, const Vec4 &b);

// The engine's quaternion-to-matrix inverted (Shepperd).
Vec4 MatToQuat(const Mat3 &r);

// A node's bind in an .mds.
Bind MdsBind(std::span<const unsigned char> mds, int node);

// Four-wide dot product; whether a key lies in [lo, hi]; name's index in names, or -1.
double Dot(const Vec4 &a, const Vec4 &b);
bool   InWindow(const MotKeyframe &key, int lo, int hi);
int    Index(const std::vector<std::string> &names, std::string_view name);

// The complement of the windows within [lo, hi].
std::vector<Span> FoldSpans(std::vector<Span> windows, int lo, int hi);

// Each span's first frame and, past a one-frame span, its last.
std::vector<int> SpanEdges(const std::vector<Span> &spans);

// The track's value at frame: lerp for translations, sign-corrected nlerp for rotations.
std::optional<Vec4> SampleTrack(const MotTrack &track, int frame);

// A pack's .mot record replaced and read; an .mds record's frame names; names joined for a message.
void                     ReplaceMot(ChrPack &pack, std::string_view mot_name, const MotFile &mot);
MotFile                  MotOf(const ChrPack &pack, std::string_view name);
std::vector<std::string> FramesOf(const ChrPack &pack, std::string_view mds);
std::string              Joined(const std::vector<std::string> &names);

// ------------------------------------------------------------------ grafts

// A body or shadow: its motion and model.
struct Model {
    std::string mot;
    std::string mds;
};

// Appends the graft's nodes, MDT chunks and .bbp rows to the base body, posed outside windows.
// The last node's two trailing words are the first MDT chunk's.
void GraftMeshNodes(ChrPack &base, const ChrPack &src, const TownMeshGraft &graft, const std::vector<Span> &windows,
                    int max_frame);

// The hub's spin: an ease-in cubic, the loop at rate, a cubic easing to rest on a whole turn.
void ApplySpin(ChrPack &base, const TownMeshGraft &graft);

// Grafts src's frames [slo, shi] into the body motion at [dlo, dhi], then the shadow's when both have one.
void GraftWindow(ChrPack &base, const CfgMotions &cfg, const ChrPack &src, const CfgMotions &scfg, int slo, int shi,
                 int dlo, int dhi, bool root_motion, const std::vector<Span> &rm_spans);

// Every track held in [dlo, dhi] at its value at frame, and the window played backwards.
void BakeHold(ChrPack &pack, std::string_view mot_name, int frame, int dlo, int dhi);
void ReverseWindow(ChrPack &pack, std::string_view mot_name, int dlo, int dhi);

// The root, the first translation track, moved by offset in [dlo, dhi].
void ApplyRootOffset(ChrPack &pack, std::string_view mot_name, int dlo, int dhi, const std::array<double, 3> &offset);

// Every translation keyed in [dlo, dhi] set there to its own value at frame.
void FreezeRoots(ChrPack &pack, std::string_view mot_name, int frame, int dlo, int dhi);

// The named nodes' tracks held in [dlo, dhi] at their values at frame.
void PinNodes(ChrPack &pack, const Model &model, const std::vector<std::string_view> &nodes, int frame, int dlo, int dhi);

// Copies the donor's shadow records in and declares them in the cfg.
void InjectShadow(ChrPack &base, const ChrPack &donor, const TownShadowInject &inject);

} // namespace expanded::town
