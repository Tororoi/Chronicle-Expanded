#include "expanded/data/cat_rig.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <regex>
#include <stdexcept>

namespace expanded::rig {

namespace {

bool InCircum(const std::vector<V2> &p, const Tri &t, const V2 &q) {
    const V2 &a = p[std::size_t(t[0])], &b = p[std::size_t(t[1])], &c = p[std::size_t(t[2])];
    double    ax = a[0] - q[0], ay = a[1] - q[1], bx = b[0] - q[0], by = b[1] - q[1], cx = c[0] - q[0], cy = c[1] - q[1];
    return ((ax * ax + ay * ay) * (bx * cy - by * cx) - (bx * bx + by * by) * (ax * cy - ay * cx) + (cx * cx + cy * cy) * (ax * by - ay * bx)) > 0;
}

double Area2(const V2 &a, const V2 &b, const V2 &c) {
    return (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1]);
}

double MeshDim(std::span<const unsigned char> mds, const RigNode &node) {
    try {
        MdtMesh m = MdtMesh::Parse(mds.subspan(std::size_t(node.mesh_off)));
        double  lo[3], hi[3];
        for (int k = 0; k < 3; k++) {
            lo[k] = std::numeric_limits<double>::infinity();
            hi[k] = -std::numeric_limits<double>::infinity();
        }
        for (const auto &v : m.positions) {
            for (int k = 0; k < 3; k++) {
                lo[k] = std::min(lo[k], v[std::size_t(k)]);
                hi[k] = std::max(hi[k], v[std::size_t(k)]);
            }
        }
        return std::max(std::max(hi[0] - lo[0], hi[1] - lo[1]), hi[2] - lo[2]);
    } catch (const std::exception &) {
        return 0.0;
    }
}

} // namespace

double KeyedVal(std::span<const std::pair<double, double>> keys, double f) {
    double v = f <= keys[0].first ? keys[0].second : keys.back().second;
    for (std::size_t i = 0; i + 1 < keys.size(); i++) {
        if (keys[i].first <= f && f <= keys[i + 1].first) {
            v = keys[i].second + (keys[i + 1].second - keys[i].second) * Smooth((f - keys[i].first) / (keys[i + 1].first - keys[i].first));
        }
    }
    return v;
}

V4 Sample(const Track &track, double f) {
    const auto &frames = track.frames;
    const auto &vals = track.vals;
    if (f <= frames[0]) {
        return vals[0];
    }
    if (f >= frames.back()) {
        return vals.back();
    }
    std::size_t hi = 0;
    while (!(frames[hi] > f)) {
        hi++;
    }
    std::size_t lo = hi - 1;
    double      u = (f - frames[lo]) / (frames[hi] - frames[lo]);
    if (track.chan == mot::kRotation) {
        return SlerpExact(vals[lo], vals[hi], u);
    }
    V4 r;
    for (std::size_t i = 0; i < 4; i++) {
        r[i] = vals[lo][i] + (vals[hi][i] - vals[lo][i]) * u;
    }
    return r;
}

std::optional<std::vector<double>> Solve(std::vector<std::vector<double>> a, const std::vector<double> &b) {
    std::size_t                      n = b.size();
    std::vector<std::vector<double>> m(n, std::vector<double>(n + 1));
    for (std::size_t i = 0; i < n; i++) {
        for (std::size_t j = 0; j < n; j++) {
            m[i][j] = a[i][j];
        }
        m[i][n] = b[i];
    }
    for (std::size_t c = 0; c < n; c++) {
        std::size_t p = c;
        for (std::size_t r = c; r < n; r++) {
            if (std::fabs(m[r][c]) > std::fabs(m[p][c])) {
                p = r;
            }
        }
        if (std::fabs(m[p][c]) < 1e-15) {
            return std::nullopt;
        }
        std::swap(m[c], m[p]);
        for (std::size_t r = 0; r < n; r++) {
            if (r == c) {
                continue;
            }
            double f = m[r][c] / m[c][c];
            if (f != 0) {
                std::vector<double> row(n + 1);
                for (std::size_t k = 0; k <= n; k++) {
                    row[k] = m[r][k] - f * m[c][k];
                }
                m[r] = row;
            }
        }
    }
    std::vector<double> x(n);
    for (std::size_t i = 0; i < n; i++) {
        x[i] = m[i][n] / m[i][i];
    }
    return x;
}

std::optional<M4> Inv4(const M4 &m) {
    constexpr std::size_t                n = 4;
    std::array<std::array<double, 8>, 4> a{};
    for (std::size_t i = 0; i < n; i++) {
        for (std::size_t j = 0; j < n; j++) {
            a[i][j] = m[i][j];
        }
        a[i][n + i] = 1.0;
    }
    for (std::size_t c = 0; c < n; c++) {
        std::size_t p = c;
        for (std::size_t r = c; r < n; r++) {
            if (std::fabs(a[r][c]) > std::fabs(a[p][c])) {
                p = r;
            }
        }
        if (std::fabs(a[p][c]) < 1e-15) {
            return std::nullopt;
        }
        std::swap(a[c], a[p]);
        double f = a[c][c];
        for (double &v : a[c]) {
            v = v / f;
        }
        for (std::size_t r = 0; r < n; r++) {
            if (r != c && a[r][c] != 0) {
                double                g = a[r][c];
                std::array<double, 8> row;
                for (std::size_t k = 0; k < 2 * n; k++) {
                    row[k] = a[r][k] - g * a[c][k];
                }
                a[r] = row;
            }
        }
    }
    M4 out{};
    for (std::size_t i = 0; i < n; i++) {
        for (std::size_t j = 0; j < n; j++) {
            out[i][j] = a[i][n + j];
        }
    }
    return out;
}

ColumnPose Col(const M4 &m) {
    ColumnPose c{};
    for (std::size_t i = 0; i < 3; i++) {
        for (std::size_t j = 0; j < 3; j++) {
            c.r[i][j] = m[j][i];
        }
    }
    c.t = {m[3][0], m[3][1], m[3][2]};
    return c;
}

V3 Blend(double w, const ColumnPose &m0, const ColumnPose &m1, const V3 &p0, const V3 &p1) {
    V3 r;
    for (std::size_t i = 0; i < 3; i++) {
        double a = Sum({m0.r[i][0] * p0[0], m0.r[i][1] * p0[1], m0.r[i][2] * p0[2]});
        double b = Sum({m1.r[i][0] * p1[0], m1.r[i][1] * p1[1], m1.r[i][2] * p1[2]});
        r[i] = w * (a + m0.t[i]) + (1 - w) * (b + m1.t[i]);
    }
    return r;
}

std::optional<std::pair<V3, V3>> TwoPoseFit(double w, const std::pair<ColumnPose, ColumnPose> &ma, const std::pair<ColumnPose, ColumnPose> &mb,
                                            const V3 &target_a, const V3 &target_b, double guard) {
    std::vector<std::array<double, 6>> a;
    std::vector<double>                rhs;
    for (const auto &[pose, target] : {
             std::pair{&ma, &target_a},
             std::pair{&mb, &target_b}
    }) {
        const ColumnPose &b0 = pose->first, &b1 = pose->second;
        for (std::size_t i = 0; i < 3; i++) {
            std::array<double, 6> row{};
            for (std::size_t j = 0; j < 3; j++) {
                row[j] = w * b0.r[i][j];
                row[3 + j] = (1 - w) * b1.r[i][j];
            }
            a.push_back(row);
            rhs.push_back((*target)[i] - w * b0.t[i] - (1 - w) * b1.t[i]);
        }
    }
    std::vector<std::vector<double>> n(6, std::vector<double>(6));
    std::vector<double>              g(6);
    for (std::size_t i = 0; i < 6; i++) {
        for (std::size_t j = 0; j < 6; j++) {
            std::vector<double> terms;
            for (std::size_t k = 0; k < 6; k++) {
                terms.push_back(a[k][i] * a[k][j]);
            }
            n[i][j] = Sum(terms);
        }
        std::vector<double> terms;
        for (std::size_t k = 0; k < 6; k++) {
            terms.push_back(a[k][i] * rhs[k]);
        }
        g[i] = Sum(terms);
    }
    double mx = -std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < 6; i++) {
        mx = std::max(mx, n[i][i]);
    }
    double lam = 1e-8 * (mx + 1e-12);
    for (std::size_t i = 0; i < 6; i++) {
        n[i][i] += lam;
    }
    auto x = Solve(n, g);
    if (!x) {
        return std::nullopt;
    }
    double biggest = 0;
    for (double v : *x) {
        if (!std::isfinite(v)) {
            return std::nullopt;
        }
        biggest = std::max(biggest, std::fabs(v));
    }
    if (biggest >= guard) {
        return std::nullopt;
    }
    return std::pair{
        V3{(*x)[0], (*x)[1], (*x)[2]},
        V3{(*x)[3], (*x)[4], (*x)[5]}
    };
}

V3 PcaNormal(const std::vector<V3> &points, const V3 &c) {
    M3 s{};
    for (std::size_t i = 0; i < 3; i++) {
        for (std::size_t j = 0; j < 3; j++) {
            std::vector<double> terms;
            for (const V3 &p : points) {
                terms.push_back((p[i] - c[i]) * (p[j] - c[j]));
            }
            s[i][j] = Sum(terms);
        }
    }
    M3 vm = {
        {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}
    };
    for (int it = 0; it < 60; it++) {
        std::vector<double> offs;
        for (std::size_t i = 0; i < 3; i++) {
            for (std::size_t j = 0; j < 3; j++) {
                if (i != j) {
                    offs.push_back(std::pow(s[i][j], 2));
                }
            }
        }
        if (Sum(offs) < 1e-18) {
            break;
        }
        for (std::size_t p = 0; p < 3; p++) {
            for (std::size_t q = p + 1; q < 3; q++) {
                if (std::fabs(s[p][q]) < 1e-30) {
                    continue;
                }
                double th = 0.5 * std::atan2(2 * s[p][q], s[q][q] - s[p][p]), cs = std::cos(th), sn = std::sin(th);
                for (std::size_t k = 0; k < 3; k++) {
                    double skp = s[k][p], skq = s[k][q];
                    s[k][p] = cs * skp - sn * skq;
                    s[k][q] = sn * skp + cs * skq;
                }
                for (std::size_t k = 0; k < 3; k++) {
                    double spk = s[p][k], sqk = s[q][k];
                    s[p][k] = cs * spk - sn * sqk;
                    s[q][k] = sn * spk + cs * sqk;
                }
                for (std::size_t k = 0; k < 3; k++) {
                    double vkp = vm[k][p], vkq = vm[k][q];
                    vm[k][p] = cs * vkp - sn * vkq;
                    vm[k][q] = sn * vkp + cs * vkq;
                }
            }
        }
    }
    std::size_t m = 0;
    for (std::size_t i = 1; i < 3; i++) {
        if (s[i][i] < s[m][m]) {
            m = i;
        }
    }
    V3     v = {vm[0][m], vm[1][m], vm[2][m]};
    double nn = std::sqrt(Sum({v[0] * v[0], v[1] * v[1], v[2] * v[2]}));
    if (nn == 0) {
        nn = 1.0;
    }
    return {v[0] / nn, v[1] / nn, v[2] / nn};
}

void RigNode::Bind(const std::vector<RigNode> &nodes) {
    M4 local = MatFromRT(r, t);
    world = parent < 0 ? local : MatMulExact(local, nodes[std::size_t(parent)].world);
    inv_world = RigidInv(world);
    world_pos = {world[3][0], world[3][1], world[3][2]};
}

std::vector<RigNode> ReadSkeleton(std::span<const unsigned char> mds) {
    int                  count = static_cast<int>(Get32(mds, mds::kCountAt));
    std::vector<RigNode> nodes(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        std::size_t p = mds::kTable + std::size_t(i) * mds::kRecord;
        RigNode    &n = nodes[std::size_t(i)];
        for (std::size_t row = 0; row < 3; row++) {
            for (std::size_t c = 0; c < 3; c++) {
                n.r[row][c] = GetF32(mds, p + mds::kRotation + row * 16 + c * 4);
            }
        }
        n.t = {GetF32(mds, p + mds::kPosition), GetF32(mds, p + mds::kPosition + 4), GetF32(mds, p + mds::kPosition + 8)};
        n.i = i;
        n.name = NameAt(mds, p + mds::kName, mds::kNameSize);
        n.mesh_off = static_cast<int>(Get32(mds, p + mds::kMesh));
        n.parent = GetI32(mds, p + mds::kParent);
        n.quat = MatToQuat(n.r);
    }
    for (RigNode &n : nodes) {
        n.Bind(nodes);
    }
    return nodes;
}

const RigNode &NodeNamed(const std::vector<RigNode> &nodes, std::string_view name) {
    for (const RigNode &n : nodes) {
        if (n.name == name) {
            return n;
        }
    }
    throw std::runtime_error("the cat has no " + std::string(name));
}

V3 Skinned(const SkinMesh &skin, int i, const std::function<const M4 &(int)> &world) {
    std::size_t k = std::size_t(i);
    V3          a = XformPt(world(skin.b0[k]), skin.p0[k]), b = XformPt(world(skin.b1[k]), skin.p1[k]);
    double      w = skin.w0[k];
    return {a[0] * w + b[0] * (1 - w), a[1] * w + b[1] * (1 - w), a[2] * w + b[2] * (1 - w)};
}

const std::vector<std::pair<int, double>> *VertexWeights::Find(int vertex) const {
    auto found = of.find(vertex);
    return found == of.end() ? nullptr : &found->second;
}

std::unordered_map<int, VertexWeights> LoadWeights(const ChrPack &pack, std::string_view wgt) {
    std::unordered_map<int, VertexWeights> out;
    const ChrRecord                       *record = pack.Find(wgt);
    if (record == nullptr) {
        return out;
    }
    for (const MotTrack &track : MotFile::FromRecord(*record).tracks) {
        if (track.w2 != mot::kWeight) {
            continue;
        }
        VertexWeights &per = out[static_cast<int>(track.w0)];
        for (const MotKeyframe &key : track.keys) {
            int  vertex = static_cast<int>(key.Frame());
            auto found = per.of.find(vertex);
            if (found == per.of.end()) {
                per.order.push_back(vertex);
                found = per.of.emplace(vertex, std::vector<std::pair<int, double>>{}).first;
            }
            found->second.emplace_back(static_cast<int>(track.w1), double(key.Value(0)) / 100.0);
        }
    }
    return out;
}

std::vector<Tri> MdtTriangles(const MdtMesh &m) {
    std::vector<Tri> tris;
    for (const MdtMesh::Submesh &sub : m.submeshes) {
        const auto &recs = sub.records;
        if (sub.prim == 3) {
            for (std::size_t k = 0; k + 2 < recs.size(); k += 3) {
                tris.push_back({recs[k][0], recs[k + 1][0], recs[k + 2][0]});
            }
        } else if (sub.prim == 4) {
            for (std::size_t i = 0; i + 2 < recs.size(); i++) {
                int a = i % 2 == 0 ? recs[i][0] : recs[i + 1][0], b = i % 2 == 0 ? recs[i + 1][0] : recs[i][0], c = recs[i + 2][0];
                if (a == b || b == c || a == c) {
                    continue;
                }
                tris.push_back({a, b, c});
            }
        } else {
            throw std::runtime_error("an MDT primitive that is neither a list nor a strip");
        }
    }
    return tris;
}

std::optional<SkinMesh> BuildMesh(std::span<const unsigned char> mds, const RigNode &node, const std::vector<RigNode> &nodes) {
    MdtMesh         m = MdtMesh::Parse(mds.subspan(std::size_t(node.mesh_off)));
    std::vector<V3> local;
    for (const auto &v : m.positions) {
        local.push_back({v[0], v[1], v[2]});
    }
    std::vector<Tri> tris = MdtTriangles(m);
    if (tris.empty()) {
        return std::nullopt;
    }
    double lo[3] = {INFINITY, INFINITY, INFINITY}, hi[3] = {-INFINITY, -INFINITY, -INFINITY};
    for (const V3 &v : local) {
        for (std::size_t k = 0; k < 3; k++) {
            lo[k] = std::min(lo[k], v[k]);
            hi[k] = std::max(hi[k], v[k]);
        }
    }
    bool                         is_skin = std::max(std::max(hi[0] - lo[0], hi[1] - lo[1]), hi[2] - lo[2]) > 5.0;
    std::vector<const RigNode *> candidates;
    for (const RigNode &n : nodes) {
        if (n.parent >= 0 && !(n.mesh_off != 0 && MeshDim(mds, n) > 5.0)) {
            candidates.push_back(&n);
        }
    }
    SkinMesh out;
    out.node = node.i;
    out.skin = is_skin;
    out.nv = static_cast<int>(local.size());
    out.tris = tris;
    for (const V3 &v : local) {
        V3 vm = XformPt(node.world, v);
        if (!is_skin || candidates.empty()) {
            out.b0.push_back(node.i);
            out.p0.push_back(v);
            out.b1.push_back(node.i);
            out.p1.push_back({0.0, 0.0, 0.0});
            out.w0.push_back(1.0);
            continue;
        }
        std::vector<std::pair<double, const RigNode *>> d;
        for (const RigNode *n : candidates) {
            d.emplace_back(std::pow(vm[0] - n->world_pos[0], 2) + std::pow(vm[1] - n->world_pos[1], 2) + std::pow(vm[2] - n->world_pos[2], 2), n);
        }
        std::stable_sort(d.begin(), d.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
        double d0 = std::sqrt(d[0].first) + 1e-4, d1 = std::sqrt(d[1].first) + 1e-4;
        double wa = (1.0 / d0) / (1.0 / d0 + 1.0 / d1);
        out.b0.push_back(d[0].second->i);
        out.p0.push_back(XformPt(d[0].second->inv_world, vm));
        out.b1.push_back(d[1].second->i);
        out.p1.push_back(XformPt(d[1].second->inv_world, vm));
        out.w0.push_back(wa);
    }
    return out;
}

std::optional<SkinMesh> BuildMeshWeighted(std::span<const unsigned char> mds, const RigNode &node, const std::vector<RigNode> &nodes,
                                          const VertexWeights &weights) {
    MdtMesh         m = MdtMesh::Parse(mds.subspan(std::size_t(node.mesh_off)));
    std::vector<V3> local;
    for (const auto &v : m.positions) {
        local.push_back({v[0], v[1], v[2]});
    }
    std::vector<Tri> tris = MdtTriangles(m);
    if (tris.empty()) {
        return std::nullopt;
    }
    SkinMesh out;
    out.node = node.i;
    out.skin = true;
    out.nv = static_cast<int>(local.size());
    out.tris = tris;
    for (std::size_t vi = 0; vi < local.size(); vi++) {
        const V3                           &v = local[vi];
        V3                                  vm = XformPt(node.world, v);
        std::vector<std::pair<int, double>> sorted;
        if (const auto *found = weights.Find(static_cast<int>(vi))) {
            sorted = *found;
        }
        std::stable_sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return -a.second < -b.second; });
        std::vector<std::pair<int, double>> infl;
        for (std::size_t k = 0; k < sorted.size() && k < 2; k++) {
            if (0 <= sorted[k].first && sorted[k].first < static_cast<int>(nodes.size())) {
                infl.push_back(sorted[k]);
            }
        }
        if (infl.empty()) {
            out.b0.push_back(node.i);
            out.p0.push_back(v);
            out.b1.push_back(node.i);
            out.p1.push_back({0.0, 0.0, 0.0});
            out.w0.push_back(1.0);
            continue;
        }
        auto [b0, wa] = infl[0];
        auto [b1, wb] = infl.size() > 1 ? infl[1] : std::pair{b0, 0.0};
        double total = wa + wb;
        out.b0.push_back(b0);
        out.p0.push_back(XformPt(nodes[std::size_t(b0)].inv_world, vm));
        out.b1.push_back(b1);
        out.p1.push_back(XformPt(nodes[std::size_t(b1)].inv_world, vm));
        out.w0.push_back(total > 0 ? wa / total : 1.0);
    }
    return out;
}

std::vector<Track> BuildTracks(const ChrPack &pack, std::string_view mot, int count) {
    std::vector<Track> tracks;
    for (const MotTrack &t : MotFile::FromRecord(pack.Require(mot)).tracks) {
        if (t.w0 >= static_cast<std::uint32_t>(count) || (t.w2 != mot::kRotation && t.w2 != mot::kTranslation) || t.keys.empty()) {
            continue;
        }
        Track track;
        track.node = static_cast<int>(t.w0);
        track.chan = static_cast<int>(t.w2);
        for (const MotKeyframe &key : t.keys) {
            track.frames.push_back(double(key.Frame()));
            track.vals.push_back({key.Value(0), key.Value(1), key.Value(2), key.Value(3)});
        }
        tracks.push_back(std::move(track));
    }
    return tracks;
}

const ChrRecord *FindCfg(const ChrPack &pack) {
    for (const ChrRecord &record : pack.records) {
        if (EndsFolded(record.name, ".cfg")) {
            return &record;
        }
    }
    return nullptr;
}

std::pair<std::string, std::string> CfgNames(std::span<const unsigned char> cfg) {
    std::string text(cfg.begin(), cfg.end());
    std::smatch model, motion;
    std::regex_search(text, model, std::regex(R"re(MODEL\s+"([^"]+)")re"));
    std::regex_search(text, motion, std::regex(R"re(MOTION\s+\d+\s*,\s*"([^"]+\.mot)")re"));
    return {model.empty() ? std::string{} : model[1].str(), motion.empty() ? std::string{} : motion[1].str()};
}

std::vector<Tri> Delaunay(const std::vector<V2> &points) {
    double minx = INFINITY, maxx = -INFINITY, miny = INFINITY, maxy = -INFINITY;
    for (const V2 &q : points) {
        minx = std::min(minx, q[0]);
        maxx = std::max(maxx, q[0]);
        miny = std::min(miny, q[1]);
        maxy = std::max(maxy, q[1]);
    }
    double          cx = 0.5 * (minx + maxx), cy = 0.5 * (miny + maxy), r = 10 * std::max(maxx - minx, maxy - miny) + 1;
    std::vector<V2> p = points;
    p.push_back({cx - r, cy - r});
    p.push_back({cx + r, cy - r});
    p.push_back({cx, cy + r});
    int              n = static_cast<int>(points.size());
    std::vector<Tri> tris = {
        {n, n + 1, n + 2}
    };
    for (int i = 0; i < n; i++) {
        std::vector<Tri> bad, keep;
        for (const Tri &t : tris) {
            (InCircum(p, t, p[std::size_t(i)]) ? bad : keep).push_back(t);
        }
        std::vector<std::pair<int, int>>   edges;
        std::map<std::pair<int, int>, int> counts;
        for (const Tri &t : bad) {
            for (int k = 0; k < 3; k++) {
                std::pair<int, int> e = {std::min(t[std::size_t(k)], t[std::size_t((k + 1) % 3)]), std::max(t[std::size_t(k)], t[std::size_t((k + 1) % 3)])};
                if (!counts.contains(e)) {
                    counts[e] = 0;
                    edges.push_back(e);
                }
                counts[e]++;
            }
        }
        tris = keep;
        for (auto [u, v] : edges) {
            if (counts[{u, v}] != 1) {
                continue;
            }
            if (Area2(p[std::size_t(u)], p[std::size_t(v)], p[std::size_t(i)]) > 0) {
                tris.push_back({u, v, i});
            } else {
                tris.push_back({v, u, i});
            }
        }
    }
    std::vector<Tri> out;
    for (const Tri &t : tris) {
        if (std::max({t[0], t[1], t[2]}) < n) {
            out.push_back(t);
        }
    }
    return out;
}

bool PtInRing(const V2 &p, const std::vector<V2> &ring) {
    bool inside = false;
    for (std::size_t i = 0; i < ring.size(); i++) {
        const V2 &a = ring[i], &b = ring[i == 0 ? ring.size() - 1 : i - 1];
        if ((a[1] > p[1]) != (b[1] > p[1]) && p[0] < a[0] + (p[1] - a[1]) / (b[1] - a[1]) * (b[0] - a[0])) {
            inside = !inside;
        }
    }
    return inside;
}

} // namespace expanded::rig
