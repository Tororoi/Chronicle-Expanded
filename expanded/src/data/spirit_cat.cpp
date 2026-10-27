#include "expanded/data/spirit_cat.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/sheet_look.hpp"
#include "expanded/data/spirit_cat_parts.hpp"

namespace expanded {

namespace {

using namespace rig;
using namespace spirit_cat;

constexpr double             kHideScale = 0.001; // the cape frame's bind 3x3: the cloth built from it starts collapsed to a point
constexpr const char        *kRootName = "catroot";
constexpr const char        *kNodePrefix = "cat_";
constexpr const char        *kBoneNames[] = {"cat_rwing1", "cat_rwing2", "cat_rwing3", "cat_rwing4", "cat_lwing1", "cat_lwing2", "cat_lwing3", "cat_lwing4"};
constexpr const char        *kMeshNames[] = {"cat_rwingm", "cat_lwingm"};
constexpr const char        *kMaskNode = "cat_mask";
constexpr const char        *kWingTexture = "catwing";
constexpr const char        *kCapeNode = "cat_cape";
constexpr const char        *kCapeTexture = "catcape"; // the mask's too
constexpr const char        *kCapeClo = "catcape.clo";
constexpr const char        *kGlowName = "catglowc";
constexpr std::size_t        kSubmeshRecords = 150;
constexpr unsigned char      kWingRgba[] = {255, 255, 255, 0x80};
constexpr unsigned char      kCapeRgba[] = {128, 28, 0, 0x80};
constexpr std::array<int, 3> kGlowCore = {50, 50, 50}, kGlowOuter = {160, 160, 160}; // the element glow at rest: none
constexpr int                kFloatKeyIndex = 7;
constexpr double             kFloatRate = 0.75;

std::string Repr(double x) {
    char buffer[64];
    auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), x);
    std::string text(buffer, end);
    if (text.find_first_of(".eE") == std::string::npos) {
        text += ".0";
    }
    return text;
}

std::string Fixed6(double v) {
    return std::format("{:.6f}", v);
}

// The rig's nodes named as the cat's runtime finds them: the root kRootName, the rest kNodePrefix + their own.
Bytes RenameRig(std::span<const unsigned char> mds) {
    Bytes                    out(mds.begin(), mds.end());
    std::vector<std::string> names = ReadMdsFrames(mds);
    for (std::size_t k = 0; k < names.size(); k++) {
        std::size_t p = mds::kTable + k * mds::kRecord + mds::kName;
        std::string name = k == 0 ? kRootName : kNodePrefix + names[k];
        if (name.size() > mds::kNameSize - 1) {
            throw std::runtime_error("cat node name too long: " + name);
        }
        std::fill_n(out.begin() + std::ptrdiff_t(p), mds::kNameSize, 0);
        std::copy(name.begin(), name.end(), out.begin() + std::ptrdiff_t(p));
    }
    return out;
}

struct ExtraNode {
    std::string            name;
    int                    parent;
    std::array<double, 16> local;
    std::optional<Bytes>   mdt;
};

// The nodes appended to an .mds: records after the last, every mesh offset moved past them, their MDTs after the meshes.
Bytes GraftExtraNodes(std::span<const unsigned char> payload, const std::vector<ExtraNode> &extra) {
    if (Get32(payload, mds::kTable + mds::kSize) != mds::kRecord) {
        throw std::runtime_error("mds stride");
    }
    std::vector<std::pair<Bytes, std::optional<Bytes>>> frames;
    for (const ExtraNode &node : extra) {
        if (node.name.size() > mds::kNameSize - 1) {
            throw std::runtime_error("node name too long: " + node.name);
        }
        Bytes record(mds::kRecord, 0);
        std::copy(node.name.begin(), node.name.end(), record.begin() + mds::kName);
        Set32(record, mds::kParent, static_cast<std::uint32_t>(node.parent));
        for (std::size_t i = 0; i < 16; i++) {
            SetF32(record, mds::kRotation + i * 4, static_cast<float>(node.local[i]));
        }
        frames.emplace_back(std::move(record), node.mdt);
    }
    return AppendMdsFrames(payload, frames);
}

// Only the keys inside the clip windows (and the one either side of each) kept.
void TrimTracks(MotFile &mot) {
    for (MotTrack &t : mot.tracks) {
        std::vector<MotKeyframe> keys = t.keys;
        SortByFrame(keys);
        std::set<std::size_t> keep;
        for (const CatKeyWindow &w : kCatKeys) {
            std::uint32_t lo = static_cast<std::uint32_t>(w.start), hi = static_cast<std::uint32_t>(w.end);
            long          before = -1, after = -1;
            for (std::size_t i = 0; i < keys.size(); i++) {
                if (keys[i].Frame() < lo) {
                    before = long(i);
                }
                if (after < 0 && keys[i].Frame() > hi) {
                    after = long(i);
                }
            }
            if (before >= 0) {
                keep.insert(std::size_t(before));
            }
            if (after >= 0) {
                keep.insert(std::size_t(after));
            }
            for (std::size_t i = 0; i < keys.size(); i++) {
                if (lo <= keys[i].Frame() && keys[i].Frame() <= hi) {
                    keep.insert(i);
                }
            }
        }
        std::vector<MotKeyframe> kept;
        for (std::size_t i : keep) {
            kept.push_back(keys[i]);
        }
        if (kept.empty() && !keys.empty()) {
            kept.push_back(keys[0]);
        }
        t.keys = kept;
    }
}

MotKeyframe Key(double frame, std::initializer_list<double> vals) {
    MotKeyframe key;
    key.SetFrame(static_cast<std::uint32_t>(static_cast<int>(frame)));
    std::size_t i = 0;
    for (double v : vals) {
        key.SetValue(int(i++), static_cast<float>(v));
    }
    for (; i < 4; i++) {
        key.SetValue(int(i), 0.0f);
    }
    return key;
}

MotKeyframe Key(double frame, const V4 &vals) {
    return Key(frame, {vals[0], vals[1], vals[2], vals[3]});
}

MotTrack Trk(std::uint32_t w0, std::uint32_t w1, std::uint32_t w2, std::uint32_t w6, std::uint32_t w7, std::vector<MotKeyframe> keys) {
    MotTrack t;
    t.w0 = w0;
    t.w1 = w1;
    t.w2 = w2;
    t.w3 = 32;
    t.w6 = w6;
    t.w7 = w7;
    t.keys = std::move(keys);
    return t;
}

std::array<double, 16> Local16(const M3 &r, const V3 &t) {
    M4                     m = MatFromRT(r, t);
    std::array<double, 16> out;
    for (std::size_t i = 0; i < 16; i++) {
        out[i] = m[i / 4][i % 4];
    }
    return out;
}

// An indexed triangle-list MDT on the skin MDT's header: positions, one normal and texture entry a vertex, one material
// naming the texture.
Bytes ListMdt(const MdtMesh &skin_mdt, const Bytes &mat_template, std::string_view texture, const std::vector<V3> &pos,
              const std::vector<V4> &normal_block, const std::vector<V4> &uv_block, const std::vector<Tri> &tris) {
    MdtMesh m;
    m.header = skin_mdt.header;
    m.colours = false;
    for (const V3 &p : pos) {
        m.positions.push_back({p[0], p[1], p[2], 1.0});
    }
    m.normals = normal_block;
    m.uvs = uv_block;
    std::vector<std::vector<int>> recs;
    for (const Tri &t : tris) {
        for (int v : t) {
            recs.push_back({v, v, v});
        }
    }
    for (std::size_t k = 0; k < recs.size(); k += kSubmeshRecords) {
        MdtMesh::Submesh sub;
        sub.prim = 3;
        sub.material = 0;
        sub.records.assign(recs.begin() + std::ptrdiff_t(k), recs.begin() + std::ptrdiff_t(std::min(recs.size(), k + kSubmeshRecords)));
        m.submeshes.push_back(std::move(sub));
    }
    Bytes mat = mat_template;
    std::fill_n(mat.begin() + MdtMesh::kMaterialTexture, 16, 0);
    std::copy(texture.begin(), texture.end(), mat.begin() + MdtMesh::kMaterialTexture);
    m.materials = {mat};
    m.preamble = {0, 16, static_cast<int>(m.submeshes.size()), 0};
    m.order = {"POS", "DL", "UV", "NORM", "MAT"};
    m.pads = {
        {"POS",  {}},
        {"UV",   {}},
        {"NORM", {}},
        {"MAT",  {}}
    };
    m.AlignDisplayList();
    std::uint32_t nv = static_cast<std::uint32_t>(pos.size());
    m.header[5] = nv;
    m.header[7] = 0;
    m.header[8] = 0xFFFFFFFF;
    m.header[11] = nv;
    m.header[13] = 1;
    m.header[15] = 0;
    Bytes blob = m.Build();
    if (blob.size() % 16 != 0) {
        throw std::runtime_error("an MDT that is not whole qwords");
    }
    return blob;
}

struct Encoded {
    std::vector<ExtraNode> nodes;
    std::vector<MotTrack>  wgt_tracks, mot_tracks;
    Bytes                  bbp;
    ExtraNode              cape;
    std::string            clo;
};

// The wing graft baked into pack data: 8 wing bones as a chain under cat_sebone2, the two wing meshes and the mask under
// the root with their MDTs, .wgt runs and the wings' .mot tracks, the cape's cloth frame and its .clo.
Encoded Encode(std::span<const unsigned char> dran, const Bytes &graft_mds, const ChrPack &graft_pack, const ChrPack &cat, const Log &log) {
    WingData          data = WingGraft(dran, graft_mds, graft_pack, log);
    const auto       &nodes = data.nodes;
    int               base = data.base, parent = data.parent, k_count = base;
    const M4         &psp = nodes[std::size_t(parent)].world;
    std::map<int, M4> open;
    for (int side0 : {base, base + 4}) {
        std::vector<V3> ts = {data.bind_open[side0].t};
        for (int k = 0; k < 3; k++) {
            double    seg = nodes[std::size_t(side0 + k + 1)].t[0];
            const M3 &r = data.bind_open[side0 + k].r;
            ts.push_back({ts[std::size_t(k)][0] + seg * r[0][0], ts[std::size_t(k)][1] + seg * r[0][1], ts[std::size_t(k)][2] + seg * r[0][2]});
        }
        for (int k = 0; k < 4; k++) {
            open[side0 + k] = MatMulExact(MatFromRT(data.bind_open[side0 + k].r, ts[std::size_t(k)]), psp);
        }
    }
    auto w_open = [&](int i) -> const M4 & {
        auto found = open.find(i);
        return found != open.end() ? found->second : nodes[std::size_t(i)].world;
    };
    Bytes                cmds = cat.Require("c04cat.mds").Payload();
    std::vector<RigNode> cnodes = ReadSkeleton(cmds);
    const RigNode       &skin_n = *std::find_if(cnodes.begin(), cnodes.end(), [](const RigNode &n) { return n.name == "skin"; });
    MdtMesh              skin_mdt = MdtMesh::Parse(std::span<const unsigned char>(cmds).subspan(std::size_t(skin_n.mesh_off)));
    MotFile              wgt0 = MotFile::FromRecord(cat.Require("c04cat.wgt"));
    std::uint32_t        w6 = wgt0.tracks[0].w6, w7 = wgt0.tracks[0].w7;

    Encoded out;
    for (int k = 0; k < 8; k++) {
        const RigNode &n = nodes[std::size_t(base + k)];
        if (!(0 <= n.parent && n.parent < base + k)) {
            throw std::runtime_error(std::format("{}: parent {}", kBoneNames[k], n.parent));
        }
        M4 l = MatMulExact(w_open(base + k), RigidInv(w_open(n.parent)));
        M3 r = RotOf(l);
        V3 t = TransOf(l);
        if (n.parent != parent && !(std::fabs(t[1]) < 1e-3 && std::fabs(t[2]) < 1e-3 && std::fabs(t[0] - n.t[0]) < 1e-3)) {
            throw std::runtime_error(std::format("{}: chained local", kBoneNames[k]));
        }
        out.nodes.push_back({kBoneNames[k], n.parent, Local16(r, t), std::nullopt});
    }
    const std::array<double, 16> ident = Local16(kIdentity, {0.0, 0.0, 0.0});
    int                          mesh_index[2];
    for (int mi = 0; mi < 2; mi++) {
        mesh_index[mi] = k_count + static_cast<int>(out.nodes.size());
        out.nodes.push_back({kMeshNames[mi], 0, ident, std::nullopt});
    }
    std::size_t mask_slot = out.nodes.size();
    int         mask_index = k_count + static_cast<int>(mask_slot);
    out.nodes.push_back({kMaskNode, 0, ident, std::nullopt});

    // Dran's normal and texture entries a vertex, keyed by its obj1 position (the first record using it).
    const MdtMesh                   &om = data.om;
    std::map<int, std::pair<V4, V4>> dran_entries;
    for (const auto &sub : om.submeshes) {
        for (const auto &r : sub.records) {
            if (!dran_entries.contains(r[0])) {
                dran_entries[r[0]] = {
                    om.normals[std::size_t(r[1])], !om.uvs.empty() ? om.uvs[std::size_t(r[2])] : V4{0.0, 0.0, 1.0, 0.0}
                };
            }
        }
    }
    const Bytes &mat_template = skin_mdt.materials.at(0);
    for (int mi = 0; mi < 2; mi++) {
        std::string     sd = mi == 0 ? "r" : "l";
        const SkinMesh &me = data.meshes[std::size_t(mi)];
        const auto     &used = data.wing_sets[sd].used;
        auto            w_ref = [&](int b) -> const M4            &{
            auto found = data.wref.find(b);
            return found != data.wref.end() ? found->second : data.wcat[std::size_t(b)];
        };
        std::vector<V3> p;
        int             two_pose = 0;
        for (std::size_t i = 0; i < std::size_t(me.nv); i++) {
            int    b0 = me.b0[i], b1 = me.b1[i];
            double w = me.w0[i];
            V3     pa = XformPt(w_open(b0), me.p0[i]), pb = XformPt(w_open(b1), me.p1[i]);
            if (w >= 0.999 || Dist(pa, pb) < 1e-3) {
                p.push_back({pa[0] * w + pb[0] * (1 - w), pa[1] * w + pb[1] * (1 - w), pa[2] * w + pb[2] * (1 - w)});
            } else {
                two_pose++;
                V3 fa = XformPt(w_ref(b0), me.p0[i]), fb = XformPt(w_ref(b1), me.p1[i]);
                V3 target = {fa[0] * w + fb[0] * (1 - w), fa[1] * w + fb[1] * (1 - w), fa[2] * w + fb[2] * (1 - w)};
                M4 a0 = MatMulExact(RigidInv(w_open(b0)), w_ref(b0)), a1 = MatMulExact(RigidInv(w_open(b1)), w_ref(b1)), mb{};
                for (std::size_t r = 0; r < 4; r++) {
                    for (std::size_t c = 0; c < 4; c++) {
                        mb[r][c] = w * a0[r][c] + (1 - w) * a1[r][c];
                    }
                }
                auto inv = Inv4(mb);
                if (!inv) {
                    throw std::runtime_error(std::format("wing vertex {}: singular blend", i));
                }
                p.push_back(XformPt(*inv, target));
            }
        }
        std::vector<V4> normal_block, uv_block;
        for (std::size_t i = 0; i < std::size_t(me.nv); i++) {
            int key;
            if (i < used.size()) {
                key = used[i];
            } else {
                std::size_t j = 0;
                double      best = INFINITY;
                for (std::size_t jj = 0; jj < used.size(); jj++) {
                    double d = Dist(p[i], p[jj]);
                    if (d < best) {
                        best = d;
                        j = jj;
                    }
                }
                key = used[j];
            }
            auto found = dran_entries.find(key);
            if (found != dran_entries.end()) {
                normal_block.push_back(found->second.first);
                uv_block.push_back(found->second.second);
            } else {
                normal_block.push_back({0.5, 0.5, 1.0, 1.0});
                uv_block.push_back({0.0, 0.0, 1.0, 0.0});
            }
        }
        Bytes blob = ListMdt(skin_mdt, mat_template, kWingTexture, p, normal_block, uv_block, me.tris);
        out.nodes[8 + std::size_t(mi)].mdt = blob;
        // The .wgt run: the reset entry, then the bones ascending, each vertex's percents summing to 100.
        std::map<int, std::vector<std::pair<int, int>>> per_bone;
        for (int i = 0; i < me.nv; i++) {
            int    b0 = me.b0[std::size_t(i)], b1 = me.b1[std::size_t(i)];
            double w = me.w0[std::size_t(i)];
            int    pa = static_cast<int>(Round(w * 100, 0));
            if (b0 == b1 || pa >= 100) {
                pa = 100;
            }
            if (pa <= 0) {
                b0 = b1;
                pa = 100;
            }
            per_bone[b0].emplace_back(i, pa);
            if (pa < 100) {
                per_bone[b1].emplace_back(i, 100 - pa);
            }
        }
        std::uint32_t m = static_cast<std::uint32_t>(mesh_index[mi]);
        out.wgt_tracks.push_back(Trk(m, 0, mot::kWeight, w6, w7, {}));
        int keys = 0;
        for (auto &[b, list] : per_bone) {
            std::stable_sort(list.begin(), list.end());
            std::vector<MotKeyframe> kfs;
            for (auto [i, pct] : list) {
                kfs.push_back(Key(i, {double(pct)}));
            }
            keys += static_cast<int>(kfs.size());
            out.wgt_tracks.push_back(Trk(m, static_cast<std::uint32_t>(b), mot::kWeight, w6, w7, std::move(kfs)));
        }
        log(std::format("  {} wing baked: {} verts, {} tris, MDT {} B, {} wgt keys, {} two-pose verts", sd, me.nv, me.tris.size(), blob.size(), keys, two_pose));
    }
    for (const Track &t : data.tracks) {
        if (t.node < base) {
            continue;
        }
        if (!(t.node < base + 8 && (t.chan == mot::kRotation || t.chan == mot::kTranslation))) {
            throw std::runtime_error(std::format("wing track node {}", t.node));
        }
        std::vector<MotKeyframe> kfs;
        for (std::size_t i = 0; i < t.frames.size(); i++) {
            kfs.push_back(Key(t.frames[i], t.vals[i]));
        }
        out.mot_tracks.push_back(Trk(static_cast<std::uint32_t>(t.node), 0, static_cast<std::uint32_t>(t.chan), MotTrack::kTagW6, MotTrack::kTagW7, std::move(kfs)));
    }

    // The Super Steve cape: a cloth frame under the root (its bind 3x3 at kHideScale, so the cloth built from it starts
    // collapsed) whose MDT is the rest lattice in the anchor's space, in the engine's width-outer order.
    const auto     &cat_nodes = data.cat_nodes;
    const RigNode  &skin_node = *std::find_if(cat_nodes.begin(), cat_nodes.end(), [](const RigNode &n) { return n.name == "cat_skin"; });
    auto            skin_w = LoadWeights(data.pack, "cat.wgt").at(skin_node.i);
    SkinMesh        skin_me = *BuildMeshWeighted(data.mds, skin_node, cat_nodes, skin_w);
    std::vector<V3> cverts0 = CapeRestLocal(cat_nodes, skin_me);
    const int       rows = kCapeRows, cols = kCapeCols;
    std::vector<V3> cverts;
    for (int c = 0; c < cols; c++) {
        for (int r = 0; r < rows; r++) {
            cverts.push_back(cverts0[std::size_t(r * cols + c)]);
        }
    }
    std::vector<Tri> ctris;
    for (int c = 0; c < cols - 1; c++) {
        for (int r = 0; r < rows - 1; r++) {
            int a = c * rows + r;
            ctris.push_back({a, a + rows, a + rows + 1});
            ctris.push_back({a, a + rows + 1, a + 1});
        }
    }
    std::vector<V4> cape_normals(cverts.size(), V4{0.5, 0.5, 1.0, 1.0}), cape_uvs(cverts.size(), V4{0.5, 0.5, 1.0, 0.0});
    Bytes           cape_mdt = ListMdt(skin_mdt, mat_template, kCapeTexture, cverts, cape_normals, cape_uvs, ctris);
    out.cape = {
        kCapeNode, 0, {kHideScale, 0, 0, 0, 0, kHideScale, 0, 0, 0, 0, kHideScale, 0, 0, 0, 0, 1},
          cape_mdt
    };
    const CapePhysics &ph = kCapePhysics;
    std::string        clo = std::format("SIZE\t{},\t{}\r\nFRAME\t\"{}\"\r\nWINDEFFECT\t{}\r\nNORMAL\t{}\r\n", cols, rows, kCapeNode, Fixed6(ph.wind), Fixed6(ph.normal));
    clo += std::format("GRAVITY\t{0},\t{0},\t{0}\r\nFOLLOW\t{1},\t{2},\t{3}\r\n", Fixed6(ph.gravity), Fixed6(ph.follow0), Fixed6(ph.follow1), Fixed6(ph.follow2));
    clo += std::format("K\t{0},\t{0},\t{0}\r\nPOLYDIV\t\"{1}\"\r\n", Fixed6(ph.k), std::string(std::size_t(cols - 1), '0'));
    std::vector<BoundLocal> bounds = CapeBoundsLocal(cat_nodes);
    for (std::size_t k = 0; k < bounds.size(); k++) {
        const BoundLocal &b = bounds[k];
        // Each bound names its own bone (the mod's .clo names the cape node, then points each at its bone in this order).
        clo += std::format("BOUND\t\"{}\"\r\n\t{},\t{},\t{}\r\n\t{},\t{},\t{}\r\n\t{},\t{},\t{}\r\n\t{},\t{},\t{}\r\n\t{}\r\n", kCapeBounds[k].bone,
                           Fixed6(b.up[0]), Fixed6(b.up[1]), Fixed6(b.up[2]), Fixed6(b.a[0]), Fixed6(b.a[1]), Fixed6(b.a[2]), Fixed6(b.b[0]),
                           Fixed6(b.b[1]), Fixed6(b.b[2]), Fixed6(b.radii[0]), Fixed6(b.radii[1]), Fixed6(b.radii[2]), Fixed6(b.damp));
    }
    out.clo = clo;
    log(std::format("  cape baked: {}x{} lattice, MDT {} B", rows, cols, cape_mdt.size()));

    // The mask: a ring around each eye with a hole in it, bound whole to the head.
    SkinMesh        mask = BuildMaskMesh(cat_nodes, skin_me, log);
    const RigNode  &head = *std::find_if(cat_nodes.begin(), cat_nodes.end(), [](const RigNode &n) { return n.name == kMaskAnchor; });
    std::vector<V3> mworld;
    for (const V3 &v : mask.p0) {
        mworld.push_back(XformPt(head.world, v));
    }
    std::vector<V4> mask_normals(mworld.size(), V4{0.5, 0.5, 1.0, 1.0}), mask_uvs(mworld.size(), V4{0.5, 0.5, 1.0, 0.0});
    out.nodes[mask_slot].mdt = ListMdt(skin_mdt, mat_template, kCapeTexture, mworld, mask_normals, mask_uvs, mask.tris);
    out.wgt_tracks.push_back(Trk(static_cast<std::uint32_t>(mask_index), 0, mot::kWeight, w6, w7, {}));
    std::vector<MotKeyframe> mask_keys;
    for (std::size_t v = 0; v < mworld.size(); v++) {
        mask_keys.push_back(Key(double(v), {100.0}));
    }
    out.wgt_tracks.push_back(Trk(static_cast<std::uint32_t>(mask_index), static_cast<std::uint32_t>(head.i), mot::kWeight, w6, w7, std::move(mask_keys)));
    log(std::format("  mask baked: {} verts, {} tris on {}", mworld.size(), mask.tris.size(), kMaskAnchor));
    out.bbp.resize(out.nodes.size() * bbp::kEntry);
    for (std::size_t i = 0; i < out.nodes.size(); i++) {
        for (std::size_t k = 0; k < 16; k++) {
            SetF32(out.bbp, i * bbp::kEntry + k * 4, static_cast<float>(out.nodes[i].local[k]));
        }
    }
    return out;
}

Bytes FlatPicture(std::span<const unsigned char> templ, const unsigned char (&rgba)[4]) {
    Bytes clut(256 * 4);
    for (std::size_t i = 0; i < 256; i++) {
        std::copy(rgba, rgba + 4, clut.begin() + std::ptrdiff_t(i * 4));
    }
    return tim8::Build(templ, 32, 32, Bytes(32 * 32, 0), clut);
}

std::string Cfg() {
    std::vector<std::string> lines = {"//Divine Beast Title cat (CatPackBakes)",
                                      "IMG 0,\"dbcat.img\"",
                                      "IMG_END",
                                      "MATERIAL_ANIME 0",
                                      "VERTEX_ANIME 1",
                                      "SHADOW_VERTEX_ANIME 0",
                                      "",
                                      std::format("ALLOC_MDT \"{}\"", kCapeNode),
                                      "ALLOC_DBUFF \"cat_skin\"",
                                      std::format("ALLOC_DBUFF \"{}\"", kMeshNames[0]),
                                      std::format("ALLOC_DBUFF \"{}\"", kMeshNames[1]),
                                      std::format("ALLOC_DBUFF \"{}\"", kMaskNode),
                                      "MODEL \"dbcat.mds\"",
                                      std::format("CLOTH \"{}\"", kCapeClo),
                                      "BODY_SIZE 3,7,60",
                                      "",
                                      "MOTION 0, \"dbcat.mot\", \"dbcat.bbp\", \"dbcat.wgt\"",
                                      std::format("KEY_START {}", kKeyStart)};
    for (int i = 0; i < static_cast<int>(std::size(kCatKeys)); i++) {
        const CatKeyWindow &k = kCatKeys[i];
        double              rate = i == kFloatKeyIndex ? kFloatRate : k.speed;
        lines.push_back(std::format("\tKEY\t{},\t{},\t{},\t\t//{} {}", k.start, k.end, Repr(rate), kKeyStart + i, k.comment));
    }
    lines.push_back("MOTION_END");
    lines.push_back("//catpack v20 standalone");
    lines.push_back("");
    std::string text;
    for (std::size_t i = 0; i < lines.size(); i++) {
        text += lines[i];
        if (i + 1 < lines.size()) {
            text += "\r\n";
        }
    }
    return text;
}

} // namespace

Bytes BuildSpiritCatModel(std::span<const unsigned char> cat_bytes, std::span<const unsigned char> town, std::span<const unsigned char> dran,
                          std::span<const unsigned char> glow_pack, const std::function<void(const std::string &)> &log_in) {
    Log     log = log_in ? log_in : Log([](const std::string &) {});
    ChrPack cat = ChrPack::Parse(cat_bytes);
    ChrPack flt = ChrPack::Parse(town);
    Bytes   spl = cat.Require("c04cat.mds").Payload();
    Bytes   cb = cat.Require("c04cat.bbp").Payload();
    ImgBank cimg = ImgBank::Parse(cat.Require("c04cat.img").Payload());

    // The clips: trimmed to the key windows, the float-up grafted by joint name into frames 285-294.
    MotFile mot = MotFile::FromRecord(cat.Require("c04cat.mot"));
    TrimTracks(mot);
    SpliceByJoint(mot, MotFile::FromRecord(flt.Require("e04c04cat.mot")), ReadMdsFrames(flt.Require("e04c04cat.mds").Payload()), ReadMdsFrames(spl),
                  kSpiritCatFloatFirst, kSpiritCatFloatLast, kSpiritCatFloatFrame, kSpiritCatFloatFrame + (kSpiritCatFloatLast - kSpiritCatFloatFirst));
    bool grafted = false;
    for (const MotTrack &t : mot.tracks) {
        for (const MotKeyframe &key : t.keys) {
            grafted = grafted || (key.Frame() >= std::uint32_t(kSpiritCatFloatFrame) &&
                                  key.Frame() <= std::uint32_t(kSpiritCatFloatFrame + kSpiritCatFloatLast - kSpiritCatFloatFirst));
        }
    }
    if (!grafted) {
        throw std::runtime_error("the float-up grafted no keys");
    }
    MotFile wgt = MotFile::FromRecord(cat.Require("c04cat.wgt"));

    // The wings are worked out on the cat as the mod's graft leaves it: renamed, its root's bind 3x3 once scaled by
    // kHideScale in floats and back.
    Bytes graft_mds = RenameRig(spl);
    for (std::size_t r = 0; r < 3; r++) {
        for (std::size_t c = 0; c < 3; c++) {
            std::size_t at = mds::kTable + mds::kRotation + r * 16 + c * 4;
            float       hidden = static_cast<float>(double(GetF32(graft_mds, at)) * kHideScale);
            SetF32(graft_mds, at, static_cast<float>(double(hidden) / kHideScale));
        }
    }
    ChrPack graft_pack;
    graft_pack.records.push_back(ChrRecord::Make("cat.mot", mot.BuildPayload()));
    graft_pack.records.push_back(ChrRecord::Make("cat.wgt", wgt.BuildPayload()));
    Encoded wd = Encode(dran, graft_mds, graft_pack, cat, log);

    // The cat's own pack.
    Bytes mds = GraftExtraNodes(RenameRig(spl), wd.nodes);
    std::vector<std::string> named = ReadMdsFrames(mds);
    auto                     anchor = std::find(named.begin(), named.end(), kCapeAnchor);
    if (anchor == named.end()) {
        throw std::runtime_error("no cape anchor in the cat rig");
    }
    std::array<double, 16> ident = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    mds = GraftExtraNodes(mds, {
                                   {wd.cape.name, static_cast<int>(anchor - named.begin()), ident, wd.cape.mdt}
    });
    Bytes bbp = cb;
    bbp.insert(bbp.end(), wd.bbp.begin(), wd.bbp.end());
    for (std::size_t i = 0; i < 16; i++) {
        Bytes row(4);
        SetF32(row, 0, static_cast<float>(ident[i]));
        bbp.insert(bbp.end(), row.begin(), row.end());
    }
    MotFile mot2 = mot, wgt2 = wgt;
    mot2.tracks.insert(mot2.tracks.end(), wd.mot_tracks.begin(), wd.mot_tracks.end());
    wgt2.tracks.insert(wgt2.tracks.end(), wd.wgt_tracks.begin(), wd.wgt_tracks.end());
    const Bytes &sheet = cimg.Block("c04cat01");
    ImgBank      img = cimg;
    img.entries.emplace_back(kWingTexture, FlatPicture(sheet, kWingRgba));
    img.entries.emplace_back(kCapeTexture, FlatPicture(sheet, kCapeRgba));
    Bytes torch = ImgBank::Parse(ChrPack::Parse(glow_pack).Require(kGlowSourceBank).Payload()).Block(kGlowSourcePicture);
    img.entries.emplace_back(kGlowName, BuildGlowDisc(sheet, torch, kGlowCore, kGlowOuter));

    return ChrPack::Build({ChrRecord::Make(kSpiritCatCfg, ToBytes(Cfg())), ChrRecord::Make("dbcat.mds", mds), ChrRecord::Make("dbcat.img", img.Build()),
                           ChrRecord::Make("dbcat.mot", mot2.BuildPayload()), ChrRecord::Make("dbcat.bbp", bbp),
                           ChrRecord::Make("dbcat.wgt", wgt2.BuildPayload()), ChrRecord::Make(kCapeClo, ToBytes(wd.clo))});
}

std::vector<DataBuild> SpiritCatModelBuilds() {
    return {DataBuild::FromSources("Divine Beast Title", kSpiritCatModelFile, {kSpiritCatSource, kSpiritCatFloatSource, kSpiritCatWingSource, kGlowSourcePack},
                                   [](const std::vector<Bytes> &s) { return BuildSpiritCatModel(s[0], s[1], s[2], s[3]); })};
}

} // namespace expanded
