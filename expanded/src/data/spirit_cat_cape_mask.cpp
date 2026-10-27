#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <map>
#include <numbers>
#include <optional>
#include <set>
#include <stdexcept>

#include "expanded/data/spirit_cat_parts.hpp"

namespace expanded::spirit_cat {

using namespace rig;

namespace {

constexpr double kPi = std::numbers::pi;

std::vector<V3> Fur(const std::vector<RigNode> &nodes, const SkinMesh &skin) {
    auto            world = [&](int b) -> const M4            &{ return nodes[std::size_t(b)].world; };
    std::vector<V3> fur;
    for (int i = 0; i < skin.nv; i++) {
        fur.push_back(Skinned(skin, i, world));
    }
    return fur;
}

// ---- the cape ----

constexpr int    kCapeCollar[] = {483, 503, 485, 477, 478};
constexpr double kCapeWidth = 3.5, kCapeLength = 4.0, kCapeLift = 0.22;
constexpr double kCapeHang = 0.52, kCapeRowTaut = 0.55, kCapeSideSlope = 0.9, kCapeBillow = 0.5;
constexpr double kCapeCollarBias[] = {0.18, 0.18};

struct BoundWorld {
    V3 centre;
    M3 axes;
    V3 radii;
};

std::vector<BoundWorld> CapeBoundsWorld(const std::vector<RigNode> &nodes) {
    std::vector<BoundWorld> out;
    for (const BoundLocal &b : CapeBoundsLocal(nodes)) {
        const M4 &wm = NodeNamed(nodes, b.bone).world;
        V3        aw = XformPt(wm, b.a), bw = XformPt(wm, b.b), upw = VecMatExact(b.up, RotOf(wm));
        V3 z = UnitExact({aw[0] - bw[0], aw[1] - bw[1], aw[2] - bw[2]}), x = UnitExact(Cross(upw, z)), y = Cross(z, x);
        out.push_back({
            {(aw[0] + bw[0]) / 2, (aw[1] + bw[1]) / 2, (aw[2] + bw[2]) / 2},
            {x,                   y,                   z                  },
            b.radii
        });
    }
    return out;
}

double BoundDepth(const BoundWorld &bw, const V3 &p) {
    V3                  d = {p[0] - bw.centre[0], p[1] - bw.centre[1], p[2] - bw.centre[2]};
    std::vector<double> terms;
    for (std::size_t i = 0; i < 3; i++) {
        terms.push_back(std::pow(DotExact(d, bw.axes[i]) / bw.radii[i], 2));
    }
    return std::sqrt(Sum(terms));
}

V3 ClearBounds(const std::vector<BoundWorld> &bws, const V3 &p, double margin = 0.03, double max_move = 1.0, double step = 0.04) {
    V3   q = p;
    auto clear = [&](const V3 &at) {
        return std::all_of(bws.begin(), bws.end(), [&](const BoundWorld &b) { return BoundDepth(b, at) >= 1 + margin; });
    };
    if (clear(q)) {
        return q;
    }
    const BoundWorld *deep = nullptr;
    double            best = std::numeric_limits<double>::infinity();
    for (const BoundWorld &b : bws) {
        double depth = BoundDepth(b, q);
        if (depth < best) {
            best = depth;
            deep = &b;
        }
    }
    V3 d;
    if (q[1] > deep->centre[1]) {
        d = {0.0, 1.0, 0.0};
    } else {
        d = {q[0] - deep->centre[0], 0.0, q[2] - deep->centre[2]};
        double n = std::sqrt(d[0] * d[0] + d[2] * d[2]);
        d = n > 1e-6 ? V3{d[0] / n, 0.0, d[2] / n} : V3{0.0, -1.0, 0.0};
    }
    for (int it = 0; it < static_cast<int>(max_move / step); it++) {
        q = {q[0] + d[0] * step, q[1] + d[1] * step, q[2] + d[2] * step};
        if (clear(q)) {
            return q;
        }
    }
    return p;
}

// The cape's rest lattice: each row walks the body's surface outward from the spine, continues past the flank at
// kCapeSideSlope, is flattened toward its chord and sampled at equal arc length; rows past the rump hang off the last;
// the spine profile is raised onto its upper hull, the billow eases in along the hang, rows clear the collision bounds.
SkinMesh BuildCapeMesh(const std::vector<RigNode> &nodes, const SkinMesh &skin) {
    const int       cols = kCapeCols;
    std::vector<V3> fur = Fur(nodes, skin);
    auto            cast = [&](double x, double z) -> std::optional<double> {
        std::optional<double> best;
        for (const Tri &t : skin.tris) {
            const V3 &a = fur[std::size_t(t[0])], &b = fur[std::size_t(t[1])], &c = fur[std::size_t(t[2])];
            double    d = (b[2] - a[2]) * (c[0] - a[0]) - (c[2] - a[2]) * (b[0] - a[0]);
            if (std::fabs(d) < 1e-9) {
                continue;
            }
            double u = ((z - a[2]) * (c[0] - a[0]) - (x - a[0]) * (c[2] - a[2])) / d;
            double v = ((x - a[0]) * (b[2] - a[2]) - (z - a[2]) * (b[0] - a[0])) / d;
            if (u < 0 || v < 0 || u + v > 1) {
                continue;
            }
            double y = a[1] + u * (b[1] - a[1]) + v * (c[1] - a[1]);
            if (!best || y > *best) {
                best = y;
            }
        }
        return best;
    };
    auto ridge = [&](double x, double z) -> std::optional<double> {
        constexpr double dz = 0.2;
        auto             here = cast(x, z);
        if (!here) {
            return std::nullopt;
        }
        std::vector<std::pair<double, double>> hits;
        for (auto [w, o] : {
                 std::pair{0.25, -dz},
                 std::pair{0.25, dz }
        }) {
            if (auto y = cast(x, z + o)) {
                hits.emplace_back(w, *y);
            }
        }
        hits.emplace_back(0.5, *here);
        std::vector<double> num, den;
        for (auto [w, y] : hits) {
            num.push_back(w * y);
            den.push_back(w);
        }
        return Sum(num) / Sum(den);
    };
    std::vector<BoundWorld> bws = CapeBoundsWorld(nodes);
    auto                    world = [&](int b) -> const M4                    &{ return nodes[std::size_t(b)].world; };
    std::vector<V3>         ring;
    for (int i : kCapeCollar) {
        ring.push_back(Skinned(skin, i, world));
    }
    std::vector<double> arc = {0.0};
    for (std::size_t k = 1; k < ring.size(); k++) {
        arc.push_back(arc.back() + Dist(ring[k - 1], ring[k]));
    }
    std::vector<V3> collar;
    for (int c = 0; c < cols; c++) {
        double      want = arc.back() * c / (cols - 1);
        std::size_t k = ring.size() - 1;
        for (std::size_t kk = 1; kk < ring.size(); kk++) {
            if (arc[kk] >= want) {
                k = kk;
                break;
            }
        }
        double t = arc[k] > arc[k - 1] ? (want - arc[k - 1]) / (arc[k] - arc[k - 1]) : 0.0;
        collar.push_back({ring[k - 1][0] + (ring[k][0] - ring[k - 1][0]) * t, ring[k - 1][1] + (ring[k][1] - ring[k - 1][1]) * t,
                          ring[k - 1][2] + (ring[k][2] - ring[k - 1][2]) * t});
    }
    std::vector<V3> top;
    for (int c = 0; c < cols; c++) {
        double g = std::cos(kPi / 2 * std::fabs(2 * c / double(cols - 1) - 1));
        top.push_back({collar[std::size_t(c)][0], collar[std::size_t(c)][1] + kCapeCollarBias[0] * g, collar[std::size_t(c)][2] + kCapeCollarBias[1] * g});
    }
    std::vector<double> zs;
    for (const V3 &p : collar) {
        zs.push_back(p[2]);
    }
    double zc = Sum(zs) / cols, top_half = 0;
    for (const V3 &p : top) {
        top_half = std::max(top_half, std::fabs(p[0]));
    }

    auto cross_section = [&](double z, double half_x) -> std::optional<std::vector<V3>> {
        constexpr double                                  step = 0.03;
        std::vector<V3>                                   out(static_cast<std::size_t>(cols));
        std::map<int, std::vector<std::array<double, 2>>> raw;
        for (int way : {-1, 1}) {
            std::vector<std::array<double, 2>>   pts;
            double                               x = 0.0;
            std::optional<std::array<double, 2>> edge;
            while (std::fabs(x) <= half_x + step) {
                double y;
                if (auto ry = ridge(x, z)) {
                    y = *ry + kCapeLift;
                    edge = std::array<double, 2>{x, y};
                } else if (edge) {
                    y = (*edge)[1] - kCapeSideSlope * std::fabs(x - (*edge)[0]);
                } else {
                    return std::nullopt;
                }
                pts.push_back({x, y});
                x += way * step;
            }
            raw[way] = pts;
        }
        for (int way : {-1, 1}) {
            auto &pts = raw[way];
            for (int it = 0; it < 6; it++) {
                for (std::size_t i = 1; i + 1 < pts.size(); i++) {
                    pts[i][1] = 0.25 * pts[i - 1][1] + 0.5 * pts[i][1] + 0.25 * pts[i + 1][1];
                }
            }
        }
        if (kCapeRowTaut < 1.0) {
            double xa = raw[-1].back()[0], ya = raw[-1].back()[1], xb = raw[1].back()[0], yb = raw[1].back()[1];
            for (int way : {-1, 1}) {
                for (auto &q : raw[way]) {
                    double t = std::fabs(xb - xa) > 1e-6 ? (q[0] - xa) / (xb - xa) : 0.0;
                    double chord = ya + (yb - ya) * t;
                    q[1] = chord + (q[1] - chord) * kCapeRowTaut;
                }
            }
        }
        for (int way : {-1, 1}) {
            std::vector<V3>                      pts;
            double                               a = 0.0;
            std::optional<std::array<double, 2>> last;
            for (const auto &q : raw[way]) {
                if (last) {
                    a += Dist(q, *last);
                }
                pts.push_back({q[0], q[1], a});
                last = q;
            }
            for (int c = 0; c < cols; c++) {
                double f = (c / double(cols - 1) - 0.5) * 2;
                if ((f < 0 && way > 0) || (f > 0 && way < 0) || (f == 0 && way > 0)) {
                    continue;
                }
                double want = std::fabs(f) * pts.back()[2];
                double qx = pts.back()[0], qy = pts.back()[1];
                for (std::size_t i = 1; i < pts.size(); i++) {
                    if (pts[i][2] >= want) {
                        double x0 = pts[i - 1][0], y0 = pts[i - 1][1], a0 = pts[i - 1][2], x1 = pts[i][0], y1 = pts[i][1], a1 = pts[i][2];
                        double t = a1 > a0 ? (want - a0) / (a1 - a0) : 0.0;
                        qx = x0 + (x1 - x0) * t;
                        qy = y0 + (y1 - y0) * t;
                        break;
                    }
                }
                out[std::size_t(c)] = {qx, qy, z};
            }
        }
        return out;
    };

    std::vector<std::vector<V3>> rows = {top};
    for (int r = 1; r < kCapeRows; r++) {
        double s = r / double(kCapeRows - 1), z = zc - kCapeLength * s;
        auto   row = cross_section(z, top_half * (1 - s) + (kCapeWidth / 2) * s);
        if (!row) {
            std::vector<V3> hang;
            for (int c = 0; c < cols; c++) {
                const V3 &above = rows.back()[std::size_t(c)];
                hang.push_back({above[0], above[1] - kCapeHang * (kCapeLength / (kCapeRows - 1)), z});
            }
            row = hang;
        }
        rows.push_back(*row);
    }
    const std::size_t mid = std::size_t(cols / 2);

    struct Prof {
        double z, y;
        int    r;
    };

    std::vector<Prof> prof;
    for (int r = 0; r < kCapeRows; r++) {
        prof.push_back({rows[std::size_t(r)][mid][2], rows[std::size_t(r)][mid][1], r});
    }
    std::stable_sort(prof.begin(), prof.end(), [](const Prof &a, const Prof &b) {
        if (a.z != b.z) {
            return a.z < b.z;
        }
        if (a.y != b.y) {
            return a.y < b.y;
        }
        return a.r < b.r;
    });
    std::vector<Prof> hull;
    for (const Prof &q : prof) {
        while (hull.size() >= 2) {
            const Prof &o = hull[hull.size() - 2], &a = hull.back();
            if ((a.z - o.z) * (q.y - o.y) - (a.y - o.y) * (q.z - o.z) >= 0) {
                hull.pop_back();
            } else {
                break;
            }
        }
        hull.push_back(q);
    }
    for (const Prof &q : prof) {
        if (q.r == 0) {
            continue;
        }
        for (std::size_t i = 0; i + 1 < hull.size(); i++) {
            double z0 = hull[i].z, y0 = hull[i].y, z1 = hull[i + 1].z, y1 = hull[i + 1].y;
            if (z0 <= q.z && q.z <= z1) {
                double lift = (y0 + (y1 - y0) * (z1 > z0 ? (q.z - z0) / (z1 - z0) : 0.0)) - q.y;
                if (lift > 0) {
                    for (V3 &v : rows[std::size_t(q.r)]) {
                        v[1] += lift;
                    }
                }
                break;
            }
        }
    }
    for (int r = 1; r < kCapeRows; r++) {
        double lift = kCapeBillow * std::pow(r / double(kCapeRows - 1), 1.5);
        for (V3 &v : rows[std::size_t(r)]) {
            v[1] += lift;
        }
    }
    for (int r = 1; r < kCapeRows; r++) {
        double lift = -std::numeric_limits<double>::infinity();
        for (const V3 &v : rows[std::size_t(r)]) {
            lift = std::max(lift, ClearBounds(bws, v)[1] - v[1]);
        }
        if (lift > 0) {
            for (V3 &v : rows[std::size_t(r)]) {
                v[1] += lift;
            }
        }
    }
    const RigNode &anchor = NodeNamed(nodes, kCapeAnchor);
    M4             inv = RigidInv(anchor.world);
    SkinMesh       me;
    me.node = skin.node;
    me.skin = true;
    for (const auto &row : rows) {
        for (const V3 &v : row) {
            V3 p0 = XformPt(inv, v);
            me.b0.push_back(anchor.i);
            me.p0.push_back(p0);
            me.b1.push_back(anchor.i);
            me.p1.push_back(p0);
            me.w0.push_back(1.0);
            me.nv++;
        }
    }
    return me;
}

// ---- the mask ----

constexpr V3     kMaskCore = {0.0, 3.36, 3.35};
constexpr double kMaskR = 0.88;
constexpr V2     kMaskEye = {0.507, 3.368};
constexpr V2     kMaskHole = {0.32, 0.20};
constexpr double kMaskHoleAt = 0.500, kMaskHoleTilt = 11.0, kMaskHoleRound = 0.70;
constexpr double kMaskEyeShape[] = {0.796, 0.999, 1.055, 0.980, 0.924, 0.934, 0.965, 0.990, 1.014, 1.024, 0.993, 0.926,
                                    0.915, 1.025, 1.163, 1.199, 1.116, 1.023, 0.987, 0.952, 0.883, 0.827, 0.746, 0.681};
constexpr V3     kMaskLobe = {0.80, 0.33, 0.40};
constexpr double kMaskLobeRise = 0.03, kMaskLobeTilt = 5.0, kMaskBridge = 0.00;
constexpr double kMaskNoseTop = 3.20, kMaskNoseW = 0.55, kMaskNoseBlend = 0.13;
constexpr double kMaskLift = 0.050;
constexpr int    kMaskSegs = 120;
constexpr double kMaskSmooth = 0.010, kMaskLattice = 0.20, kMaskGrade = 0.42;
constexpr int    kMaskHoleSegs = 22;

double EllipseSd(const V2 &p, const V2 &c, double rx, double ryu, double ryd, double tilt) {
    double ca = std::cos(-tilt), sa = std::sin(-tilt), dx = p[0] - c[0], dy = p[1] - c[1];
    double ux = dx * ca - dy * sa, uy = dx * sa + dy * ca, ry = uy >= 0 ? ryu : ryd;
    return (Hypot({ux / rx, uy / ry}) - 1.0) * std::min(rx, ry);
}

double Smin(double a, double b, double k) {
    if (k <= 1e-6) {
        return std::min(a, b);
    }
    double h = std::max(0.0, std::min(1.0, 0.5 + 0.5 * (b - a) / k));
    return b * (1 - h) + a * h - k * h * (1 - h);
}

double Smax(double a, double b, double k) {
    return -Smin(-a, -b, k);
}

double MaskField(const V2 &p) {
    double tilt = Radians(kMaskLobeTilt);
    double l0 = EllipseSd(p, {-1 * kMaskEye[0], kMaskEye[1] + kMaskLobeRise}, kMaskLobe[0], kMaskLobe[1], kMaskLobe[2], -1 * tilt);
    double l1 = EllipseSd(p, {1 * kMaskEye[0], kMaskEye[1] + kMaskLobeRise}, kMaskLobe[0], kMaskLobe[1], kMaskLobe[2], 1 * tilt);
    double f = Smin(l0, l1, kMaskBridge);
    if (kMaskNoseW > 0) {
        f = Smax(f, (kMaskNoseTop - 0.25 * std::pow(p[0] / kMaskNoseW, 2)) - p[1], kMaskNoseBlend);
    }
    return f;
}

V2 OutlineAt(double th) {
    V2     c = {0.0, kMaskEye[1] + kMaskLobeRise}, d = {std::cos(th), std::sin(th)};
    double lo = 0.0, hi = 4.0;
    for (int i = 0; i < 40; i++) {
        double mid = 0.5 * (lo + hi);
        if (MaskField({c[0] + d[0] * mid, c[1] + d[1] * mid}) < 0) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    double r = 0.5 * (lo + hi);
    return {c[0] + d[0] * r, c[1] + d[1] * r};
}

double SegDist(const std::vector<double> &p, const std::vector<double> &a, const std::vector<double> &b) {
    std::size_t         n = a.size();
    std::vector<double> ab(n), ap(n);
    for (std::size_t k = 0; k < n; k++) {
        ab[k] = b[k] - a[k];
        ap[k] = p[k] - a[k];
    }
    std::vector<double> sq, dot;
    for (std::size_t k = 0; k < n; k++) {
        sq.push_back(ab[k] * ab[k]);
        dot.push_back(ap[k] * ab[k]);
    }
    double              n2 = Sum(sq);
    double              t = n2 < 1e-18 ? 0.0 : std::max(0.0, std::min(1.0, Sum(dot) / n2));
    std::vector<double> rest;
    for (std::size_t k = 0; k < n; k++) {
        rest.push_back(std::pow(ap[k] - t * ab[k], 2));
    }
    return std::sqrt(Sum(rest));
}

std::vector<int> DecimateIdx(const std::vector<std::vector<double>> &keys, double tol, int lo, int hi) {
    if (hi - lo < 2) {
        return {lo, hi};
    }
    double worst = -1.0;
    int    wi = lo;
    for (int i = lo + 1; i < hi; i++) {
        double d = SegDist(keys[std::size_t(i)], keys[std::size_t(lo)], keys[std::size_t(hi)]);
        if (d > worst) {
            worst = d;
            wi = i;
        }
    }
    if (worst <= tol) {
        return {lo, hi};
    }
    std::vector<int> left = DecimateIdx(keys, tol, lo, wi);
    left.pop_back();
    std::vector<int> right = DecimateIdx(keys, tol, wi, hi);
    left.insert(left.end(), right.begin(), right.end());
    return left;
}

using Project = std::function<V3(double, double)>;

std::vector<V2> MaskOutlineHalf(const Project &project) {
    std::vector<V2> line;
    for (int k = 0; k <= kMaskSegs; k++) {
        line.push_back(OutlineAt(-kPi / 2 + kPi * k / kMaskSegs));
    }
    line[0] = {0.0, line[0][1]};
    line.back() = {0.0, line.back()[1]};
    std::vector<std::vector<double>> keys;
    for (const V2 &q : line) {
        if (project) {
            V3 p = project(q[0], q[1]);
            keys.push_back({p[0], p[1], p[2]});
        } else {
            keys.push_back({q[0], q[1]});
        }
    }
    std::vector<int> kept = DecimateIdx(keys, kMaskSmooth, 0, static_cast<int>(keys.size()) - 1);
    std::vector<int> out;
    for (std::size_t i = 0; i + 1 < kept.size(); i++) {
        int a = kept[i], b = kept[i + 1];
        out.push_back(a);
        int n = static_cast<int>(Dist(line[std::size_t(a)], line[std::size_t(b)]) / kMaskLattice);
        for (int k = 1; k < n; k++) {
            out.push_back(a + static_cast<int>(Round((b - a) * k / double(n), 0)));
        }
    }
    out.push_back(kept.back());
    std::set<int>   seen;
    std::vector<V2> result;
    for (int i : out) {
        if (seen.insert(i).second) {
            result.push_back(line[std::size_t(i)]);
        }
    }
    return result;
}

double EyeRadius(double th) {
    constexpr int n = static_cast<int>(std::size(kMaskEyeShape));
    double        x = FMod(th, 2 * kPi) / (2 * kPi) * n;
    int           i = Mod(static_cast<int>(x), n);
    double        f = x - static_cast<int>(x);
    double        r = kMaskEyeShape[i] * (1 - f) + kMaskEyeShape[(i + 1) % n] * f;
    return r * (1 - kMaskHoleRound) + kMaskHoleRound;
}

std::vector<V2> MaskHoleRing(int side, int segs = 0, double grow = 0.0) {
    if (segs == 0) {
        segs = kMaskHoleSegs;
    }
    double          tilt = Radians(side * kMaskHoleTilt), ca = std::cos(tilt), sa = std::sin(tilt), cx = side * kMaskHoleAt, cy = kMaskEye[1];
    std::vector<V2> pts;
    for (int k = 0; k < segs; k++) {
        double th = -2 * kPi * k / segs;
        double r = EyeRadius(side > 0 ? th : kPi - th);
        double ux = kMaskHole[0] * r * std::cos(th), uy = kMaskHole[1] * r * std::sin(th);
        if (grow != 0) {
            double d = Hypot({ux, uy});
            if (d > 1e-9) {
                ux *= (d + grow) / d;
                uy *= (d + grow) / d;
            }
        }
        pts.push_back({cx + ux * ca - uy * sa, cy + ux * sa + uy * ca});
    }
    return pts;
}

std::pair<std::vector<V2>, std::vector<Tri>> MaskMesh2d(const Project &project) {
    const double    h = kMaskLattice;
    std::vector<V2> pts = MaskOutlineHalf(project);
    for (const V2 &p : MaskHoleRing(1)) {
        pts.push_back(p);
    }
    std::vector<V2> lip = MaskHoleRing(1, 64, 0.0);
    std::vector<V2> bnd = pts;
    double          step = h / 3.0;
    double          y = kMaskEye[1] - 1.2;
    while (y < kMaskEye[1] + 1.2) {
        double x = 0.0;
        while (x < 1.8) {
            V2     q = {x, y};
            double d = std::numeric_limits<double>::infinity();
            for (const V2 &b : bnd) {
                d = std::min(d, Dist(q, b));
            }
            double r = std::max(kMaskGrade * h, std::min(h, 0.85 * d));
            if (x > 1e-9 && x < 0.5 * r) {
                x += step;
                continue;
            }
            if (MaskField(q) < -0.40 * r && !PtInRing(q, MaskHoleRing(1, 40, 0.40 * r)) &&
                std::all_of(pts.begin(), pts.end(), [&](const V2 &o) { return Dist(q, o) >= r; })) {
                pts.push_back(q);
            }
            x += step;
        }
        y += step;
    }
    std::vector<Tri> half;
    for (const Tri &t : Delaunay(pts)) {
        V2 c = {Sum({pts[std::size_t(t[0])][0], pts[std::size_t(t[1])][0], pts[std::size_t(t[2])][0]}) / 3.0,
                Sum({pts[std::size_t(t[0])][1], pts[std::size_t(t[1])][1], pts[std::size_t(t[2])][1]}) / 3.0};
        if (c[0] < 0) {
            continue;
        }
        if (MaskField(c) < 0 && !PtInRing(c, lip)) {
            half.push_back(t);
        }
    }
    std::vector<V2>    out = pts;
    std::map<int, int> mir;
    for (std::size_t i = 0; i < pts.size(); i++) {
        if (std::fabs(pts[i][0]) < 1e-9) {
            mir[int(i)] = int(i);
        } else {
            mir[int(i)] = static_cast<int>(out.size());
            out.push_back({-pts[i][0], pts[i][1]});
        }
    }
    std::vector<Tri> tris = half;
    for (const Tri &t : half) {
        tris.push_back({mir[t[2]], mir[t[1]], mir[t[0]]});
    }
    std::set<int> used_set;
    for (const Tri &t : tris) {
        for (int i : t) {
            used_set.insert(i);
        }
    }
    std::vector<int>   used(used_set.begin(), used_set.end());
    std::map<int, int> ren;
    for (std::size_t k = 0; k < used.size(); k++) {
        ren[used[k]] = int(k);
    }
    std::vector<V2> poly;
    for (int i : used) {
        poly.push_back(out[std::size_t(i)]);
    }
    std::vector<Tri> renamed;
    for (const Tri &t : tris) {
        renamed.push_back({ren[t[0]], ren[t[1]], ren[t[2]]});
    }
    return {poly, renamed};
}

double MaskClearance() {
    std::optional<double> worst;
    for (int side : {-1, 1}) {
        for (const V2 &p : MaskHoleRing(side, 72)) {
            double d = -MaskField(p);
            if (!worst || d < *worst) {
                worst = d;
            }
        }
    }
    return *worst;
}

V3 MaskDir(double u, double v) {
    double yaw = u / kMaskR, pitch = (v - kMaskCore[1]) / kMaskR;
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
}

std::optional<std::pair<V3, V3>> MaskRay(const std::vector<V3> &fur, const std::vector<Tri> &tris, double u, double v) {
    V3                    d = MaskDir(u, v);
    const V3             &o = kMaskCore;
    std::optional<double> best;
    std::vector<V3>       acc;
    for (const Tri &tri : tris) {
        const V3 &a = fur[std::size_t(tri[0])], &b = fur[std::size_t(tri[1])], &c = fur[std::size_t(tri[2])];
        V3        e1 = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, e2 = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        V3        h = Cross(d, e2);
        double    det = DotExact(e1, h);
        if (std::fabs(det) < 1e-9) {
            continue;
        }
        V3     s = {o[0] - a[0], o[1] - a[1], o[2] - a[2]};
        double uu = DotExact(s, h) / det;
        if (uu < -1e-9 || uu > 1 + 1e-9) {
            continue;
        }
        V3     q = Cross(s, e1);
        double vv = DotExact(d, q) / det;
        if (vv < -1e-9 || uu + vv > 1 + 1e-9) {
            continue;
        }
        double t = DotExact(e2, q) / det;
        if (t <= 0.05) {
            continue;
        }
        V3 n = UnitExact(Cross(e1, e2));
        if (DotExact(n, d) < 0) {
            n = {-n[0], -n[1], -n[2]};
        }
        if (!best || t > *best + 1e-9) {
            best = t;
            acc = {n};
        } else if (t > *best - 1e-9) {
            acc.push_back(n);
        }
    }
    if (!best) {
        return std::nullopt;
    }
    std::vector<double> xs, ys, zs;
    for (const V3 &n : acc) {
        xs.push_back(n[0]);
        ys.push_back(n[1]);
        zs.push_back(n[2]);
    }
    V3 nn = UnitExact({Sum(xs), Sum(ys), Sum(zs)});
    return std::pair{
        V3{o[0] + d[0] * *best, o[1] + d[1] * *best, o[2] + d[2] * *best},
        nn
    };
}

} // namespace

std::vector<BoundLocal> CapeBoundsLocal(const std::vector<RigNode> &cat_nodes) {
    std::vector<BoundLocal> out;
    for (const CapeBound &b : kCapeBounds) {
        const RigNode &n = NodeNamed(cat_nodes, b.bone);
        M4             inv = RigidInv(n.world);
        const M4      &r = n.world;
        V3             a = XformPt(inv, {b.centre[0] + b.axis[0], b.centre[1] + b.axis[1], b.centre[2] + b.axis[2]});
        V3             bb = XformPt(inv, {b.centre[0] - b.axis[0], b.centre[1] - b.axis[1], b.centre[2] - b.axis[2]});
        V3             up;
        for (std::size_t j = 0; j < 3; j++) {
            up[j] = Sum({r[j][0] * 0.0, r[j][1] * 1.0, r[j][2] * 0.0});
        }
        BoundLocal local{b.bone, {}, {}, {}, b.radii, b.damp};
        for (std::size_t k = 0; k < 3; k++) {
            local.a[k] = Round(a[k], 6);
            local.b[k] = Round(bb[k], 6);
            local.up[k] = Round(up[k], 6);
        }
        out.push_back(local);
    }
    return out;
}

std::vector<V3> CapeRestLocal(const std::vector<RigNode> &cat_nodes, const SkinMesh &skin) {
    SkinMesh        me = BuildCapeMesh(cat_nodes, skin);
    auto            world = [&](int b) -> const M4            &{ return cat_nodes[std::size_t(b)].world; };
    M4              inv = RigidInv(NodeNamed(cat_nodes, kCapeAnchor).world);
    std::vector<V3> out;
    for (int i = 0; i < me.nv; i++) {
        out.push_back(XformPt(inv, Skinned(me, i, world)));
    }
    return out;
}

SkinMesh BuildMaskMesh(const std::vector<RigNode> &cat_nodes, const SkinMesh &skin, const Log &log) {
    std::vector<V3> fur = Fur(cat_nodes, skin);
    const RigNode  &head = NodeNamed(cat_nodes, kMaskAnchor);
    M4              inv = RigidInv(head.world);
    double          gap = MaskClearance();
    if (gap < 0.02) {
        throw std::runtime_error(std::format("mask: the eye hole breaks the outline (clearance {:+.3f})", gap));
    }
    Project project = [&](double u, double v) {
        auto hit = MaskRay(fur, skin.tris, std::fabs(u), v);
        return hit ? hit->first : V3{std::fabs(u), v, kMaskCore[2] + kMaskR};
    };
    auto [poly, tris] = MaskMesh2d(project);
    std::vector<std::pair<V3, double>> dirs;
    std::vector<double>                floor;
    int                                misses = 0;
    for (const V2 &uv : poly) {
        double u = uv[0], v = uv[1], au = std::fabs(u);
        V3     d = MaskDir(au, v);
        auto   hit = MaskRay(fur, skin.tris, au, v);
        for (double pull : {0.92, 0.85, 0.78, 0.7}) {
            if (hit) {
                break;
            }
            hit = MaskRay(fur, skin.tris, au * pull, kMaskEye[1] + (v - kMaskEye[1]) * pull);
            if (hit) {
                misses++;
            }
        }
        if (!hit) {
            floor.push_back(kMaskR);
            misses++;
        } else {
            auto [p, n] = *hit;
            double face = std::fabs(DotExact(d, n));
            floor.push_back(Dist(p, kMaskCore) + kMaskLift / std::max(face, 0.35));
        }
        dirs.emplace_back(d, u < 0 ? -1.0 : 1.0);
    }
    if (misses > 0) {
        log(std::format("  mask: {} of {} points found no face on their ray", misses, poly.size()));
    }
    SkinMesh me;
    me.node = head.i;
    me.skin = true;
    me.tris = tris;
    for (std::size_t i = 0; i < poly.size(); i++) {
        auto [d, sd] = dirs[i];
        double r = floor[i];
        V3     v = {sd * (kMaskCore[0] + d[0] * r), kMaskCore[1] + d[1] * r, kMaskCore[2] + d[2] * r};
        V3     p = XformPt(inv, v);
        me.b0.push_back(head.i);
        me.p0.push_back(p);
        me.b1.push_back(head.i);
        me.p1.push_back(p);
        me.w0.push_back(1.0);
        me.nv++;
    }
    return me;
}

} // namespace expanded::spirit_cat
