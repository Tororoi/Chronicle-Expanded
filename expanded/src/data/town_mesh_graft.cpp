#include <algorithm>
#include <bit>
#include <cmath>
#include <format>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <numbers>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "expanded/data/model_parts.hpp"
#include "expanded/data/town_rig.hpp"

namespace expanded::town {

namespace {

double RoundEven(double x) {
    double r = std::floor(x);
    double d = x - r;
    if (d > 0.5 || (d == 0.5 && std::fmod(r, 2.0) != 0)) {
        r += 1;
    }
    return r;
}

} // namespace

void GraftMeshNodes(ChrPack &base, const ChrPack &src, const TownMeshGraft &graft, const std::vector<Span> &windows,
                    int max_frame) {
    CfgMotions               scfg = CfgMotionsOf(src);
    CfgMotions               cfg = CfgMotionsOf(base);
    std::vector<std::string> names(graft.nodes.begin(), graft.nodes.end());
    Bytes                    bp = base.Require(cfg.body_mds).Payload();
    Bytes                    spl = src.Require(scfg.body_mds).Payload();
    std::vector<std::string> bnames = ReadMdsFrames(bp);
    std::vector<std::string> snames = ReadMdsFrames(spl);
    int                      nb = static_cast<int>(bnames.size());
    int                      k_count = static_cast<int>(names.size());
    std::vector<std::string> dup, missing;
    for (const std::string &name : names) {
        if (Index(bnames, name) >= 0) {
            dup.push_back(name);
        }
        if (Index(snames, name) < 0) {
            missing.push_back(name);
        }
    }
    if (!dup.empty()) {
        Bad("mesh graft: nodes already in the base rig: " + Joined(dup));
    }
    if (!missing.empty()) {
        Bad("mesh graft: nodes not in the source rig: " + Joined(missing));
    }
    if (cfg.shadow_mds) {
        std::vector<std::string> shadow = FramesOf(base, *cfg.shadow_mds), clash;
        for (const std::string &name : names) {
            if (Index(shadow, name) >= 0) {
                clash.push_back(name);
            }
        }
        if (!clash.empty()) {
            Bad("mesh graft: the base's shadow rig carries " + Joined(clash));
        }
    }
    std::size_t   mesh0 = kNodeAt + static_cast<std::size_t>(nb) * mds::kRecord - mds::kName;
    std::uint32_t first = std::numeric_limits<std::uint32_t>::max();
    for (int i = 0; i < nb; i++) {
        std::uint32_t off = Get32(bp, kNodeAt + i * mds::kRecord + kNodeMesh);
        if (off != 0) {
            first = std::min(first, off);
        }
    }
    auto is_mdt = [](std::span<const unsigned char> data, std::size_t at) {
        return at + 4 <= data.size() && data[at] == 'M' && data[at + 1] == 'D' && data[at + 2] == 'T' && data[at + 3] == 0;
    };
    if (first != mesh0 || !is_mdt(bp, mesh0)) {
        Bad("mesh graft: the base .mds's table and mesh do not overlap as expected");
    }
    std::uint32_t                        delta = static_cast<std::uint32_t>(k_count * mds::kRecord);
    std::unordered_map<std::string, int> new_index;
    for (int k = 0; k < k_count; k++) {
        new_index[names[k]] = nb + k;
    }
    // The table: the old last node completed, the count raised, the mesh offsets moved.
    Bytes table(bp.begin(), bp.begin() + mesh0);
    table.resize(mesh0 + mds::kName);
    Set32(table, mesh0, static_cast<std::uint32_t>(nb));
    Set32(table, mesh0 + 4, mds::kRecord);
    Set32(table, 0x08, static_cast<std::uint32_t>(nb + k_count));
    for (int i = 0; i < nb; i++) {
        std::size_t at = kNodeAt + i * mds::kRecord + kNodeMesh;
        if (std::uint32_t off = Get32(table, at); off != 0) {
            Set32(table, at, off + delta);
        }
    }
    // The grafted nodes, parents found by name, and their MDT chunks after the mesh block.
    Bytes       mesh_block(bp.begin() + mesh0, bp.end());
    std::size_t chunks_at = kNodeAt + static_cast<std::size_t>(nb + k_count) * mds::kRecord - mds::kName + mesh_block.size();
    Bytes       nodes, chunks;
    for (int k = 0; k < k_count; k++) {
        const std::string &name = names[k];
        int                si = Index(snames, name);
        if (kNodeAt + static_cast<std::size_t>(si) * mds::kRecord + kNodeSize > spl.size()) {
            Bad("mesh graft: node " + name + " lies past the source .mds");
        }
        Bytes                      node(spl.begin() + kNodeAt + si * mds::kRecord, spl.begin() + kNodeAt + si * mds::kRecord + kNodeSize);
        int                        sparent = static_cast<int>(Get32(node, kNodeParent));
        std::optional<std::string> pname;
        if (sparent >= 0) {
            if (sparent >= static_cast<int>(snames.size())) {
                Bad("mesh graft: node " + name + "'s parent is out of range");
            }
            pname = snames[sparent];
        }
        int parent;
        if (pname && Index(bnames, *pname) >= 0) {
            parent = Index(bnames, *pname);
        } else if (pname && new_index.contains(*pname)) {
            parent = new_index[*pname];
        } else {
            Bad("mesh graft: node " + name + "'s parent is in neither the base nor the graft");
        }
        if (parent >= nb + k) {
            Bad("mesh graft: node " + name + "'s parent comes after it");
        }
        Set32(node, kNodeParent, static_cast<std::uint32_t>(parent));
        if (std::uint32_t mesh = Get32(node, kNodeMesh); mesh != 0) {
            if (!is_mdt(spl, mesh)) {
                Bad("mesh graft: node " + name + "'s mesh is not an MDT chunk");
            }
            std::uint32_t size = Get32(spl, mesh + 8);
            if (mesh + std::size_t{size} > spl.size()) {
                Bad("mesh graft: node " + name + "'s mesh runs past the .mds");
            }
            Set32(node, kNodeMesh, static_cast<std::uint32_t>(chunks_at + chunks.size()));
            chunks.insert(chunks.end(), spl.begin() + mesh, spl.begin() + mesh + size);
            chunks.resize((chunks.size() + 15) & ~std::size_t{15});
        }
        nodes.insert(nodes.end(), node.begin(), node.end());
        nodes.resize(nodes.size() + mds::kName);
        Set32(nodes, nodes.size() - mds::kName, static_cast<std::uint32_t>(nb + k + 1));
        Set32(nodes, nodes.size() - 4, mds::kRecord);
    }
    nodes.resize(nodes.size() - mds::kName);
    Bytes mds = table;
    mds.insert(mds.end(), nodes.begin(), nodes.end());
    mds.insert(mds.end(), mesh_block.begin(), mesh_block.end());
    mds.insert(mds.end(), chunks.begin(), chunks.end());
    base.Require(cfg.body_mds).ReplacePayload(mds);
    // .bbp: the source's 64-byte bind rows of the grafted nodes appended.
    std::string bbp = CfgBbp(base.records[cfg.cfg]);
    std::string sbbp = CfgBbp(src.records[scfg.cfg]);
    Bytes       bb = base.Require(bbp).Payload();
    Bytes       sb = src.Require(sbbp).Payload();
    if (bb.size() != static_cast<std::size_t>(nb) * 64 || sb.size() != snames.size() * 64) {
        Bad("mesh graft: a .bbp is not 64 bytes a node");
    }
    for (const std::string &name : names) {
        std::size_t at = static_cast<std::size_t>(Index(snames, name)) * 64;
        bb.insert(bb.end(), sb.begin() + at, sb.begin() + at + 64);
    }
    base.Require(bbp).ReplacePayload(bb);
    // Body .mot: a track for each source channel of the graft, holding the pose outside windows.
    MotFile          sm = MotOf(src, scfg.body_mot);
    MotFile          dm = MotOf(base, cfg.body_mot);
    std::vector<int> edges = SpanEdges(FoldSpans(windows, 1, max_frame));
    const auto      &spin = graft.spin;
    if (spin && !new_index.contains(std::string(spin->node))) {
        Bad("mesh graft: the spin's node is not grafted");
    }
    std::map<std::string, std::vector<const MotTrack *>> by_node;
    for (const MotTrack &track : sm.tracks) {
        if (track.w0 < snames.size() && new_index.contains(snames[track.w0])) {
            by_node[snames[track.w0]].push_back(&track);
        }
    }
    for (const std::string &name : names) {
        const std::vector<const MotTrack *> &tracks = by_node[name];
        for (const MotTrack *track : tracks) {
            std::optional<Vec4> value = SampleTrack(*track, graft.pose_frame);
            if (!value) {
                continue;
            }
            MotTrack added = *track;
            added.w0 = static_cast<std::uint32_t>(new_index[name]);
            added.keys.clear();
            for (int frame : edges) {
                added.keys.push_back(MotKeyframe::Make(frame, *value));
            }
            dm.tracks.push_back(std::move(added));
        }
        if (spin && name == spin->node &&
            std::none_of(tracks.begin(), tracks.end(), [](const MotTrack *t) { return t->w2 == mot::kRotation; })) {
            // A hub without a source rotation: an anchor-only rotation track at its bind, for the spin.
            Vec4     bind = MatToQuat(MdsBind(spl, Index(snames, name)).rotation);
            MotTrack added{.w0 = static_cast<std::uint32_t>(new_index[name]), .w3 = 0x20};
            added.w6 = MotTrack::kTagW6;
            added.w7 = MotTrack::kTagW7;
            for (int frame : edges) {
                added.keys.push_back(MotKeyframe::Make(frame, bind));
            }
            dm.tracks.push_back(std::move(added));
        }
    }
    ReplaceMot(base, cfg.body_mot, dm);
}

void ApplySpin(ChrPack &base, const TownMeshGraft &graft) {
    const TownSpin &spin = *graft.spin;
    CfgMotions      cfg = CfgMotionsOf(base);
    Bytes           mds = base.Require(cfg.body_mds).Payload();
    int             w0 = Index(ReadMdsFrames(mds), spin.node);
    MotFile         dm = MotOf(base, cfg.body_mot);
    MotTrack       *track = dm.TrackBy(static_cast<std::uint32_t>(w0), 0);
    if (!track) {
        Bad("spin: the hub has no rotation track");
    }
    auto [loop_lo, loop_hi] = spin.loop;
    double rate = spin.rate;
    if (rate <= 0 || rate >= 90.0) {
        Bad("spin: the rate is outside (0, 90) degrees a frame");
    }
    double loop_deg = std::fmod(rate * (loop_hi - loop_lo), 360.0);
    if (std::abs(loop_deg) > 1e-6 && std::abs(loop_deg - 360.0) > 1e-6) {
        Bad("spin: the loop is not whole turns");
    }
    double h = spin.handoff_advance;
    if (h != 0 && !(0.0 < h && h < 90.0)) {
        Bad("spin: the hand-off advance is outside (0, 90)");
    }
    std::map<int, double> keys;
    {
        auto [lo, hi] = spin.ramp_up;
        int    n = hi - lo;
        double turns = std::max(1.0, RoundEven(rate * n * 2.0 / 3.0 / 360.0));
        double t = 360.0 * turns;
        double a = (rate * n - 2 * t) / (static_cast<double>(n) * n * n);
        double b = (3 * t - rate * n) / (static_cast<double>(n) * n);
        for (int frame = lo; frame <= hi; frame++) {
            int x = frame - lo;
            keys[frame] = a * (static_cast<double>(x) * x * x) + b * (static_cast<double>(x) * x);
        }
    }
    for (int frame = loop_lo; frame <= loop_hi; frame++) {
        keys[frame] = h + (frame - loop_lo) * rate;
    }
    {
        auto [lo, hi] = spin.ramp_down;
        int                 n = hi - lo;
        std::vector<double> turns;
        for (int k = 1; k <= 8; k++) {
            turns.push_back(k * 360.0);
        }
        double target = h + rate * n * 2.0 / 3.0;
        std::stable_sort(turns.begin(), turns.end(),
                         [&](double x, double y) { return std::abs(x - target) < std::abs(y - target); });
        std::function<double(double)> solution;
        for (double t : turns) {
            double              d = t - h - rate * n;
            double              c3 = (-rate * n - 2 * d) / (static_cast<double>(n) * n * n);
            double              c2 = (d - c3 * (static_cast<double>(n) * n * n)) / (static_cast<double>(n) * n);
            auto                th = [=](double x) { return c3 * x * x * x + c2 * x * x + rate * x + h; };
            auto                dth = [=](double x) { return 3 * c3 * x * x + 2 * c2 * x + rate; };
            std::vector<double> points{0.0, static_cast<double>(n)};
            if (c3 != 0) {
                double x = -c2 / (3 * c3);
                if (0 < x && x < n) {
                    points.push_back(x);
                }
            }
            bool   monotonic = std::all_of(points.begin(), points.end(), [&](double x) { return dth(x) > -1e-9; });
            double max_step = -std::numeric_limits<double>::infinity();
            for (int k = 0; k < n; k++) {
                max_step = std::max(max_step, th(k + 1) - th(k));
            }
            if (monotonic && max_step < 90.0) {
                solution = th;
                break;
            }
        }
        if (!solution) {
            Bad("spin: no whole-turn ramp down fits");
        }
        for (int frame = lo; frame <= hi; frame++) {
            keys[frame] = solution(frame - lo);
        }
    }
    for (const MotKeyframe &key : track->keys) {
        if (keys.contains(static_cast<int>(key.Frame()))) {
            Bad("spin: a spin window meets the hub's anchors");
        }
    }
    std::vector<std::pair<int, double>> ordered(keys.begin(), keys.end());
    for (std::size_t i = 1; i < ordered.size(); i++) {
        if (ordered[i].first == ordered[i - 1].first + 1 && std::abs(ordered[i].second - ordered[i - 1].second) >= 90.0) {
            Bad("spin: a step of 90 degrees or more");
        }
    }
    Vec4 bind = MatToQuat(MdsBind(mds, w0).rotation);
    for (auto [frame, deg] : ordered) {
        double th = deg * (std::numbers::pi / 180.0);
        double s = std::sin(th / 2);
        Vec4   q = QuatMul({std::cos(th / 2), spin.axis[0] * s, spin.axis[1] * s, spin.axis[2] * s}, bind);
        track->keys.push_back(MotKeyframe::Make(frame, q));
    }
    SortByFrame(track->keys);
    // One hemisphere throughout, for the engine's nlerp.
    std::optional<Vec4> prev;
    for (MotKeyframe &key : track->keys) {
        Vec4 v = Wide(key.Values());
        if (prev && Dot(*prev, v) < 0) {
            for (double &x : v) {
                x = -x;
            }
            key.SetValues(v);
            v = Wide(key.Values());
        }
        prev = v;
    }
    ReplaceMot(base, cfg.body_mot, dm);
}

} // namespace expanded::town
