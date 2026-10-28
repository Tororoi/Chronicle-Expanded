#include "expanded/data/yellow_drops_bank.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/fishing_collision.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/rig_math.hpp"

namespace expanded {

namespace {

using scene::Triangle;
using Point = std::array<double, 3>;
using Chain = std::vector<Point>;
using Matrix = std::array<double, 16>; // column-major, as stored

constexpr double      kBulge = 70.0;
constexpr double      kSpanFrom = -145.0, kSpanTo = 274.0;
constexpr int         kSubdivisions = 1;       // stations added between each two
constexpr double      kFishWallBottom = -24.0; // under the fish
constexpr double      kLocalY = 210.0;         // s1301's frames sit 210 over the world
constexpr double      kTolerance = 0.35;
constexpr std::size_t kDiscS1301Size = 0x4CA50; // the block offsets below are the disc's s1301's
constexpr std::size_t kVisual = 0xB20, kVisualSize = 0x39AF0, kCrown = 0x3A610, kCrownSize = 0x9BD0, kCamera = 0x441E0,
                      kCameraSize = 0x8870;
// s1302's water sheet: its frame's Y translation.
constexpr std::size_t kWaterSheetY = 0x244;

// The west bank's stations, north-west to south-east, in the world.
constexpr std::array<Point, 5> kEdgeTop = {
    {{-426.0, 23.0, -142.0}, {-424.0, 23.0, -23.0}, {-413.0, 23.0, 102.0}, {-388.0, 23.0, 206.0}, {-382.0, 23.0, 275.0}}
};
constexpr std::array<Point, 5> kEdgeBottom = {
    {{-427.0, -10.0, -142.0}, {-424.0, -10.0, -23.0}, {-413.0, -10.0, 102.0}, {-388.0, -10.0, 206.0}, {-382.0, -10.0, 275.0}}
};
constexpr std::array<Point, 5> kCrownRow = {
    {{-420.0, 30.0, -148.0}, {-417.0, 30.0, -24.0}, {-404.0, 30.0, 103.0}, {-378.0, 30.0, 206.0}, {-369.0, 30.0, 273.0}}
};
constexpr std::array<Point, 5> kCameraRow = {
    {{-441.0, 30.0, -134.0}, {-441.0, 30.0, -22.0}, {-430.0, 30.0, 103.0}, {-405.0, 30.0, 208.0}, {-398.0, 30.0, 275.0}}
};

// How far west the bank moves at z.
double Bulge(double z) {
    if (!(kSpanFrom < z && z < kSpanTo)) {
        return 0.0;
    }
    return -kBulge * std::sin(std::numbers::pi * (z - kSpanFrom) / (kSpanTo - kSpanFrom));
}

// The row's stations from z lo to hi, in s1301's frames.
Chain Stations(const std::array<Point, 5> &row, double lo, double hi) {
    Chain chain;
    for (const Point &p : row) {
        if (lo <= p[2] && p[2] <= hi) {
            chain.push_back({p[0], p[1] + kLocalY, p[2]});
        }
    }
    return chain;
}

// The same stations at another height.
Chain AtHeight(const std::array<Point, 5> &row, double y) {
    Chain chain;
    for (const Point &p : row) {
        chain.push_back({p[0], y, p[2]});
    }
    return chain;
}

std::vector<Chain> Grid10() {
    return {Stations(kEdgeTop, -150, 0), Stations(kEdgeBottom, -150, 0), Stations(kCrownRow, -150, 0)};
}

std::vector<Chain> Grid11() {
    return {Stations(kEdgeTop, -30, 280), Stations(kEdgeBottom, -30, 280), Stations(kCrownRow, -30, 280)};
}

Point Lerp(const Point &a, const Point &b, int k) {
    double t = k / (kSubdivisions + 1.0);
    return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
}

std::vector<std::pair<Point, Point>> Segments(const std::vector<Chain> &chains) {
    std::vector<std::pair<Point, Point>> segments;
    for (const Chain &chain : chains) {
        for (std::size_t i = 0; i + 1 < chain.size(); i++) {
            segments.emplace_back(chain[i], chain[i + 1]);
        }
    }
    return segments;
}

template <typename P>
bool AtStation(const P &p, const Point &s) {
    return std::abs(p[0] - s[0]) <= kTolerance && std::abs(p[1] - s[1]) <= 1.0 && std::abs(p[2] - s[2]) <= kTolerance;
}

template <typename P>
bool OnSegment(const std::vector<std::pair<Point, Point>> &segments, const P &a, const P &b) {
    for (const auto &[s, e] : segments) {
        if ((AtStation(a, s) && AtStation(b, e)) || (AtStation(a, e) && AtStation(b, s))) {
            return true;
        }
    }
    return false;
}

// Every station and every one added between them: the columns that move.
std::vector<Point> Columns(const std::vector<Chain> &chains) {
    std::vector<Point> columns;
    for (const Chain &chain : chains) {
        columns.insert(columns.end(), chain.begin(), chain.end());
        for (std::size_t i = 0; i + 1 < chain.size(); i++) {
            for (int k = 1; k <= kSubdivisions; k++) {
                columns.push_back(Lerp(chain[i], chain[i + 1], k));
            }
        }
    }
    return columns;
}

template <typename P>
void MoveColumns(std::vector<P> &points, const std::vector<Chain> &chains) {
    std::vector<Point> columns = Columns(chains);
    for (P &p : points) {
        for (const Point &c : columns) {
            if (AtStation(p, c)) {
                p[0] += Bulge(p[2]);
                break;
            }
        }
    }
}

// A visual mesh with every edge between two neighbouring stations split at its middle (its attributes lerped), then
// the columns moved; laid as triangle lists, one a material, every block 16-aligned.
Bytes BulgeVisual(std::span<const unsigned char> mdt, const std::vector<Chain> &chains) {
    MdtMesh m = MdtMesh::Parse(mdt);

    struct Tri {
        int                             material;
        std::array<std::vector<int>, 3> records;
    };

    std::vector<Tri> tris;
    for (const MdtMesh::Submesh &sub : m.submeshes) {
        const auto &r = sub.records;
        if (sub.prim == 3) {
            for (std::size_t k = 0; k + 2 < r.size(); k += 3) {
                tris.push_back({
                    sub.material, {r[k], r[k + 1], r[k + 2]}
                });
            }
        } else if (sub.prim == 4) {
            for (std::size_t i = 0; i + 2 < r.size(); i++) {
                const std::vector<int> &a = i % 2 == 0 ? r[i] : r[i + 1], &b = i % 2 == 0 ? r[i + 1] : r[i], &c = r[i + 2];
                if (a[0] == b[0] || b[0] == c[0] || a[0] == c[0]) {
                    continue;
                }
                tris.push_back({
                    sub.material, {a, b, c}
                });
            }
        } else {
            Bad(std::format("an MDT submesh of kind {}", sub.prim));
        }
    }
    std::vector<std::pair<Point, Point>>                          segments = Segments(chains);
    std::map<std::tuple<int, int, double>, int>                   middles;
    std::map<std::tuple<int, int, double, std::string_view>, int> lerped;
    auto                                                          block = [&](std::string_view name) -> std::vector<std::array<double, 4>>                                                          &{
        // The mod's UV block is the one MdtMesh calls normals, its NORM block the one it calls uvs.
        return name == "uv" ? m.normals : name == "norm" ? m.uvs
                                                                                                                  : m.colour_values;
    };
    auto lerp_entry = [&](std::string_view name, int ia, int ib, double t) {
        auto key = std::make_tuple(std::min(ia, ib), std::max(ia, ib), Round(t, 4), name);
        if (auto found = lerped.find(key); found != lerped.end()) {
            return found->second;
        }
        auto                 &list = block(name);
        std::array<double, 4> va = list[static_cast<std::size_t>(ia)], vb = list[static_cast<std::size_t>(ib)], v{};
        double                t2 = ia > ib ? 1.0 - t : t;
        for (std::size_t j = 0; j < 4; j++) {
            v[j] = va[j] + (vb[j] - va[j]) * t2;
        }
        list.push_back(v);
        lerped[key] = static_cast<int>(list.size()) - 1;
        return lerped[key];
    };
    auto middle = [&](const std::vector<int> &ra, const std::vector<int> &rb, double t) {
        auto key = std::make_tuple(std::min(ra[0], rb[0]), std::max(ra[0], rb[0]), Round(ra[0] < rb[0] ? t : 1 - t, 4));
        if (!middles.contains(key)) {
            std::array<double, 4> pa = m.positions[static_cast<std::size_t>(ra[0])], pb = m.positions[static_cast<std::size_t>(rb[0])], p{};
            for (std::size_t j = 0; j < 4; j++) {
                p[j] = pa[j] + (pb[j] - pa[j]) * t;
            }
            m.positions.push_back(p);
            middles[key] = static_cast<int>(m.positions.size()) - 1;
        }
        std::vector<int> record = {middles[key], lerp_entry("uv", ra[1], rb[1], t)};
        record.push_back(!m.uvs.empty() ? lerp_entry("norm", ra[2], rb[2], t) : 0);
        if (m.colours) {
            record.push_back(lerp_entry("col", ra[3], rb[3], t));
        }
        return record;
    };
    bool changed = true;
    for (int guard = 0; changed && guard < 30; guard++) {
        changed = false;
        std::vector<Tri> split;
        for (const Tri &tri : tris) {
            bool done = false;
            for (std::size_t e = 0; e < 3; e++) {
                const std::vector<int> &ra = tri.records[e], &rb = tri.records[(e + 1) % 3], &rc = tri.records[(e + 2) % 3];
                if (OnSegment(segments, m.positions[static_cast<std::size_t>(ra[0])], m.positions[static_cast<std::size_t>(rb[0])])) {
                    std::vector<int> mid = middle(ra, rb, 0.5);
                    split.push_back({
                        tri.material, {ra, mid, rc}
                    });
                    split.push_back({
                        tri.material, {mid, rb, rc}
                    });
                    changed = done = true;
                    break;
                }
            }
            if (!done) {
                split.push_back(tri);
            }
        }
        tris = std::move(split);
    }
    MoveColumns(m.positions, chains);

    m.submeshes.clear();
    for (const Tri &tri : tris) {
        auto sub = std::ranges::find(m.submeshes, tri.material, &MdtMesh::Submesh::material);
        if (sub == m.submeshes.end()) {
            m.submeshes.push_back({3, tri.material, {}});
            sub = m.submeshes.end() - 1;
        }
        sub->records.insert(sub->records.end(), tri.records.begin(), tri.records.end());
    }
    m.header[5] = static_cast<std::uint32_t>(m.normals.size());
    m.header[11] = static_cast<std::uint32_t>(m.uvs.size());
    if (m.colours) {
        m.header[7] = static_cast<std::uint32_t>(m.colour_values.size());
    }
    std::size_t display_list = 16;
    for (const MdtMesh::Submesh &sub : m.submeshes) {
        display_list += 12 + sub.records.size() * (m.colours ? 16 : 12);
    }
    std::map<std::string, std::size_t> lengths = {
        {"POS",  16 * m.positions.size()                    },
        {"UV",   16 * m.normals.size()                      },
        {"NORM", 16 * m.uvs.size()                          },
        {"COL",  m.colours ? 16 * m.colour_values.size() : 0},
        {"DL",   display_list                               },
        {"MAT",  m.materials.size() * MdtMesh::kMaterialSize},
    };
    for (const std::string &name : m.order) {
        m.pads[name] = Bytes((16 - lengths[name] % 16) % 16, 0);
    }
    Bytes out = m.Build();
    for (std::size_t word : {4, 6, 10, 12, 14}) {
        if (Get32(out, word * 4) % 16 != 0) {
            Bad("a rebuilt bank mesh's block is not 16-aligned");
        }
    }
    return out;
}

// A collision mesh (positions @0x10, display list @0x28, its triangles @+0x14, five words each from +0x18) so split
// and moved: the new positions and display list after its old bytes.
Bytes BulgeCollision(std::span<const unsigned char> mdt, const std::vector<Chain> &chains) {
    std::size_t                     positions = Get32(mdt, 0x10), list = Get32(mdt, 0x28), total = Get32(mdt, 8);
    std::size_t                     count = Get32(mdt, list + 0x14);
    std::vector<std::array<int, 5>> records(count);
    int                             most = -1;
    for (std::size_t t = 0; t < count; t++) {
        for (std::size_t k = 0; k < 5; k++) {
            records[t][k] = GetI32(mdt, list + 0x18 + t * 0x14 + k * 4);
        }
        most = std::max({most, records[t][0], records[t][1], records[t][2]});
    }
    std::vector<std::array<double, 4>> pos(static_cast<std::size_t>(most + 1));
    for (std::size_t i = 0; i < pos.size(); i++) {
        for (std::size_t k = 0; k < 4; k++) {
            pos[i][k] = GetF32(mdt, positions + i * 0x10 + k * 4);
        }
    }
    std::vector<std::pair<Point, Point>> segments = Segments(chains);
    std::map<std::pair<int, int>, int>   middles;
    auto                                 middle = [&](int ia, int ib) {
        auto key = std::make_pair(std::min(ia, ib), std::max(ia, ib));
        if (!middles.contains(key)) {
            const auto &a = pos[static_cast<std::size_t>(ia)], &b = pos[static_cast<std::size_t>(ib)];
            pos.push_back({(a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2, (a[3] + b[3]) / 2});
            middles[key] = static_cast<int>(pos.size()) - 1;
        }
        return middles[key];
    };
    bool changed = true;
    for (int guard = 0; changed && guard < 30; guard++) {
        changed = false;
        std::vector<std::array<int, 5>> split;
        for (const auto &r : records) {
            bool done = false;
            for (std::size_t e = 0; e < 3; e++) {
                int ia = r[e], ib = r[(e + 1) % 3];
                if (OnSegment(segments, pos[static_cast<std::size_t>(ia)], pos[static_cast<std::size_t>(ib)])) {
                    int  mid = middle(ia, ib);
                    auto r1 = r, r2 = r;
                    r1[(e + 1) % 3] = mid;
                    r2[e] = mid;
                    split.push_back(r1);
                    split.push_back(r2);
                    changed = done = true;
                    break;
                }
            }
            if (!done) {
                split.push_back(r);
            }
        }
        records = std::move(split);
    }
    MoveColumns(pos, chains);

    Bytes out(mdt.begin(), mdt.begin() + static_cast<std::ptrdiff_t>(total));
    PadTo16(out);
    std::size_t new_positions = out.size();
    for (const auto &p : pos) {
        for (double v : p) {
            Append32(out, std::bit_cast<std::uint32_t>(static_cast<float>(v)));
        }
    }
    std::size_t new_list = out.size();
    out.insert(out.end(), mdt.begin() + static_cast<std::ptrdiff_t>(list), mdt.begin() + static_cast<std::ptrdiff_t>(list + 0x18));
    for (const auto &r : records) {
        for (int v : r) {
            Append32(out, static_cast<std::uint32_t>(v));
        }
    }
    PadTo16(out);
    Set32(out, 0x08, static_cast<std::uint32_t>(out.size()));
    Set32(out, 0x0C, static_cast<std::uint32_t>(pos.size()));
    Set32(out, 0x10, static_cast<std::uint32_t>(new_positions));
    Set32(out, 0x28, static_cast<std::uint32_t>(new_list));
    Set32(out, new_list + 0x14, static_cast<std::uint32_t>(records.size()));
    return out;
}

// The .mds of size at offset in sub laid again: its meshes in their order, edited ones (by frame name) replaced, and
// frames added after its own (each a copy of frame 1's record under its name, with its mesh).
Bytes Relay(std::span<const unsigned char> sub, std::size_t offset, std::size_t size, const std::map<std::string, Bytes> &edits,
            const std::vector<std::pair<std::string, Bytes>> &added = {}) {
    std::size_t count = Get32(sub, offset + mds::kCountAt), table = Get32(sub, offset + mds::kTableAt);

    struct Node {
        std::size_t  index;
        std::string  name;
        std::int32_t mesh;
    };

    std::vector<Node> nodes;
    for (std::size_t i = 0; i < count; i++) {
        std::size_t record = offset + table + i * mds::kRecord;
        Node        node{i, NameAt(sub, record + mds::kName, 16), GetI32(sub, record + mds::kMesh)};
        if (node.mesh != 0) {
            nodes.push_back(node);
        }
    }
    std::ranges::stable_sort(nodes, {}, &Node::mesh);
    Bytes out(sub.begin() + static_cast<std::ptrdiff_t>(offset), sub.begin() + static_cast<std::ptrdiff_t>(offset + table + count * mds::kRecord));
    for (std::size_t k = 0; k < added.size(); k++) {
        std::size_t template_record = offset + table + mds::kRecord;
        Bytes       record(sub.begin() + static_cast<std::ptrdiff_t>(template_record), sub.begin() + static_cast<std::ptrdiff_t>(template_record + mds::kRecord));
        Set32(record, 0, static_cast<std::uint32_t>(count + k));
        for (std::size_t c = 0; c < 16; c++) {
            record[mds::kName + c] = c < added[k].first.size() ? static_cast<unsigned char>(added[k].first[c]) : 0;
        }
        out.insert(out.end(), record.begin(), record.end());
    }
    Set32(out, mds::kCountAt, static_cast<std::uint32_t>(count + added.size()));
    std::vector<std::pair<std::size_t, std::size_t>> meshes; // where a record's mesh offset goes, the offset
    for (std::size_t k = 0; k < nodes.size(); k++) {
        std::size_t from = offset + static_cast<std::size_t>(nodes[k].mesh);
        std::size_t to = offset + (k + 1 < nodes.size() ? static_cast<std::size_t>(nodes[k + 1].mesh) : size);
        meshes.emplace_back(table + nodes[k].index * mds::kRecord + mds::kMesh, out.size());
        if (auto edit = edits.find(nodes[k].name); edit != edits.end()) {
            out.insert(out.end(), edit->second.begin(), edit->second.end());
        } else {
            out.insert(out.end(), sub.begin() + static_cast<std::ptrdiff_t>(from), sub.begin() + static_cast<std::ptrdiff_t>(to));
        }
        PadTo16(out);
    }
    for (std::size_t k = 0; k < added.size(); k++) {
        meshes.emplace_back(table + (count + k) * mds::kRecord + mds::kMesh, out.size());
        out.insert(out.end(), added[k].second.begin(), added[k].second.end());
        PadTo16(out);
    }
    for (const auto &[at, mesh] : meshes) {
        Set32(out, at, static_cast<std::uint32_t>(mesh));
    }
    return out;
}

// The mesh of every frame of the .mds at offset holding a station of chains, so bulged.
std::map<std::string, Bytes> BulgeCollisions(std::span<const unsigned char> sub, std::size_t offset, const std::vector<Chain> &chains) {
    std::map<std::string, Bytes> edits;
    std::vector<Point>           stations;
    for (const Chain &chain : chains) {
        stations.insert(stations.end(), chain.begin(), chain.end());
    }
    std::size_t count = Get32(sub, offset + mds::kCountAt), table = Get32(sub, offset + mds::kTableAt);
    for (std::size_t i = 0; i < count; i++) {
        std::size_t  record = offset + table + i * mds::kRecord;
        std::int32_t mesh = GetI32(sub, record + mds::kMesh);
        if (mesh == 0) {
            continue;
        }
        std::size_t at = offset + static_cast<std::size_t>(mesh);
        if (sub[at] != 'M' || sub[at + 1] != 'D' || sub[at + 2] != 'T') {
            continue;
        }
        std::span<const unsigned char> mdt = sub.subspan(at);
        std::size_t                    positions = Get32(mdt, 0x10), list = Get32(mdt, 0x28), triangles = Get32(mdt, list + 0x14);
        int                            vertices = 0;
        for (std::size_t t = 0; t < triangles; t++) {
            for (std::size_t k = 0; k < 3; k++) {
                vertices = std::max(vertices, GetI32(mdt, list + 0x18 + t * 0x14 + k * 4) + 1);
            }
        }
        bool hit = false;
        for (std::size_t v = 0; v < static_cast<std::size_t>(vertices) && !hit; v++) {
            Point p = {GetF32(mdt, positions + v * 0x10), GetF32(mdt, positions + v * 0x10 + 4), GetF32(mdt, positions + v * 0x10 + 8)};
            hit = std::ranges::any_of(stations, [&](const Point &s) { return AtStation(p, s); });
        }
        if (hit) {
            edits[NameAt(sub, record + mds::kName, 16)] = BulgeCollision(mdt, chains);
        }
    }
    return edits;
}

// The pillars' camera hulls and the dome's camera ring.
constexpr double kPillarPad = 8.0, kHullLow = -10.0, kHullHigh = 130.0, kBandLow = 20.0, kBandHigh = 45.0;
constexpr int    kHullCorners = 8;
constexpr double kDomePull = 20.0, kDomeClear = 4.0, kDomeX = -2.0, kDomeZ = -6.0;

double Length(double x, double y) {
    return std::sqrt(x * x + y * y);
}

// Python's a // b for floats.
double FloorDiv(double a, double b) {
    double mod = std::fmod(a, b);
    double div = (a - mod) / b;
    if (mod != 0 && ((b < 0) != (mod < 0))) {
        div -= 1.0;
    }
    if (div == 0) {
        return std::copysign(0.0, a / b);
    }
    double floored = std::floor(div);
    if (div - floored > 0.5) {
        floored += 1.0;
    }
    return floored;
}

using Point2 = std::array<double, 2>;

// The points split in two across the widest gap of their middle half along their main axis.
std::array<std::vector<Point2>, 2> SplitLegs(const std::vector<Point2> &points) {
    std::size_t n = points.size();
    double      cx = 0, cz = 0, sxx = 0, szz = 0, sxz = 0;
    for (const Point2 &p : points) {
        cx += p[0];
    }
    cx /= static_cast<double>(n);
    for (const Point2 &p : points) {
        cz += p[1];
    }
    cz /= static_cast<double>(n);
    for (const Point2 &p : points) {
        sxx += (p[0] - cx) * (p[0] - cx);
    }
    for (const Point2 &p : points) {
        szz += (p[1] - cz) * (p[1] - cz);
    }
    for (const Point2 &p : points) {
        sxz += (p[0] - cx) * (p[1] - cz);
    }
    double                                 angle = 0.5 * std::atan2(2 * sxz, sxx - szz), ax = std::cos(angle), az = std::sin(angle);
    std::vector<std::pair<double, Point2>> along;
    for (const Point2 &p : points) {
        along.emplace_back(p[0] * ax + p[1] * az, p);
    }
    std::ranges::stable_sort(along, [](const auto &a, const auto &b) {
        return a.first != b.first ? a.first < b.first : a.second[0] != b.second[0] ? a.second[0] < b.second[0]
                                                                                   : a.second[1] < b.second[1];
    });
    std::size_t gap_at = 0;
    double      gap = -1.0;
    for (std::size_t i = n / 4; i < 3 * n / 4; i++) {
        double d = along[i + 1].first - along[i].first;
        if (d > gap) {
            gap = d;
            gap_at = i;
        }
    }
    std::array<std::vector<Point2>, 2> legs;
    for (std::size_t i = 0; i < n; i++) {
        legs[i <= gap_at ? 0 : 1].push_back(along[i].second);
    }
    return legs;
}

std::vector<Point2> Hull(const std::vector<Point2> &points) {
    std::vector<Point2> unique;
    for (const Point2 &p : points) {
        if (std::ranges::none_of(unique, [&](const Point2 &u) { return u[0] == p[0] && u[1] == p[1]; })) {
            unique.push_back(p);
        }
    }
    std::ranges::stable_sort(unique, [](const Point2 &a, const Point2 &b) { return a[0] != b[0] ? a[0] < b[0] : a[1] < b[1]; });
    if (unique.size() < 3) {
        return unique;
    }
    auto half = [](auto first, auto last) {
        std::vector<Point2> h;
        for (auto it = first; it != last; ++it) {
            const Point2 &p = *it;
            while (h.size() >= 2 && (h[h.size() - 1][0] - h[h.size() - 2][0]) * (p[1] - h[h.size() - 2][1]) -
                                            (h[h.size() - 1][1] - h[h.size() - 2][1]) * (p[0] - h[h.size() - 2][0]) <=
                                        0) {
                h.pop_back();
            }
            h.push_back(p);
        }
        return h;
    };
    std::vector<Point2> lower = half(unique.begin(), unique.end()), upper = half(unique.rbegin(), unique.rend());
    std::vector<Point2> hull(lower.begin(), lower.end() - 1);
    hull.insert(hull.end(), upper.begin(), upper.end() - 1);
    return hull;
}

// The hull's farthest corner in each of n sectors round its middle.
std::vector<Point2> Simplify(const std::vector<Point2> &hull, int n) {
    if (hull.size() <= static_cast<std::size_t>(n)) {
        return hull;
    }
    double cx = 0, cz = 0;
    for (const Point2 &p : hull) {
        cx += p[0];
    }
    cx /= static_cast<double>(hull.size());
    for (const Point2 &p : hull) {
        cz += p[1];
    }
    cz /= static_cast<double>(hull.size());
    std::map<int, std::pair<double, Point2>> sectors;
    for (const Point2 &p : hull) {
        int    sector = Mod(static_cast<int>((std::atan2(p[1] - cz, p[0] - cx) + std::numbers::pi) / (2 * std::numbers::pi) * n), n);
        double r = Length(p[0] - cx, p[1] - cz);
        if (!sectors.contains(sector) || r > sectors[sector].first) {
            sectors[sector] = {r, p};
        }
    }
    std::vector<Point2> out;
    for (const auto &[sector, corner] : sectors) {
        out.push_back(corner.second);
    }
    return out;
}

// One Chaikin pass over a closed polygon.
std::vector<Point2> Chaikin(const std::vector<Point2> &polygon) {
    std::vector<Point2> out;
    for (std::size_t i = 0; i < polygon.size(); i++) {
        const Point2 &a = polygon[i], &b = polygon[(i + 1) % polygon.size()];
        out.push_back({0.75 * a[0] + 0.25 * b[0], 0.75 * a[1] + 0.25 * b[1]});
        out.push_back({0.25 * a[0] + 0.75 * b[0], 0.25 * a[1] + 0.75 * b[1]});
    }
    return out;
}

std::vector<Triangle> Walls(const std::vector<std::vector<Point2>> &feet) {
    std::vector<Triangle> tris;
    for (const std::vector<Point2> &foot : feet) {
        std::size_t n = foot.size();
        double      area2 = 0;
        for (std::size_t i = 0; i < n; i++) {
            area2 += foot[i][0] * foot[(i + 1) % n][1] - foot[(i + 1) % n][0] * foot[i][1];
        }
        std::vector<Point2> loop = foot;
        if (!(area2 < 0)) {
            std::ranges::reverse(loop);
        }
        for (std::size_t i = 0; i < n; i++) {
            const Point2 &a = loop[i], &b = loop[(i + 1) % n];
            tris.push_back({
                {{a[0], kHullLow, a[1]}, {b[0], kHullLow, b[1]}, {b[0], kHullHigh, b[1]}}
            });
            tris.push_back({
                {{a[0], kHullLow, a[1]}, {b[0], kHullHigh, b[1]}, {a[0], kHullHigh, a[1]}}
            });
        }
    }
    return tris;
}

// A pillar's camera hull: its two legs' footprints at the bank's band, padded out and rounded, as walls.
std::vector<Triangle> PillarHull(const std::vector<scene::PlacedMesh> &placed, std::string_view sub) {
    std::vector<Point2> points;
    for (const scene::PlacedMesh &mesh : placed) {
        if (mesh.sub != sub || mesh.instance != 0 || !mesh.frame.starts_with("extru")) {
            continue;
        }
        for (const scene::Vec3 &v : mesh.vertices) {
            if (kBandLow <= v[1] && v[1] <= kBandHigh) {
                points.push_back({Round(v[0], 2), Round(v[2], 2)});
            }
        }
    }
    std::vector<std::vector<Point2>> feet;
    for (const std::vector<Point2> &leg : SplitLegs(points)) {
        std::vector<Point2> hull = Simplify(Hull(leg), kHullCorners);
        double              cx = 0, cz = 0;
        for (const Point2 &p : hull) {
            cx += p[0];
        }
        cx /= static_cast<double>(hull.size());
        for (const Point2 &p : hull) {
            cz += p[1];
        }
        cz /= static_cast<double>(hull.size());
        std::vector<Point2> padded;
        for (const Point2 &p : hull) {
            double d = Length(p[0] - cx, p[1] - cz);
            if (d == 0) {
                d = 1.0;
            }
            padded.push_back({p[0] + (p[0] - cx) / d * kPillarPad, p[1] + (p[1] - cz) / d * kPillarPad});
        }
        feet.push_back(Chaikin(padded));
    }
    return Walls(feet);
}

// A collision mesh of triangles (corners shared by their 3-decimal rounding), lifted by lift for frames that sit at
// -lift.
Bytes CollisionMesh(const std::vector<Triangle> &tris, double lift) {
    std::map<std::tuple<double, double, double>, int> index;
    std::vector<Point>                                vertices;
    std::vector<std::array<int, 3>>                   corners;
    for (const Triangle &t : tris) {
        std::array<int, 3> tri{};
        for (std::size_t k = 0; k < 3; k++) {
            const scene::Vec3 &p = t[k];
            auto               key = std::make_tuple(Round(p[0], 3), Round(p[1] + lift, 3), Round(p[2], 3));
            auto               found = index.find(key);
            if (found == index.end()) {
                found = index.emplace(key, static_cast<int>(vertices.size())).first;
                vertices.push_back({p[0], p[1] + lift, p[2]});
            }
            tri[k] = found->second;
        }
        corners.push_back(tri);
    }
    std::size_t positions = 0x40, list = (positions + vertices.size() * 16 + 0xF) & ~std::size_t{0xF};
    std::size_t total = (list + 0x18 + corners.size() * 0x14 + 0xF) & ~std::size_t{0xF};
    Bytes       out(total, 0);
    for (std::size_t i = 0; i < vertices.size(); i++) {
        for (std::size_t k = 0; k < 3; k++) {
            SetF32(out, positions + i * 16 + k * 4, static_cast<float>(vertices[i][k]));
        }
        SetF32(out, positions + i * 16 + 12, 1.0f);
    }
    std::size_t at = list;
    for (std::uint32_t word : {0x003400B8u, 0x10u, static_cast<std::uint32_t>(corners.size()), 0x20202020u, 3u,
                               static_cast<std::uint32_t>(corners.size())}) {
        Set32(out, at, word);
        at += 4;
    }
    for (const auto &tri : corners) {
        for (int v : tri) {
            Set32(out, at, static_cast<std::uint32_t>(v));
            at += 4;
        }
        at += 8; // colour 0, pad
    }
    const std::uint32_t header[16] = {0x0054444D, 0x40, static_cast<std::uint32_t>(total), static_cast<std::uint32_t>(vertices.size()),
                                      static_cast<std::uint32_t>(positions), static_cast<std::uint32_t>(2 * corners.size()), 0, 0, 0, 0,
                                      static_cast<std::uint32_t>(list), 0, 0, 1, 0, 0};
    for (std::size_t i = 0; i < 16; i++) {
        Set32(out, i * 4, header[i]);
    }
    return out;
}

// The dome's widest reach in each 10-degree sector (and its neighbours), from its sphere27 mesh.
std::array<double, 36> DomeSectors(const std::vector<scene::PlacedMesh> &placed) {
    std::array<double, 36> sectors{};
    for (const scene::PlacedMesh &mesh : placed) {
        if (mesh.frame != "sphere27") {
            continue;
        }
        for (const scene::Vec3 &v : mesh.vertices) {
            if (!(20.0 <= v[1] && v[1] <= 140.0)) {
                continue;
            }
            double r = Length(v[0] - kDomeX, v[2] - kDomeZ);
            int    i = Mod(static_cast<int>(FloorDiv(std::atan2(v[2] - kDomeZ, v[0] - kDomeX) * (180.0 / std::numbers::pi), 10)), 36);
            for (int j : {i - 1, i, i + 1}) {
                sectors[static_cast<std::size_t>(Mod(j, 36))] = std::max(sectors[static_cast<std::size_t>(Mod(j, 36))], r);
            }
        }
    }
    return sectors;
}

// x, z pulled in toward the dome, at most kDomePull, never inside its reach.
std::pair<double, double> HugDome(const std::array<double, 36> &sectors, double x, double z) {
    double dx = x - kDomeX, dz = z - kDomeZ, r = Length(dx, dz);
    if (r <= 1.0) {
        return {x, z};
    }
    double reach = sectors[static_cast<std::size_t>(Mod(static_cast<int>(FloorDiv(std::atan2(dz, dx) * (180.0 / std::numbers::pi), 10)), 36))] + kDomeClear;
    double r2 = std::max(r - kDomePull, std::min(reach, r));
    return {kDomeX + dx * r2 / r, kDomeZ + dz * r2 / r};
}

Matrix Compose(const Matrix &a, const Matrix &b) {
    Matrix r{};
    for (std::size_t c = 0; c < 4; c++) {
        for (std::size_t row = 0; row < 4; row++) {
            double s = 0;
            for (std::size_t k = 0; k < 4; k++) {
                s += a[k * 4 + row] * b[c * 4 + k];
            }
            r[c * 4 + row] = s;
        }
    }
    return r;
}

// The camera mesh doumu_c's corners hugging the dome, through its frame's world matrix and back.
std::optional<Bytes> HugDomeCamera(std::span<const unsigned char> sub, std::size_t offset, const std::array<double, 36> &sectors) {
    std::size_t         count = Get32(sub, offset + mds::kCountAt), table = Get32(sub, offset + mds::kTableAt);
    std::vector<Matrix> locals(count);
    std::vector<int>    parents(count);
    for (std::size_t i = 0; i < count; i++) {
        std::size_t record = offset + table + i * mds::kRecord;
        for (std::size_t k = 0; k < 16; k++) {
            locals[i][k] = GetF32(sub, record + mds::kRotation + k * 4);
        }
        parents[i] = GetI32(sub, record + mds::kParent);
    }
    std::vector<std::optional<Matrix>> worlds(count);
    auto                               world = [&](auto &self, std::size_t i) -> const Matrix                               &{
        if (!worlds[i]) {
            int parent = parents[i];
            worlds[i] = parent < 0 || static_cast<std::size_t>(parent) >= count ? locals[i] : Compose(self(self, static_cast<std::size_t>(parent)), locals[i]);
        }
        return *worlds[i];
    };
    std::optional<Bytes> hugged;
    for (std::size_t i = 0; i < count; i++) {
        std::size_t  record = offset + table + i * mds::kRecord;
        std::int32_t mesh = GetI32(sub, record + mds::kMesh);
        if (NameAt(sub, record + mds::kName, 16) != "doumu_c" || mesh == 0) {
            continue;
        }
        const Matrix         &m = world(world, i);
        double                a = m[0], b = m[4], c = m[8], d = m[1], e = m[5], f = m[9], g = m[2], h = m[6], k = m[10];
        double                det = a * (e * k - f * h) - b * (d * k - f * g) + c * (d * h - e * g);
        std::array<double, 9> inv = {(e * k - f * h) / det, (c * h - b * k) / det, (b * f - c * e) / det,
                                     (f * g - d * k) / det, (a * k - c * g) / det, (c * d - a * f) / det,
                                     (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det};
        std::size_t           at = offset + static_cast<std::size_t>(mesh);
        std::size_t           total = Get32(sub, at + 8), vertices = Get32(sub, at + 12), positions = Get32(sub, at + 16);
        Bytes                 block(sub.begin() + static_cast<std::ptrdiff_t>(at), sub.begin() + static_cast<std::ptrdiff_t>(at + total));
        for (std::size_t v = 0; v < vertices; v++) {
            std::size_t o = positions + v * 0x10;
            double      vx = GetF32(block, o), vy = GetF32(block, o + 4), vz = GetF32(block, o + 8);
            float       vw = GetF32(block, o + 12);
            double      wx = m[0] * vx + m[4] * vy + m[8] * vz + m[12], wy = m[1] * vx + m[5] * vy + m[9] * vz + m[13],
                   wz = m[2] * vx + m[6] * vy + m[10] * vz + m[14];
            auto [nx, nz] = HugDome(sectors, wx, wz);
            if (std::abs(nx - wx) + std::abs(nz - wz) > 1e-6) {
                double rx = nx - m[12], ry = wy - m[13], rz = nz - m[14];
                double lx = inv[0] * rx + inv[1] * ry + inv[2] * rz, lz = inv[6] * rx + inv[7] * ry + inv[8] * rz;
                SetF32(block, o, static_cast<float>(lx));
                SetF32(block, o + 4, static_cast<float>(vy));
                SetF32(block, o + 8, static_cast<float>(lz));
                SetF32(block, o + 12, vw);
            }
        }
        hugged = std::move(block);
    }
    return hugged;
}

Bytes RebuildS1301(std::span<const unsigned char> scene, std::span<const unsigned char> cfg) {
    std::optional<scene::SubFile> found;
    for (const scene::SubFile &file : scene::SubFiles(scene)) {
        if (file.name == "s1301") {
            found = file;
            break;
        }
    }
    if (!found || found->size != kDiscS1301Size) {
        Bad("s1301 is not the disc's");
    }
    std::span<const unsigned char> sub = Slice(scene, found->offset, found->size, "s1301");
    for (auto [offset, size] : {
             std::pair{kVisual, kVisualSize},
             std::pair{kCrown,  kCrownSize },
             std::pair{kCamera, kCameraSize}
    }) {
        Bytes same = Relay(sub, offset, size, {});
        if (!std::ranges::equal(same, sub.subspan(offset, size))) {
            Bad("s1301's models are not laid as the disc's");
        }
    }

    std::map<std::string, Bytes> visual;
    {
        std::size_t count = Get32(sub, kVisual + mds::kCountAt), table = Get32(sub, kVisual + mds::kTableAt);
        for (std::size_t i = 0; i < count; i++) {
            std::size_t record = kVisual + table + i * mds::kRecord;
            std::string name = NameAt(sub, record + mds::kName, 16);
            if (name != "grid10" && name != "grid11") {
                continue;
            }
            std::size_t mesh = kVisual + static_cast<std::size_t>(GetI32(sub, record + mds::kMesh));
            visual[name] = BulgeVisual(sub.subspan(mesh), name == "grid10" ? Grid10() : Grid11());
        }
    }
    std::map<std::string, Bytes> crown = BulgeCollisions(sub, kCrown, {AtHeight(kCrownRow, 240.0), AtHeight(kCrownRow, 336.0)});
    std::map<std::string, Bytes> camera = BulgeCollisions(sub, kCamera, {AtHeight(kCameraRow, 240.0), AtHeight(kCameraRow, 200.0)});
    if (crown.empty() || camera.empty()) {
        Bad("s1301 has no bank walls");
    }
    std::vector<scene::PlacedMesh> placed = scene::PlacedMeshes(scene, cfg);
    if (std::optional<Bytes> dome = HugDomeCamera(sub, kCamera, DomeSectors(placed))) {
        camera["doumu_c"] = std::move(*dome);
    }
    std::vector<std::pair<std::string, Bytes>> hulls = {
        {"pcam_xs", CollisionMesh(PillarHull(placed, "s1303"), kLocalY)},
        {"pcam_xn", CollisionMesh(PillarHull(placed, "s1304"), kLocalY)},
    };

    Bytes vis = Relay(sub, kVisual, kVisualSize, visual), walls = Relay(sub, kCrown, kCrownSize, crown);
    Bytes cams = Relay(sub, kCamera, kCameraSize, camera, hulls);
    Bytes out(sub.begin(), sub.begin() + static_cast<std::ptrdiff_t>(kVisual));
    out.insert(out.end(), vis.begin(), vis.end());
    out.insert(out.end(), walls.begin(), walls.end());
    out.insert(out.end(), cams.begin(), cams.end());
    std::size_t walls_at = kVisual + vis.size(), cams_at = walls_at + walls.size();
    for (std::size_t field : {0x4C, 0x50, 0x54, 0x78}) {
        Set32(out, field, static_cast<std::uint32_t>(walls_at));
    }
    Set32(out, 0x58, static_cast<std::uint32_t>(vis.size()));
    Set32(out, 0x7C, static_cast<std::uint32_t>(walls.size()));
    for (std::size_t field : {0x90, 0xA8, 0xC0}) {
        Set32(out, field, static_cast<std::uint32_t>(cams_at));
    }
    Set32(out, 0xC4, static_cast<std::uint32_t>(cams.size()));
    return out;
}

} // namespace

Bytes YellowDropsWestBank(std::span<const unsigned char> scene, std::span<const unsigned char> cfg) {
    Bytes raised(scene.begin(), scene.end());
    bool  sheet = false;
    for (const scene::SubFile &file : scene::SubFiles(scene)) {
        if (file.name == "s1302") {
            if (std::abs(GetF32(raised, file.offset + kWaterSheetY)) > 0.001f) {
                Bad("s1302's water sheet is not the disc's");
            }
            SetF32(raised, file.offset + kWaterSheetY, kYellowDropsWater);
            sheet = true;
            break;
        }
    }
    if (!sheet) {
        Bad("the scene has no water sheet s1302");
    }
    return scene::ReplaceSub(raised, "s1301", RebuildS1301(raised, cfg));
}

Bytes YellowDropsWater(std::span<const unsigned char> cfg) {
    std::string text = ToText(cfg);
    for (auto [from, to] : {
             std::pair{"\t\t\t-320, 0, -320,", "\t\t\t-320, 4.25, -320,"},
             std::pair{"\t\t\t320, 0, 320,",   "\t\t\t320, 4.25, 320,"  }
    }) {
        std::string_view f = from, t = to;
        std::size_t      at = text.find(f);
        if (at == std::string::npos) {
            Bad("the mapinfo has no water surface at 0");
        }
        for (; at != std::string::npos; at = text.find(f, at + t.size())) {
            text.replace(at, f.size(), t);
        }
    }
    return ToBytes(text);
}

std::vector<Triangle> YellowDropsBankWalls() {
    std::vector<Point> chain = {
        {-485.0, 16.0, -197.0}
    }; // the shore's corner north-west of the section
    for (std::size_t i = 0; i + 1 < kEdgeTop.size(); i++) {
        chain.push_back(kEdgeTop[i]);
        for (int k = 1; k <= kSubdivisions; k++) {
            chain.push_back(Lerp(kEdgeTop[i], kEdgeTop[i + 1], k));
        }
    }
    chain.push_back(kEdgeTop.back());
    for (std::size_t i = 1; i < chain.size(); i++) {
        chain[i][0] += Bulge(chain[i][2]);
    }
    std::vector<Triangle> walls;
    for (std::size_t i = 0; i + 1 < chain.size(); i++) {
        const Point &a = chain[i], &b = chain[i + 1];
        Point        a_bottom = {a[0], kFishWallBottom, a[2]}, b_bottom = {b[0], kFishWallBottom, b[2]};
        walls.push_back({a, b, b_bottom});
        walls.push_back({a, b_bottom, a_bottom});
    }
    std::vector<Triangle> pillars = YellowDropsPillarBases();
    walls.insert(walls.end(), pillars.begin(), pillars.end());
    return walls;
}

} // namespace expanded
