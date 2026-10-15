#include "expanded/data/town_model.hpp"

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

// The mod's ISO bake's math, in doubles rounded to float on store, so the bytes match its.

namespace expanded {

namespace {

namespace fs = std::filesystem;

using Vec4 = std::array<double, 4>;
using Span = std::pair<int, int>;

[[noreturn]] void Bad(const std::string &what) {
    throw std::runtime_error(what);
}

char Fold(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

bool EqualsFolded(std::string_view a, std::string_view b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return Fold(x) == Fold(y); });
}

bool EndsFolded(std::string_view text, std::string_view tail) {
    return text.size() >= tail.size() && EqualsFolded(text.substr(text.size() - tail.size()), tail);
}

std::string Text(std::span<const unsigned char> bytes) {
    return std::string(bytes.begin(), bytes.end());
}

Bytes ToBytes(std::string_view text) {
    return Bytes(text.begin(), text.end());
}

std::size_t IndexOf(std::string_view text, std::string_view what, std::size_t from = 0) {
    std::size_t at = text.find(what, from);
    if (at == std::string_view::npos) {
        Bad(std::format("cfg lacks \"{}\"", what));
    }
    return at;
}

// ------------------------------------------------------------------ cfg text

// A step of the few cfg patterns: a literal, [ \t]+, [ \t]*, "([^"]+)" (kept or skipped).
struct Step {
    enum Kind {
        Literal,
        Blanks,
        MaybeBlanks,
        Quoted,
        SkipQuoted
    } kind;

    std::string_view literal = {};
};

bool Blank(char c) {
    return c == ' ' || c == '\t';
}

// The capture of the leftmost match of steps in text, as the bake's regexes find it.
std::optional<std::string> Search(std::string_view text, std::initializer_list<Step> steps) {
    for (std::size_t start = 0; start < text.size(); start++) {
        std::size_t                p = start;
        std::optional<std::string> capture;
        bool                       ok = true;
        for (const Step &step : steps) {
            switch (step.kind) {
                case Step::Literal:
                    ok = text.substr(p).starts_with(step.literal);
                    p += step.literal.size();
                    break;
                case Step::Blanks:
                case Step::MaybeBlanks: {
                    std::size_t from = p;
                    while (p < text.size() && Blank(text[p])) {
                        p++;
                    }
                    ok = step.kind == Step::MaybeBlanks || p > from;
                    break;
                }
                case Step::Quoted:
                case Step::SkipQuoted: {
                    if (p >= text.size() || text[p] != '"') {
                        ok = false;
                        break;
                    }
                    std::size_t close = text.find('"', p + 1);
                    if (close == std::string_view::npos || close == p + 1) {
                        ok = false;
                        break;
                    }
                    if (step.kind == Step::Quoted) {
                        capture = std::string(text.substr(p + 1, close - p - 1));
                    }
                    p = close + 1;
                    break;
                }
            }
            if (!ok) {
                break;
            }
        }
        if (ok) {
            return capture;
        }
    }
    return std::nullopt;
}

// The pack's main cfg and its model and motion names; prefer names it in a two-character pack.
struct CfgMotions {
    std::size_t                cfg = 0;
    std::string                body_mot;
    std::string                body_mds;
    std::optional<std::string> shadow_mot;
    std::optional<std::string> shadow_mds;
};

CfgMotions CfgMotionsOf(const ChrPack &pack, std::string_view prefer = {}) {
    std::vector<std::size_t> candidates;
    for (std::size_t i = 0; i < pack.records.size(); i++) {
        if (EndsFolded(pack.records[i].name, ".cfg")) {
            candidates.push_back(i);
        }
    }
    if (!prefer.empty()) {
        std::stable_partition(candidates.begin(), candidates.end(),
                              [&](std::size_t i) { return EqualsFolded(pack.records[i].name, prefer); });
    }
    if (candidates.empty()) {
        Bad("the pack has no cfg");
    }
    CfgMotions result{.cfg = candidates[0]};
    for (std::size_t i : candidates) {
        std::string text = Text(pack.records[i].Payload());
        if (text.contains("KEY_START") && text.contains("MOTION")) {
            result.cfg = i;
            break;
        }
    }
    std::string text = Text(pack.records[result.cfg].Payload());
    using enum Step::Kind;
    auto body_mot = Search(text, {
                                     {Literal, "MOTION"},
                                     {Blanks},
                                     {Literal, "0,"},
                                     {Blanks},
                                     {Quoted}
    });
    if (!body_mot) {
        body_mot = Search(text, {
                                    {Literal, "MOTION"},
                                    {Blanks},
                                    {Literal, "0,"},
                                    {MaybeBlanks},
                                    {Quoted}
        });
    }
    auto body_mds = Search(text, {
                                     {Literal, "MODEL"},
                                     {Blanks},
                                     {Quoted}
    });
    if (!body_mot || !body_mds) {
        Bad(std::format("{} names no body model and motion", pack.records[result.cfg].name));
    }
    result.body_mot = *body_mot;
    result.body_mds = *body_mds;
    result.shadow_mot = Search(text, {
                                         {Literal, "SHADOW_MOTION"},
                                         {Blanks},
                                         {Quoted}
    });
    result.shadow_mds = Search(text, {
                                         {Literal, "SHADOW_MODEL"},
                                         {Blanks},
                                         {Quoted}
    });
    return result;
}

// The body MOTION 0 line's .bbp, its second quoted name.
std::string CfgBbp(const ChrRecord &cfg) {
    using enum Step::Kind;
    auto bbp = Search(Text(cfg.Payload()), {
                                               {Literal, "MOTION"},
                                               {Blanks},
                                               {Literal, "0,"},
                                               {MaybeBlanks},
                                               {SkipQuoted},
                                               {Literal, ","},
                                               {MaybeBlanks},
                                               {Quoted}
    });
    if (!bbp) {
        Bad(std::format("{} names no body .bbp", cfg.name));
    }
    return *bbp;
}

// ------------------------------------------------------------------ math

// Hamilton product, w first as .mot rotations store it.
Vec4 QuatMul(const Vec4 &a, const Vec4 &b) {
    return {
        a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
        a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
        a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
        a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0],
    };
}

using Mat3 = std::array<std::array<double, 3>, 3>;

// The engine's quaternion-to-matrix inverted (Shepperd).
Vec4 MatToQuat(const Mat3 &r) {
    double m00 = r[0][0], m01 = r[0][1], m02 = r[0][2], m10 = r[1][0], m11 = r[1][1], m12 = r[1][2], m20 = r[2][0],
           m21 = r[2][1], m22 = r[2][2];
    double tr = m00 + m11 + m22, s;
    if (tr > 0) {
        s = std::sqrt(tr + 1.0) * 2;
        return {0.25 * s, (m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s};
    }
    if (m00 >= m11 && m00 >= m22) {
        s = std::sqrt(1.0 + m00 - m11 - m22) * 2;
        return {(m21 - m12) / s, 0.25 * s, (m01 + m10) / s, (m02 + m20) / s};
    }
    if (m11 >= m22) {
        s = std::sqrt(1.0 + m11 - m00 - m22) * 2;
        return {(m02 - m20) / s, (m01 + m10) / s, 0.25 * s, (m12 + m21) / s};
    }
    s = std::sqrt(1.0 + m22 - m00 - m11) * 2;
    return {(m10 - m01) / s, (m02 + m20) / s, (m12 + m21) / s, 0.25 * s};
}

// A .mds node's parent, bind rotation rows and bind translation.
struct Bind {
    int                   parent;
    Mat3                  rotation;
    std::array<double, 3> translation;
};

Bind MdsBind(std::span<const unsigned char> mds, int node) {
    std::size_t p = 0x18 + static_cast<std::size_t>(node) * 0x70;
    double      m[16];
    for (int k = 0; k < 16; k++) {
        m[k] = GetF32(mds, p + 0x28 + k * 4);
    }
    return {
        static_cast<int>(Get32(mds, p + 0x24)),
        {{{m[0], m[1], m[2]}, {m[4], m[5], m[6]}, {m[8], m[9], m[10]}}},
        {m[12], m[13], m[14]}
    };
}

Vec4 Values(const MotKeyframe &key) {
    return {key.Value(0), key.Value(1), key.Value(2), key.Value(3)};
}

void PackValue(MotKeyframe &key, const Vec4 &value) {
    for (int i = 0; i < 4; i++) {
        key.SetValue(i, static_cast<float>(value[i]));
    }
}

MotKeyframe NewKey(std::uint32_t frame, const Vec4 &value) {
    MotKeyframe key;
    key.SetFrame(frame);
    PackValue(key, value);
    return key;
}

double Dot(const Vec4 &a, const Vec4 &b) {
    double sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

void SortKeys(MotTrack &track) {
    std::stable_sort(track.keys.begin(), track.keys.end(),
                     [](const MotKeyframe &a, const MotKeyframe &b) { return a.Frame() < b.Frame(); });
}

bool InWindow(const MotKeyframe &key, int lo, int hi) {
    return lo <= static_cast<std::int64_t>(key.Frame()) && key.Frame() <= static_cast<std::int64_t>(hi);
}

int Index(const std::vector<std::string> &names, std::string_view name) {
    auto found = std::find(names.begin(), names.end(), name);
    return found == names.end() ? -1 : static_cast<int>(found - names.begin());
}

// The complement of the windows within [lo, hi].
std::vector<Span> FoldSpans(std::vector<Span> windows, int lo, int hi) {
    std::stable_sort(windows.begin(), windows.end());
    std::vector<Span> spans;
    int               cur = lo;
    for (auto [a, b] : windows) {
        if (cur < a) {
            spans.push_back({cur, a - 1});
        }
        cur = std::max(cur, b + 1);
    }
    if (cur <= hi) {
        spans.push_back({cur, hi});
    }
    return spans;
}

std::vector<int> SpanEdges(const std::vector<Span> &spans) {
    std::vector<int> edges;
    for (auto [lo, hi] : spans) {
        edges.push_back(lo);
        if (hi != lo) {
            edges.push_back(hi);
        }
    }
    return edges;
}

// The track's value at frame: lerp for translations, sign-corrected nlerp for rotations.
std::optional<Vec4> SampleTrack(const MotTrack &track, int frame) {
    const MotKeyframe *prev = nullptr;
    const MotKeyframe *next = nullptr;
    for (const MotKeyframe &key : track.keys) {
        std::int64_t at = key.Frame();
        if (at <= frame && (!prev || key.Frame() > prev->Frame())) {
            prev = &key;
        }
        if (at >= frame && (!next || key.Frame() < next->Frame())) {
            next = &key;
        }
    }
    if (!prev && !next) {
        return std::nullopt;
    }
    if (!prev) {
        return Values(*next);
    }
    if (!next || next->Frame() == prev->Frame()) {
        return Values(*prev);
    }
    double u = static_cast<double>(frame - static_cast<std::int64_t>(prev->Frame())) /
               static_cast<double>(next->Frame() - prev->Frame());
    Vec4   a = Values(*prev);
    Vec4   b = Values(*next);
    Vec4   v;
    if (track.w2 == 0) {
        if (Dot(a, b) < 0) {
            for (double &x : b) {
                x = -x;
            }
        }
        for (int i = 0; i < 4; i++) {
            v[i] = a[i] + (b[i] - a[i]) * u;
        }
        double n = std::sqrt(Dot(v, v));
        if (n == 0) {
            n = 1.0;
        }
        for (double &x : v) {
            x /= n;
        }
        return v;
    }
    for (int i = 0; i < 4; i++) {
        v[i] = a[i] + (b[i] - a[i]) * u;
    }
    return v;
}

void ReplaceMot(ChrPack &pack, std::string_view mot_name, const MotFile &mot) {
    pack.Require(mot_name).ReplacePayload(mot.BuildPayload());
}

MotFile MotOf(const ChrPack &pack, std::string_view name) {
    return MotFile::FromRecord(pack.Require(name));
}

std::vector<std::string> FramesOf(const ChrPack &pack, std::string_view mds) {
    return ReadMdsFrames(pack.Require(mds).Payload());
}

std::string Joined(const std::vector<std::string> &names) {
    std::string out;
    for (const std::string &name : names) {
        out += (out.empty() ? "" : ", ") + name;
    }
    return out;
}

// ------------------------------------------------------------------ mesh-node graft and spin

// Appends the graft's nodes, MDT chunks and .bbp rows to the base body, posed outside windows.
// A .mds node is 0x70 bytes; the last node's two trailing words are the first MDT chunk's.
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
    std::size_t   mesh0 = 0x18 + static_cast<std::size_t>(nb) * 0x70 - 8;
    std::uint32_t first = std::numeric_limits<std::uint32_t>::max();
    for (int i = 0; i < nb; i++) {
        std::uint32_t off = Get32(bp, 0x18 + i * 0x70 + 0x20);
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
    std::uint32_t                        delta = static_cast<std::uint32_t>(k_count) * 0x70;
    std::unordered_map<std::string, int> new_index;
    for (int k = 0; k < k_count; k++) {
        new_index[names[k]] = nb + k;
    }
    // The table: the old last node completed, the count raised, the mesh offsets moved.
    Bytes table(bp.begin(), bp.begin() + mesh0);
    table.resize(mesh0 + 8);
    Set32(table, mesh0, static_cast<std::uint32_t>(nb));
    Set32(table, mesh0 + 4, 0x70);
    Set32(table, 0x08, static_cast<std::uint32_t>(nb + k_count));
    for (int i = 0; i < nb; i++) {
        std::size_t at = 0x18 + i * 0x70 + 0x20;
        if (std::uint32_t off = Get32(table, at); off != 0) {
            Set32(table, at, off + delta);
        }
    }
    // The grafted nodes, parents found by name, and their MDT chunks after the mesh block.
    Bytes       mesh_block(bp.begin() + mesh0, bp.end());
    std::size_t chunks_at = 0x18 + static_cast<std::size_t>(nb + k_count) * 0x70 - 8 + mesh_block.size();
    Bytes       nodes, chunks;
    for (int k = 0; k < k_count; k++) {
        const std::string &name = names[k];
        int                si = Index(snames, name);
        if (0x18 + static_cast<std::size_t>(si) * 0x70 + 0x68 > spl.size()) {
            Bad("mesh graft: node " + name + " lies past the source .mds");
        }
        Bytes                      node(spl.begin() + 0x18 + si * 0x70, spl.begin() + 0x18 + si * 0x70 + 0x68);
        int                        sparent = static_cast<int>(Get32(node, 0x24));
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
        Set32(node, 0x24, static_cast<std::uint32_t>(parent));
        if (std::uint32_t mesh = Get32(node, 0x20); mesh != 0) {
            if (!is_mdt(spl, mesh)) {
                Bad("mesh graft: node " + name + "'s mesh is not an MDT chunk");
            }
            std::uint32_t size = Get32(spl, mesh + 8);
            if (mesh + std::size_t{size} > spl.size()) {
                Bad("mesh graft: node " + name + "'s mesh runs past the .mds");
            }
            Set32(node, 0x20, static_cast<std::uint32_t>(chunks_at + chunks.size()));
            chunks.insert(chunks.end(), spl.begin() + mesh, spl.begin() + mesh + size);
            chunks.resize((chunks.size() + 15) & ~std::size_t{15});
        }
        nodes.insert(nodes.end(), node.begin(), node.end());
        nodes.resize(nodes.size() + 8);
        Set32(nodes, nodes.size() - 8, static_cast<std::uint32_t>(nb + k + 1));
        Set32(nodes, nodes.size() - 4, 0x70);
    }
    nodes.resize(nodes.size() - 8);
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
                added.keys.push_back(NewKey(frame, *value));
            }
            dm.tracks.push_back(std::move(added));
        }
        if (spin && name == spin->node &&
            std::none_of(tracks.begin(), tracks.end(), [](const MotTrack *t) { return t->w2 == 0; })) {
            // A hub without a source rotation: an anchor-only rotation track at its bind, for the spin.
            Vec4     bind = MatToQuat(MdsBind(spl, Index(snames, name)).rotation);
            MotTrack added{.w0 = static_cast<std::uint32_t>(new_index[name]), .w3 = 0x20};
            added.w6 = MotTrack::kTagW6;
            added.w7 = MotTrack::kTagW7;
            for (int frame : edges) {
                added.keys.push_back(NewKey(frame, bind));
            }
            dm.tracks.push_back(std::move(added));
        }
    }
    ReplaceMot(base, cfg.body_mot, dm);
}

double RoundEven(double x) {
    double r = std::floor(x);
    double d = x - r;
    if (d > 0.5 || (d == 0.5 && std::fmod(r, 2.0) != 0)) {
        r += 1;
    }
    return r;
}

// The hub's spin: an ease-in cubic, the loop at rate, a cubic easing to rest on a whole turn.
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
        track->keys.push_back(NewKey(frame, q));
    }
    SortKeys(*track);
    // One hemisphere throughout, for the engine's nlerp.
    std::optional<Vec4> prev;
    for (MotKeyframe &key : track->keys) {
        Vec4 v = Values(key);
        if (prev && Dot(*prev, v) < 0) {
            for (double &x : v) {
                x = -x;
            }
            PackValue(key, v);
            v = Values(key);
        }
        prev = v;
    }
    ReplaceMot(base, cfg.body_mot, dm);
}

// ------------------------------------------------------------------ grafts

// A source rig whose root alone is named differently takes the dest's root name.
std::vector<std::string> AliasRoot(std::vector<std::string> s, const std::vector<std::string> &d) {
    if (!s.empty() && !d.empty() && s[0] != d[0] && Index(d, s[0]) < 0 && Index(s, d[0]) < 0) {
        s[0] = d[0];
    }
    return s;
}

// Makes the dest tracks a grafted window lacks, held at the dest's bind outside the grafts.
void CreateMissingTracks(MotFile &dst, const MotFile &src, const std::vector<std::string> &sframes,
                         const std::vector<std::string> &dframes, int slo, int shi, std::span<const unsigned char> dpl,
                         const std::vector<Span> &spans) {
    std::vector<int>                                  remap = BuildJointRemap(sframes, dframes);
    std::set<std::pair<std::uint32_t, std::uint32_t>> have;
    for (const MotTrack &track : dst.tracks) {
        have.insert({track.w0, track.w2});
    }
    std::vector<int> edges = SpanEdges(spans);
    for (const MotTrack &st : src.tracks) {
        if (std::none_of(st.keys.begin(), st.keys.end(), [&](const MotKeyframe &k) { return InWindow(k, slo, shi); })) {
            continue;
        }
        int dw = st.w0 < remap.size() ? remap[st.w0] : -1;
        if (dw < 0 || have.contains({static_cast<std::uint32_t>(dw), st.w2})) {
            continue;
        }
        Bind     b = MdsBind(dpl, dw);
        Vec4     v = st.w2 == 0 ? MatToQuat(b.rotation) : Vec4{b.translation[0], b.translation[1], b.translation[2], 0.0};
        MotTrack added = st;
        added.w0 = static_cast<std::uint32_t>(dw);
        added.keys.clear();
        for (int frame : edges) {
            added.keys.push_back(NewKey(frame, v));
        }
        auto at = std::find_if(dst.tracks.begin(), dst.tracks.end(), [&](const MotTrack &t) {
            return t.w0 > static_cast<std::uint32_t>(dw) || (t.w0 == static_cast<std::uint32_t>(dw) && t.w2 > st.w2);
        });
        dst.tracks.insert(at, std::move(added));
        have.insert({static_cast<std::uint32_t>(dw), st.w2});
    }
}

struct Model {
    std::string mot;
    std::string mds;
};

void Graft(ChrPack &dst_pack, const Model &dst_model, const ChrPack &src_pack, const Model &src_model, int slo, int shi,
           int dlo, int dhi, bool root_motion, const std::vector<Span> &spans) {
    MotFile                  dst = MotOf(dst_pack, dst_model.mot);
    MotFile                  src = MotOf(src_pack, src_model.mot);
    Bytes                    dpl = dst_pack.Require(dst_model.mds).Payload();
    std::vector<std::string> dframes = ReadMdsFrames(dpl);
    std::vector<std::string> sframes = FramesOf(src_pack, src_model.mds);
    if (root_motion) {
        sframes = AliasRoot(sframes, dframes);
        CreateMissingTracks(dst, src, sframes, dframes, slo, shi, dpl, spans);
    }
    SpliceByJoint(dst, src, sframes, dframes, slo, shi, dlo, dhi);
    ReplaceMot(dst_pack, dst_model.mot, dst);
}

// Keys every grafted track at the window's edges, so a sparse one stays out of the next clip.
void SealGraft(ChrPack &dst_pack, const Model &dst_model, const ChrPack &src_pack, const Model &src_model, int slo,
               int shi, int dlo, int dhi, bool root_motion) {
    std::vector<std::string> dframes = FramesOf(dst_pack, dst_model.mds);
    std::vector<std::string> sframes = FramesOf(src_pack, src_model.mds);
    if (root_motion) {
        sframes = AliasRoot(sframes, dframes);
    }
    std::vector<int> remap(sframes.size());
    for (std::size_t i = 0; i < sframes.size(); i++) {
        remap[i] = Index(dframes, sframes[i]);
    }
    MotFile                                                       dm = MotOf(dst_pack, dst_model.mot);
    MotFile                                                       sm = MotOf(src_pack, src_model.mot);
    std::map<std::pair<std::uint32_t, std::uint32_t>, MotTrack *> dtracks;
    for (MotTrack &track : dm.tracks) {
        dtracks[{track.w0, track.w2}] = &track;
    }
    bool changed = false;
    for (const MotTrack &st : sm.tracks) {
        if (st.w0 >= remap.size() || remap[st.w0] < 0) {
            continue;
        }
        auto found = dtracks.find({static_cast<std::uint32_t>(remap[st.w0]), st.w2});
        if (found == dtracks.end() || found->second->keys.empty()) {
            continue;
        }
        MotTrack &dt = *found->second;
        for (auto [sf, df] : {
                 Span{slo, dlo},
                 Span{shi, dhi}
        }) {
            auto at_edge = [&](const MotKeyframe &k) { return k.Frame() == static_cast<std::uint32_t>(df); };
            if (std::any_of(dt.keys.begin(), dt.keys.end(), at_edge)) {
                continue;
            }
            std::optional<Vec4> v = SampleTrack(st, sf);
            if (!v) {
                continue;
            }
            MotKeyframe key = dt.keys[0];
            key.SetFrame(df);
            PackValue(key, *v);
            dt.keys.push_back(key);
            SortKeys(dt);
            changed = true;
        }
    }
    if (changed) {
        ReplaceMot(dst_pack, dst_model.mot, dm);
    }
}

// ------------------------------------------------------------------ window post-ops

// Every track's keys in [dlo, dhi] dropped and its value at frame keyed at both edges.
void HoldTracks(MotFile &mot, int frame, int dlo, int dhi, const std::function<bool(const MotTrack &)> &which,
                const char *what) {
    for (MotTrack &track : mot.tracks) {
        if (!which(track)) {
            continue;
        }
        std::optional<Vec4> v = SampleTrack(track, frame);
        if (!v || track.keys.empty()) {
            continue;
        }
        std::erase_if(track.keys, [&](const MotKeyframe &k) { return InWindow(k, dlo, dhi); });
        if (track.keys.empty()) {
            Bad(std::format("{}: a track was keyed only inside the window", what));
        }
        for (int edge : {dlo, dhi}) {
            MotKeyframe key = track.keys[0];
            key.SetFrame(edge);
            PackValue(key, *v);
            track.keys.push_back(key);
        }
        SortKeys(track);
    }
}

void BakeHold(ChrPack &pack, std::string_view mot_name, int frame, int dlo, int dhi) {
    MotFile mot = MotOf(pack, mot_name);
    HoldTracks(mot, frame, dlo, dhi, [](const MotTrack &) { return true; }, "hold");
    ReplaceMot(pack, mot_name, mot);
}

void ReverseWindow(ChrPack &pack, std::string_view mot_name, int dlo, int dhi) {
    MotFile mot = MotOf(pack, mot_name);
    for (MotTrack &track : mot.tracks) {
        bool any = false;
        for (MotKeyframe &key : track.keys) {
            if (InWindow(key, dlo, dhi)) {
                key.SetFrame(static_cast<std::uint32_t>(dlo + dhi - static_cast<int>(key.Frame())));
                any = true;
            }
        }
        if (any) {
            SortKeys(track);
        }
    }
    ReplaceMot(pack, mot_name, mot);
}

// The root, the first translation track, moved by offset in [dlo, dhi].
void ApplyRootOffset(ChrPack &pack, std::string_view mot_name, int dlo, int dhi, const std::array<double, 3> &offset) {
    MotFile mot = MotOf(pack, mot_name);
    auto    root = std::find_if(mot.tracks.begin(), mot.tracks.end(), [](const MotTrack &t) { return t.w2 == 2; });
    if (root == mot.tracks.end()) {
        return;
    }
    for (MotKeyframe &key : root->keys) {
        if (InWindow(key, dlo, dhi)) {
            Vec4 v = Values(key);
            for (int i = 0; i < 3; i++) {
                v[i] += offset[i];
            }
            PackValue(key, v);
        }
    }
    ReplaceMot(pack, mot_name, mot);
}

// Every translation keyed in [dlo, dhi] set there to its own value at frame.
void FreezeRoots(ChrPack &pack, std::string_view mot_name, int frame, int dlo, int dhi) {
    MotFile mot = MotOf(pack, mot_name);
    for (MotTrack &track : mot.tracks) {
        if (track.w2 != 2 ||
            std::none_of(track.keys.begin(), track.keys.end(), [&](const MotKeyframe &k) {
                return InWindow(k, dlo, dhi);
            })) {
            continue;
        }
        std::optional<Vec4> v = SampleTrack(track, frame);
        if (!v) {
            continue;
        }
        for (MotKeyframe &key : track.keys) {
            if (InWindow(key, dlo, dhi)) {
                PackValue(key, *v);
            }
        }
    }
    ReplaceMot(pack, mot_name, mot);
}

void PinNodes(ChrPack &pack, const Model &model, const std::vector<std::string_view> &nodes, int frame, int dlo, int dhi) {
    MotFile                  mot = MotOf(pack, model.mot);
    std::vector<std::string> frames = FramesOf(pack, model.mds);
    HoldTracks(
        mot, frame, dlo, dhi,
        [&](const MotTrack &t) {
            return t.w0 < frames.size() && std::find(nodes.begin(), nodes.end(), frames[t.w0]) != nodes.end();
        },
        "pin");
    ReplaceMot(pack, model.mot, mot);
}

// Copies the donor's shadow records in and declares them in the cfg.
void InjectShadow(ChrPack &base, const ChrPack &donor, const TownShadowInject &inject) {
    for (std::string_view name : {inject.model, inject.bbp, inject.wgt, inject.mot}) {
        const ChrRecord *record = donor.Find(name);
        if (!record) {
            Bad(std::format("shadow inject: the donor lacks {}", name));
        }
        if (base.Find(record->name)) {
            Bad(std::format("shadow inject: the base has {} already", record->name));
        }
        base.records.push_back(*record);
    }
    ChrRecord  &cfg = base.records[CfgMotionsOf(base).cfg];
    std::string text = Text(cfg.Payload());
    std::size_t va = IndexOf(text, "\r\n", IndexOf(text, "VERTEX_ANIME")) + 2;
    text.insert(va, "SHADOW_VERTEX_ANIME 1\r\n");
    std::size_t eol = IndexOf(text, "\r\n", IndexOf(text, "MODEL ")) + 2;
    text.insert(eol, std::format("SHADOW_MODEL \"{}\"\r\n", inject.model));
    if (std::size_t at = text.find("SHADOW_MOTION"); at != std::string::npos) {
        std::size_t end = text.find_first_of("\r\n", at);
        end = end == std::string::npos ? text.size() : end;
        text.replace(at, end - at, std::format("SHADOW_MOTION \"{}\", \"{}\", \"{}\"", inject.mot, inject.bbp, inject.wgt));
    }
    cfg.ReplacePayload(ToBytes(text));
}

} // namespace

Bytes RewriteTownKeys(std::span<const unsigned char> cfg, const std::vector<TownSlot> &slots) {
    std::vector<const TownSlot *> ordered;
    for (const TownSlot &slot : slots) {
        ordered.push_back(&slot);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const TownSlot *a, const TownSlot *b) { return a->idx < b->idx; });
    std::string keys;
    for (const TownSlot *slot : ordered) {
        keys += std::format("KEY\t{},\t{},\t{:.2f}, //{}\r\n", slot->lo, slot->hi, slot->speed, slot->name.substr(0, 12));
    }
    std::string text = Text(cfg);
    std::size_t line = IndexOf(text, "\r\n", IndexOf(text, "KEY_START")) + 2;
    std::size_t end = IndexOf(text, "MOTION_END", line);
    return ToBytes(text.substr(0, line) + keys + text.substr(end));
}

Bytes BuildTownModel(const TownAlly &ally, const DiscReader &read) {
    ChrPack base = ChrPack::Parse(read(ally.base));
    if (ally.shadow_inject) {
        InjectShadow(base, ChrPack::Parse(read(ally.shadow_inject->src)), *ally.shadow_inject);
    }
    CfgMotions                          cfg = CfgMotionsOf(base);
    std::map<std::string_view, ChrPack> cache;

    auto source = [&](std::string_view path) -> const ChrPack & {
        auto found = cache.find(path);
        if (found == cache.end()) {
            found = cache.emplace(path, ChrPack::Parse(read(path))).first;
        }
        return found->second;
    };
    int max_frame = 0;
    for (const TownSlot &slot : ally.slots) {
        max_frame = std::max(max_frame, slot.hi);
    }
    const auto &graft = ally.mesh_graft;
    if (graft) {
        std::vector<Span> windows;
        for (const TownSlot &slot : ally.slots) {
            if (slot.src == graft->src) {
                windows.push_back({slot.lo, slot.hi});
            }
        }
        GraftMeshNodes(base, source(graft->src), *graft, windows, max_frame);
    }
    bool              rm = ally.root_motion;
    std::vector<Span> rm_spans;
    if (rm) {
        std::vector<Span> windows;
        for (const TownSlot &slot : ally.slots) {
            if (!slot.src.empty() || slot.hold) {
                windows.push_back({slot.lo, slot.hi});
            }
        }
        rm_spans = FoldSpans(windows, 1, max_frame);
    }
    Model body{cfg.body_mot, cfg.body_mds};
    for (const TownSlot &slot : ally.slots) {
        int dlo = slot.lo, dhi = slot.hi;
        if (!slot.src.empty()) {
            const ChrPack &sp = source(slot.src);
            CfgMotions     scfg = CfgMotionsOf(sp, slot.src_cfg);
            if (graft && slot.src != graft->src) {
                std::vector<std::string> frames = FramesOf(sp, scfg.body_mds);
                for (std::string_view node : graft->nodes) {
                    if (Index(frames, node) >= 0) {
                        Bad(std::format("slot {}: its source carries the grafted node {}", slot.name, node));
                    }
                }
            }
            auto [wlo, whi] = slot.win;
            Model sbody{scfg.body_mot, scfg.body_mds};
            Graft(base, body, sp, sbody, wlo, whi, dlo, dhi, rm, rm_spans);
            SealGraft(base, body, sp, sbody, wlo, whi, dlo, dhi, rm);
            if (cfg.shadow_mot && scfg.shadow_mot) {
                Model shadow{*cfg.shadow_mot, cfg.shadow_mds.value_or("")};
                Model sshadow{*scfg.shadow_mot, scfg.shadow_mds.value_or("")};
                Graft(base, shadow, sp, sshadow, wlo, whi, dlo, dhi, rm, rm_spans);
                SealGraft(base, shadow, sp, sshadow, wlo, whi, dlo, dhi, rm);
            }
        }
        // Each post-op runs on the body, then the shadow; pins on the body alone.
        auto both = [&](const std::function<void(std::string_view)> &op) {
            op(cfg.body_mot);
            if (cfg.shadow_mot) {
                op(*cfg.shadow_mot);
            }
        };
        if (slot.hold) {
            both([&](std::string_view mot) { BakeHold(base, mot, *slot.hold, dlo, dhi); });
        }
        if (slot.root_offset) {
            both([&](std::string_view mot) { ApplyRootOffset(base, mot, dlo, dhi, *slot.root_offset); });
        }
        if (slot.reverse) {
            both([&](std::string_view mot) { ReverseWindow(base, mot, dlo, dhi); });
        }
        if (slot.root_freeze) {
            both([&](std::string_view mot) { FreezeRoots(base, mot, *slot.root_freeze, dlo, dhi); });
        }
        if (!slot.pin_nodes.empty()) {
            PinNodes(base, body, slot.pin_nodes, slot.pin_frame, dlo, dhi);
        }
    }
    if (graft && graft->spin) {
        ApplySpin(base, *graft);
    }
    ChrRecord &cfg_record = base.records[cfg.cfg];
    cfg_record.ReplacePayload(RewriteTownKeys(cfg_record.Payload(), ally.slots));
    return base.Rebuild();
}

int BuildTownModels(const std::filesystem::path &data, std::FILE *log) {
    auto say = [&](const std::string &line) {
        if (log) {
            std::fputs(line.c_str(), log);
        }
    };
    auto read = [&](std::string_view path) {
        fs::path      file = data / fs::path(path);
        std::ifstream stream(file, std::ios::binary);
        if (!stream) {
            Bad(std::format("cannot read {}", file.string()));
        }
        return Bytes(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    int failures = 0;
    for (const TownAlly &ally : TownAllies()) {
        std::error_code error;
        if (!fs::is_regular_file(data / fs::path(ally.base), error)) {
            say(std::format("town model {}: skipped, the data has no {}\n", ally.name, ally.base));
            continue;
        }
        try {
            Bytes    model = BuildTownModel(ally, read);
            fs::path target = data / fs::path(ally.file);
            bool     current = fs::is_regular_file(target, error) && fs::file_size(target, error) == model.size() &&
                               read(ally.file) == model;
            if (!current) {
                fs::create_directories(target.parent_path());
                std::ofstream stream(target, std::ios::binary | std::ios::trunc);
                stream.write(reinterpret_cast<const char *>(model.data()), static_cast<std::streamsize>(model.size()));
                stream.close();
                if (!stream) {
                    Bad(std::format("cannot write {}", target.string()));
                }
            }
            say(std::format("town model {}: {} ({} bytes{})\n", ally.name, ally.file, model.size(),
                            current ? ", unchanged" : ""));
        } catch (const std::exception &failure) {
            say(std::format("town model {}: not built: {}\n", ally.name, failure.what()));
            failures++;
        }
    }
    return failures;
}

} // namespace expanded
