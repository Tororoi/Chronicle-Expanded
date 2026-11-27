#include "expanded/town/camera_sweep.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <utility>

namespace expanded {

namespace {

// Triangles a leaf of the tree holds at the most.
constexpr std::uint32_t kLeaf = 8;

// How near two pieces' bounds come to count as touching.
constexpr float kTouch = 1.0f;

// A triangle is flat when its normal is this near straight up or down.
constexpr double kFlat = 0.7;

// The share of a piece's area that is flat for it to be floor.
constexpr double kFloorShare = 0.6;

// Halvings of the distance a sweep's contact is found to: 2^-30 of the way, well under a millionth of a unit.
constexpr int kSearches = 30;

struct Vec {
    double x, y, z;
};

Vec V(const Point &p) { return {p[0], p[1], p[2]}; }

Vec operator+(Vec a, Vec b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

Vec operator-(Vec a, Vec b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

Vec operator*(Vec a, double s) { return {a.x * s, a.y * s, a.z * s}; }

double Dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec Cross(Vec a, Vec b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

double Length(Vec a) { return std::sqrt(Dot(a, a)); }

// Ericson, Real-Time Collision Detection 5.1.5.
Vec Nearest(Vec p, Vec a, Vec b, Vec c) {
    const Vec    ab = b - a, ac = c - a, ap = p - a;
    const double d1 = Dot(ab, ap), d2 = Dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) {
        return a;
    }
    const Vec    bp = p - b;
    const double d3 = Dot(ab, bp), d4 = Dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) {
        return b;
    }
    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        return a + ab * (d1 / (d1 - d3));
    }
    const Vec    cp = p - c;
    const double d5 = Dot(ab, cp), d6 = Dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) {
        return c;
    }
    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        return a + ac * (d2 / (d2 - d6));
    }
    const double va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    }
    const double denom = 1.0 / (va + vb + vc);
    return a + ab * (vb * denom) + ac * (vc * denom);
}

struct Tri {
    Vec a, b, c;
};

Tri T(const Triangle &t) { return {V(t.a), V(t.b), V(t.c)}; }

double Distance(Vec p, const Tri &t) { return Length(p - Nearest(p, t.a, t.b, t.c)); }

// Whether the segment from + t * along, t in [0, limit], passes through the box grown by radius.
bool SegmentMeetsBox(Vec from, Vec along, const Point &min, const Point &max, double radius, double limit) {
    double       low = 0.0, high = limit;
    const double start[3] = {from.x, from.y, from.z};
    const double step[3] = {along.x, along.y, along.z};
    for (int axis = 0; axis < 3; axis++) {
        const double lo = min[axis] - radius, hi = max[axis] + radius;
        if (std::fabs(step[axis]) < 1e-12) {
            if (start[axis] < lo || start[axis] > hi) {
                return false;
            }
            continue;
        }
        double t0 = (lo - start[axis]) / step[axis];
        double t1 = (hi - start[axis]) / step[axis];
        if (t0 > t1) {
            std::swap(t0, t1);
        }
        low = std::max(low, t0);
        high = std::min(high, t1);
        if (low > high) {
            return false;
        }
    }
    return true;
}

bool PointMeetsBox(Vec p, const Point &min, const Point &max, double radius) {
    return p.x >= min[0] - radius && p.x <= max[0] + radius && p.y >= min[1] - radius && p.y <= max[1] + radius &&
           p.z >= min[2] - radius && p.z <= max[2] + radius;
}

// How far along a sphere goes before touching the triangle, at most limit (limit when it does not).
double SweepTriangle(Vec from, Vec along, double radius, const Tri &t, double limit) {
    const double start = Distance(from, t);
    if (start < radius) {
        // Already touching: it stops the sphere as soon as it would come any nearer, wherever on the triangle that is
        // (a sphere starting beside a curved mesh may move away from its nearest point and still pass through the
        // triangle further along).
        if (start <= 1e-9) {
            return 0.0;
        }
        radius = start * (1.0 - 1e-6);
    }
    // A point's distance from a triangle along a line is convex, so its least is found by trisection and the
    // contact before it by halving.
    double lo = 0.0, hi = limit;
    for (int i = 0; i < 2 * kSearches; i++) {
        const double m1 = lo + (hi - lo) / 3.0, m2 = hi - (hi - lo) / 3.0;
        if (Distance(from + along * m1, t) < Distance(from + along * m2, t)) {
            hi = m2;
        } else {
            lo = m1;
        }
    }
    const double least = 0.5 * (lo + hi);
    if (Distance(from + along * least, t) >= radius) {
        return limit;
    }
    lo = 0.0;
    hi = least;
    for (int i = 0; i < kSearches; i++) {
        const double mid = 0.5 * (lo + hi);
        if (Distance(from + along * mid, t) < radius) {
            hi = mid;
        } else {
            lo = mid;
        }
    }
    return lo;
}

// Where along from + t * along the line meets the triangle (Moller-Trumbore), or limit.
double RayTriangle(Vec from, Vec along, const Tri &t, double limit) {
    const Vec    e1 = t.b - t.a, e2 = t.c - t.a;
    const Vec    p = Cross(along, e2);
    const double det = Dot(e1, p);
    if (std::fabs(det) < 1e-12) {
        return limit;
    }
    const double inv = 1.0 / det;
    const Vec    s = from - t.a;
    const double u = Dot(s, p) * inv;
    if (u < 0.0 || u > 1.0) {
        return limit;
    }
    const Vec    q = Cross(s, e1);
    const double v = Dot(along, q) * inv;
    if (v < 0.0 || u + v > 1.0) {
        return limit;
    }
    const double hit = Dot(e2, q) * inv;
    return hit >= 0.0 && hit < limit ? hit : limit;
}

Point Bound(const Triangle &t, bool most) {
    Point out;
    for (int axis = 0; axis < 3; axis++) {
        out[axis] = most ? std::max({t.a[axis], t.b[axis], t.c[axis]}) : std::min({t.a[axis], t.b[axis], t.c[axis]});
    }
    return out;
}

} // namespace

std::vector<Triangle> KeepLarge(const std::vector<Triangle> &triangles, const Largeness &largeness) {
    const std::vector<bool> large = LargeMask(triangles, largeness);
    std::vector<Triangle>   out;
    out.reserve(triangles.size());
    for (std::size_t i = 0; i < triangles.size(); i++) {
        if (large[i]) {
            out.push_back(triangles[i]);
        }
    }
    return out;
}

std::vector<bool> LargeMask(const std::vector<Triangle> &triangles, const Largeness &largeness) {
    // Union-find over corners welded by position.
    std::vector<std::uint32_t> parent(triangles.size());
    for (std::uint32_t i = 0; i < parent.size(); i++) {
        parent[i] = i;
    }
    auto find = [&](std::uint32_t i) {
        while (parent[i] != i) {
            parent[i] = parent[parent[i]];
            i = parent[i];
        }
        return i;
    };

    struct Key {
        std::int64_t x, y, z;
        bool         operator==(const Key &) const = default;
    };

    struct Hash {
        std::size_t operator()(const Key &k) const {
            return std::hash<std::int64_t>()(k.x * 73856093LL ^ k.y * 19349663LL ^ k.z * 83492791LL);
        }
    };

    std::unordered_map<Key, std::uint32_t, Hash> corners;
    corners.reserve(triangles.size() * 2);
    for (std::uint32_t i = 0; i < triangles.size(); i++) {
        for (const Point *corner : {&triangles[i].a, &triangles[i].b, &triangles[i].c}) {
            const Key key{std::llround((*corner)[0] * 100.0), std::llround((*corner)[1] * 100.0), std::llround((*corner)[2] * 100.0)};
            const auto [at, added] = corners.emplace(key, i);
            if (!added) {
                const std::uint32_t a = find(at->second), b = find(i);
                if (a != b) {
                    parent[a] = b;
                }
            }
        }
    }

    struct Bounds {
        Point  min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        Point  max{-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
        double area = 0.0, flat = 0.0; // all its triangles' area, and that of those facing up or down
    };

    std::unordered_map<std::uint32_t, Bounds> pieces;
    for (std::uint32_t i = 0; i < triangles.size(); i++) {
        Bounds      &bounds = pieces[find(i)];
        const Tri    t = T(triangles[i]);
        const Vec    normal = Cross(t.b - t.a, t.c - t.a);
        const double area = 0.5 * Length(normal);
        bounds.area += area;
        if (area > 0.0 && std::abs(normal.y) >= kFlat * 2.0 * area) {
            bounds.flat += area;
        }
        const Point lo = Bound(triangles[i], false), hi = Bound(triangles[i], true);
        for (int axis = 0; axis < 3; axis++) {
            bounds.min[axis] = std::min(bounds.min[axis], lo[axis]);
            bounds.max[axis] = std::max(bounds.max[axis], hi[axis]);
        }
    }
    // A low piece touching a large one is part of it (the curtain across a tent's door, a step); a narrow tall one
    // (a pillar, an arch's leg) is not, small pieces do not make one another large, and the ground and its broad slabs
    // make nothing on them part of them.
    auto wide_of = [](const Bounds &b) { return std::max(b.max[0] - b.min[0], b.max[2] - b.min[2]); };
    auto alone = [&](const Bounds &b) {
        const float wide = wide_of(b);
        return wide >= largeness.wide && b.max[1] - b.min[1] >= largeness.tall;
    };
    auto touch = [&](const Bounds &a, const Bounds &b) {
        for (int axis = 0; axis < 3; axis++) {
            if (a.max[axis] + kTouch < b.min[axis] || b.max[axis] + kTouch < a.min[axis]) {
                return false;
            }
        }
        return true;
    };
    std::vector<const Bounds *> bodies;
    for (const auto &[root, bounds] : pieces) {
        if (alone(bounds) && wide_of(bounds) < largeness.broad) {
            bodies.push_back(&bounds);
        }
    }
    // Floor: mostly facing up or down and big enough to stand on (the tiles of a plaza, a roof, a step).
    auto                                    floor = [&](const Bounds &b) { return b.flat >= largeness.floor && b.flat >= kFloorShare * b.area; };
    std::unordered_map<std::uint32_t, bool> large;
    for (const auto &[root, bounds] : pieces) {
        bool is_large = wide_of(bounds) >= largeness.broad || alone(bounds) || floor(bounds);
        // A pillar stays a pillar, whatever it holds up (an arch's legs), and a small thing stuck to a body stays small
        // (a lamp on a wall, the caps on a bridge's corners).
        const bool pillar = bounds.max[1] - bounds.min[1] >= largeness.tall;
        const bool tiny = wide_of(bounds) < 0.5f * largeness.wide;
        for (std::size_t i = 0; !is_large && !pillar && !tiny && i < bodies.size(); i++) {
            is_large = touch(bounds, *bodies[i]);
        }
        large[root] = is_large;
    }
    std::vector<bool> out(triangles.size());
    for (std::uint32_t i = 0; i < triangles.size(); i++) {
        out[i] = large[find(i)];
    }
    return out;
}

Point NearestOnTriangle(const Point &p, const Triangle &triangle) {
    const Tri t = T(triangle);
    const Vec q = Nearest(V(p), t.a, t.b, t.c);
    return {static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z)};
}

void SweepMesh::Build(std::vector<Triangle> triangles) {
    triangles_.clear();
    nodes_.clear();
    for (const Triangle &t : triangles) {
        const Tri tri = T(t);
        if (Length(Cross(tri.b - tri.a, tri.c - tri.a)) > 1e-6) {
            triangles_.push_back(t);
        }
    }
    if (triangles_.empty()) {
        min_ = max_ = {};
        return;
    }
    nodes_.reserve(2 * triangles_.size() / kLeaf + 2);
    BuildNode(0, static_cast<std::uint32_t>(triangles_.size()));
    min_ = nodes_[0].min;
    max_ = nodes_[0].max;
}

std::uint32_t SweepMesh::BuildNode(std::uint32_t first, std::uint32_t count) {
    const auto index = static_cast<std::uint32_t>(nodes_.size());
    nodes_.push_back({});
    Point min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Point max{-min[0], -min[1], -min[2]};
    Point centre_min = min, centre_max = max;
    for (std::uint32_t i = first; i < first + count; i++) {
        const Point lo = Bound(triangles_[i], false), hi = Bound(triangles_[i], true);
        for (int axis = 0; axis < 3; axis++) {
            min[axis] = std::min(min[axis], lo[axis]);
            max[axis] = std::max(max[axis], hi[axis]);
            const float centre = 0.5f * (lo[axis] + hi[axis]);
            centre_min[axis] = std::min(centre_min[axis], centre);
            centre_max[axis] = std::max(centre_max[axis], centre);
        }
    }
    nodes_[index].min = min;
    nodes_[index].max = max;
    if (count <= kLeaf) {
        nodes_[index].first = first;
        nodes_[index].count = count;
        return index;
    }
    int axis = 0;
    for (int a = 1; a < 3; a++) {
        if (centre_max[a] - centre_min[a] > centre_max[axis] - centre_min[axis]) {
            axis = a;
        }
    }
    const std::uint32_t half = count / 2;
    auto                centre = [axis](const Triangle &t) { return t.a[axis] + t.b[axis] + t.c[axis]; };
    std::nth_element(triangles_.begin() + first, triangles_.begin() + first + half, triangles_.begin() + first + count,
                     [&](const Triangle &l, const Triangle &r) { return centre(l) < centre(r); });
    BuildNode(first, half);
    const std::uint32_t second = BuildNode(first + half, count - half);
    nodes_[index].first = second;
    nodes_[index].count = 0;
    return index;
}

float SweepMesh::Sweep(const Point &from, const Point &to, float radius, float limit) const {
    if (nodes_.empty()) {
        return limit;
    }
    const Vec     start = V(from);
    const Vec     along = V(to) - start;
    double        best = limit;
    std::uint32_t stack[64];
    int           depth = 0;
    stack[depth++] = 0;
    while (depth > 0) {
        const Node &node = nodes_[stack[--depth]];
        if (!SegmentMeetsBox(start, along, node.min, node.max, radius, best)) {
            continue;
        }
        if (node.count == 0) {
            const auto self = static_cast<std::uint32_t>(&node - nodes_.data());
            stack[depth++] = node.first;
            stack[depth++] = self + 1;
            continue;
        }
        for (std::uint32_t i = node.first; i < node.first + node.count; i++) {
            const Triangle &t = triangles_[i];
            if (!SegmentMeetsBox(start, along, Bound(t, false), Bound(t, true), radius, best)) {
                continue;
            }
            best = std::min(best, SweepTriangle(start, along, radius, T(t), best));
        }
    }
    return static_cast<float>(best);
}

Point SweepMesh::PushOut(const Point &point, float radius, float most) const {
    Vec       p = V(point);
    const Vec origin = p;
    for (int pass = 0; pass < 4 && !nodes_.empty(); pass++) {
        double        nearest = radius;
        Vec           push{};
        std::uint32_t stack[64];
        int           depth = 0;
        stack[depth++] = 0;
        while (depth > 0) {
            const Node &node = nodes_[stack[--depth]];
            if (!PointMeetsBox(p, node.min, node.max, radius)) {
                continue;
            }
            if (node.count == 0) {
                const auto self = static_cast<std::uint32_t>(&node - nodes_.data());
                stack[depth++] = node.first;
                stack[depth++] = self + 1;
                continue;
            }
            for (std::uint32_t i = node.first; i < node.first + node.count; i++) {
                const Tri    t = T(triangles_[i]);
                const Vec    q = Nearest(p, t.a, t.b, t.c);
                const double d = Length(p - q);
                if (d < nearest && d > 1e-9) {
                    nearest = d;
                    push = (p - q) * ((radius - d) / d);
                }
            }
        }
        if (nearest >= radius) {
            break;
        }
        p = p + push;
    }
    Vec moved = p - origin;
    if (const double length = Length(moved); length > most) {
        moved = moved * (most / length);
    }
    p = origin + moved;
    return {static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)};
}

float SweepMesh::Ray(const Point &from, const Point &to, float limit) const {
    if (nodes_.empty()) {
        return limit;
    }
    const Vec     start = V(from);
    const Vec     along = V(to) - start;
    double        best = limit;
    std::uint32_t stack[64];
    int           depth = 0;
    stack[depth++] = 0;
    while (depth > 0) {
        const Node &node = nodes_[stack[--depth]];
        if (!SegmentMeetsBox(start, along, node.min, node.max, 0.0, best)) {
            continue;
        }
        if (node.count == 0) {
            const auto self = static_cast<std::uint32_t>(&node - nodes_.data());
            stack[depth++] = node.first;
            stack[depth++] = self + 1;
            continue;
        }
        for (std::uint32_t i = node.first; i < node.first + node.count; i++) {
            best = std::min(best, RayTriangle(start, along, T(triangles_[i]), best));
        }
    }
    return static_cast<float>(best);
}

bool SweepMesh::Touches(const Point &point, float radius) const {
    if (nodes_.empty()) {
        return false;
    }
    const Vec     p = V(point);
    std::uint32_t stack[64];
    int           depth = 0;
    stack[depth++] = 0;
    while (depth > 0) {
        const Node &node = nodes_[stack[--depth]];
        if (!PointMeetsBox(p, node.min, node.max, radius)) {
            continue;
        }
        if (node.count == 0) {
            const auto self = static_cast<std::uint32_t>(&node - nodes_.data());
            stack[depth++] = node.first;
            stack[depth++] = self + 1;
            continue;
        }
        for (std::uint32_t i = node.first; i < node.first + node.count; i++) {
            if (Distance(p, T(triangles_[i])) < radius) {
                return true;
            }
        }
    }
    return false;
}

void SweepMesh::Push(const Point &point, float radius, Point &push) const {
    if (nodes_.empty()) {
        return;
    }
    const Vec     p = V(point);
    std::uint32_t stack[64];
    int           depth = 0;
    stack[depth++] = 0;
    while (depth > 0) {
        const Node &node = nodes_[stack[--depth]];
        if (!PointMeetsBox(p, node.min, node.max, radius)) {
            continue;
        }
        if (node.count == 0) {
            const auto self = static_cast<std::uint32_t>(&node - nodes_.data());
            stack[depth++] = node.first;
            stack[depth++] = self + 1;
            continue;
        }
        for (std::uint32_t i = node.first; i < node.first + node.count; i++) {
            const Tri    t = T(triangles_[i]);
            const Vec    away = p - Nearest(p, t.a, t.b, t.c);
            const double d = Length(away);
            if (d < radius && d > 1e-9) {
                const double weight = (radius - d) / d;
                push[0] += static_cast<float>(away.x * weight);
                push[1] += static_cast<float>(away.y * weight);
                push[2] += static_cast<float>(away.z * weight);
            }
        }
    }
}

float SweepMesh::Closest(const Point &point, float radius, Point &nearest) const {
    double best = radius;
    if (nodes_.empty()) {
        return radius;
    }
    const Vec     p = V(point);
    std::uint32_t stack[64];
    int           depth = 0;
    stack[depth++] = 0;
    while (depth > 0) {
        const Node &node = nodes_[stack[--depth]];
        if (!PointMeetsBox(p, node.min, node.max, best)) {
            continue;
        }
        if (node.count == 0) {
            const auto self = static_cast<std::uint32_t>(&node - nodes_.data());
            stack[depth++] = node.first;
            stack[depth++] = self + 1;
            continue;
        }
        for (std::uint32_t i = node.first; i < node.first + node.count; i++) {
            const Tri    t = T(triangles_[i]);
            const Vec    q = Nearest(p, t.a, t.b, t.c);
            const double d = Length(p - q);
            if (d < best) {
                best = d;
                nearest = {static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z)};
            }
        }
    }
    return static_cast<float>(best);
}

} // namespace expanded
