#include "expanded/data/wolf_steve.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <numbers>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/rig_math.hpp"

namespace expanded {

namespace {

constexpr const char *kSteveMds = "c04w13.mds", *kSteveImg = "c04w13.img", *kSteveBbp = "c04w13.bbp", *kSteveSheet = "c04w13";
constexpr const char *kBody = "c04w13", *kGlove = "c04w11";
constexpr const char *kLambMds = "c01w15.mds", *kLambImg = "c01w15.img", *kWolfMesh = "w15b", *kWolfSheet = "c01w15b";

// One triangle of a mesh: its corners' display-list records (position, normal, texture coordinate) and material, its
// corners in the bind pose's world.
struct Tri {
    int                             material = 0;
    std::array<std::vector<int>, 3> rec;
    std::array<Vec3, 3>             world;

    Vec3 Centroid() const {
        Vec3 c{};
        for (const Vec3 &w : world) {
            for (int i = 0; i < 3; i++) {
                c[std::size_t(i)] += w[std::size_t(i)] / 3;
            }
        }
        return c;
    }
};

// The mesh's triangles (strips with their degenerates dropped), in the frame's bind-pose world.
std::vector<Tri> Triangles(const MdtMesh &m, const MdsFrame &frame) {
    std::vector<Tri> out;
    auto             add = [&](int material, const std::vector<int> &a, const std::vector<int> &b, const std::vector<int> &c) {
        if (a[0] == b[0] || b[0] == c[0] || a[0] == c[0]) {
            return;
        }
        Tri t;
        t.material = material;
        t.rec = {a, b, c};
        for (int k = 0; k < 3; k++) {
            const auto &p = m.positions.at(std::size_t(t.rec[std::size_t(k)][0]));
            t.world[std::size_t(k)] = XformPt(frame.world, {p[0], p[1], p[2]});
        }
        out.push_back(std::move(t));
    };
    for (const MdtMesh::Submesh &sub : m.submeshes) {
        const auto &r = sub.records;
        if (sub.prim == 3) {
            for (std::size_t k = 0; k + 2 < r.size(); k += 3) {
                add(sub.material, r[k], r[k + 1], r[k + 2]);
            }
        } else if (sub.prim == 4) {
            for (std::size_t i = 0; i + 2 < r.size(); i++) {
                i % 2 == 0 ? add(sub.material, r[i], r[i + 1], r[i + 2]) : add(sub.material, r[i + 1], r[i], r[i + 2]);
            }
        } else {
            throw std::runtime_error("an MDT primitive that is neither a list nor a strip");
        }
    }
    return out;
}

std::string Key(const Vec3 &v) {
    char buf[96];
    std::snprintf(buf, sizeof buf, "%.3f,%.3f,%.3f", v[0], v[1], v[2]);
    return buf;
}

// The mesh's connected pieces (triangles sharing a position, or a corner at the same place), each its triangles' indices
// in the order of its first.
std::vector<std::vector<int>> Pieces(const std::vector<Tri> &tris) {
    std::vector<int> parent(tris.size());
    std::iota(parent.begin(), parent.end(), 0);
    auto find = [&](int x) {
        while (parent[std::size_t(x)] != x) {
            parent[std::size_t(x)] = parent[std::size_t(parent[std::size_t(x)])];
            x = parent[std::size_t(x)];
        }
        return x;
    };
    std::map<int, int>         by_position;
    std::map<std::string, int> by_place;
    for (std::size_t t = 0; t < tris.size(); t++) {
        for (int k = 0; k < 3; k++) {
            int pos = tris[t].rec[std::size_t(k)][0];
            if (auto [it, fresh] = by_position.emplace(pos, int(t)); !fresh) {
                parent[std::size_t(find(int(t)))] = find(it->second);
            }
            if (auto [it, fresh] = by_place.emplace(Key(tris[t].world[std::size_t(k)]), int(t)); !fresh) {
                parent[std::size_t(find(int(t)))] = find(it->second);
            }
        }
    }
    std::map<int, std::vector<int>> groups;
    for (std::size_t t = 0; t < tris.size(); t++) {
        groups[find(int(t))].push_back(int(t));
    }
    std::vector<std::vector<int>> out;
    for (auto &[root, members] : groups) {
        out.push_back(std::move(members));
    }
    std::ranges::sort(out, {}, [](const std::vector<int> &p) { return p.front(); });
    return out;
}

struct Box {
    Vec3 lo{1e9, 1e9, 1e9}, hi{-1e9, -1e9, -1e9};

    Vec3 C() const { return {(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2}; }
};

Box Bounds(const std::vector<Tri> &tris, const std::vector<int> &ids) {
    Box b;
    for (int t : ids) {
        for (const Vec3 &v : tris[std::size_t(t)].world) {
            for (std::size_t i = 0; i < 3; i++) {
                b.lo[i] = std::min(b.lo[i], v[i]);
                b.hi[i] = std::max(b.hi[i], v[i]);
            }
        }
    }
    return b;
}

// An 8-bit picture's colour at a texture coordinate.
struct Sheet {
    tim8::Picture picture;

    std::array<int, 3> At(double u, double v) const {
        int x = std::clamp(int(u * picture.width), 0, picture.width - 1), y = std::clamp(int(v * picture.height), 0, picture.height - 1);
        int slot = tim8::Csm1(picture.pixels.at(std::size_t(y * picture.width + x))) * 4;
        return {picture.clut[std::size_t(slot)], picture.clut[std::size_t(slot + 1)], picture.clut[std::size_t(slot + 2)]};
    }
};

Vec2 Uv(const MdtMesh &m, const std::vector<int> &record) {
    const auto &uv = m.uvs.at(std::size_t(record[2]));
    return {uv[0], uv[1]};
}

// The colour at the middle of a triangle's texture.
std::array<int, 3> MiddleColour(const Sheet &sheet, const MdtMesh &m, const Tri &t) {
    Vec2 a = Uv(m, t.rec[0]), b = Uv(m, t.rec[1]), c = Uv(m, t.rec[2]);
    return sheet.At((a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3);
}

// The loops of the triangles' open edges (edges of one triangle), each its corners in order.
std::vector<std::vector<Vec3>> OpenLoops(const std::vector<Tri> &tris, const std::vector<int> &ids) {
    std::map<std::pair<std::string, std::string>, int> count;
    std::map<std::string, Vec3>                        at;
    std::vector<std::string>                           order;
    for (int t : ids) {
        std::array<std::string, 3> k;
        for (int i = 0; i < 3; i++) {
            k[std::size_t(i)] = Key(tris[std::size_t(t)].world[std::size_t(i)]);
            at.emplace(k[std::size_t(i)], tris[std::size_t(t)].world[std::size_t(i)]);
        }
        for (auto [a, b] : {
                 std::pair{0, 1},
                 std::pair{1, 2},
                 std::pair{2, 0}
        }) {
            if (k[std::size_t(a)] != k[std::size_t(b)]) {
                count[std::minmax(k[std::size_t(a)], k[std::size_t(b)])]++;
            }
        }
    }
    std::map<std::string, std::vector<std::string>> adj;
    for (const auto &[edge, n] : count) {
        if (n == 1) {
            if (!adj.contains(edge.first)) {
                order.push_back(edge.first);
            }
            if (!adj.contains(edge.second)) {
                order.push_back(edge.second);
            }
            adj[edge.first].push_back(edge.second);
            adj[edge.second].push_back(edge.first);
        }
    }
    std::vector<std::vector<Vec3>> loops;
    std::map<std::string, bool>    seen;
    for (const std::string &start : order) {
        if (seen[start]) {
            continue;
        }
        std::vector<Vec3> loop{at[start]};
        seen[start] = true;
        std::string prev, cur = start;
        while (true) {
            std::string next;
            for (const std::string &n : adj[cur]) {
                if (n != prev && !seen[n]) {
                    next = n;
                    break;
                }
            }
            if (next.empty()) {
                break;
            }
            prev = cur;
            cur = next;
            loop.push_back(at[cur]);
            seen[cur] = true;
        }
        loops.push_back(std::move(loop));
    }
    return loops;
}

// A new mesh: triangles, a list per material, their corners' positions, normals and texture coordinates each shared where
// they are the same.
struct NewMesh {
    MdtMesh                             m;
    std::map<int, int>                  submesh_of; // material -> submesh
    std::map<std::array<float, 4>, int> positions, normals, uvs;

    // The entry holding v (as the file stores it, in single precision), added when there is none.
    static int Index(std::vector<std::array<double, 4>> &block, std::map<std::array<float, 4>, int> &seen, const std::array<double, 4> &v) {
        std::array<float, 4> key = {float(v[0]), float(v[1]), float(v[2]), float(v[3])};
        auto [it, fresh] = seen.emplace(key, int(block.size()));
        if (fresh) {
            block.push_back(v);
        }
        return it->second;
    }

    explicit NewMesh(const MdtMesh &templ) : m(templ) {
        m.positions.clear(), m.normals.clear(), m.uvs.clear(), m.colour_values.clear(), m.submeshes.clear(), m.materials.clear();
        m.colours = false;
        m.pads.clear();
    }

    int Material(const Bytes &material) {
        m.materials.push_back(material);
        return int(m.materials.size()) - 1;
    }

    void Add(int material, const std::array<Vec3, 3> &pos, const std::array<Vec3, 3> &normal, const std::array<Vec2, 3> &uv) {
        auto [it, fresh] = submesh_of.emplace(material, int(m.submeshes.size()));
        if (fresh) {
            m.submeshes.push_back({3, material, {}});
        }
        for (int k = 0; k < 3; k++) {
            int p = Index(m.positions, positions, {pos[std::size_t(k)][0], pos[std::size_t(k)][1], pos[std::size_t(k)][2], 1.0});
            int n = Index(m.normals, normals, {normal[std::size_t(k)][0], normal[std::size_t(k)][1], normal[std::size_t(k)][2], 1.0});
            int t = Index(m.uvs, uvs, {uv[std::size_t(k)][0], uv[std::size_t(k)][1], 1.0, 0.0});
            m.submeshes[std::size_t(it->second)].records.push_back({p, n, t});
        }
    }

    Bytes Build() {
        m.header[5] = std::uint32_t(m.normals.size());
        m.header[8] = 0;
        m.header[11] = std::uint32_t(m.uvs.size());
        m.AlignDisplayList();
        return m.Build();
    }
};

// A mesh kept to some of its triangles, as lists by material; its vertices (and so its .wgt) as they were.
Bytes Keep(MdtMesh m, const std::vector<Tri> &tris, const std::vector<bool> &kept) {
    std::map<int, std::size_t>    sub;
    std::vector<MdtMesh::Submesh> subs;
    for (std::size_t t = 0; t < tris.size(); t++) {
        if (!kept[t]) {
            continue;
        }
        auto [it, fresh] = sub.emplace(tris[t].material, subs.size());
        if (fresh) {
            subs.push_back({3, tris[t].material, {}});
        }
        for (const auto &r : tris[t].rec) {
            subs[it->second].records.push_back(r);
        }
    }
    m.submeshes = std::move(subs);
    m.AlignDisplayList();
    return m.Build();
}

Vec3 Normal(const MdtMesh &m, const std::vector<int> &record) {
    const auto &n = m.normals.at(std::size_t(record[1]));
    return {n[0], n[1], n[2]};
}

// The sword's frame into Steve's: its face (+y) looks the way Steve's does (+x), the top of its head (+z) stays up.
Vec3 Turn(const Vec3 &v) {
    return {v[1], -v[0], v[2]};
}

Vec3 Sub(const Vec3 &a, const Vec3 &b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

double Sign(double x) {
    return x > 0 ? 1.0 : x < 0 ? -1.0
                               : 0.0;
}

double Lerp(double a0, double a1, double b0, double b1, double x) {
    return b0 + (x - a0) / (a1 - a0) * (b1 - b0);
}

} // namespace

namespace {

// Macho's hat in Steve's body mesh's coordinates: its triangles, each corner's position, normal and texture point.
struct HatTri {
    int                 material;
    std::array<Vec3, 3> p, n;
    std::array<Vec2, 3> uv;
};

std::vector<HatTri> MachoHat(const MdtMesh &head, const std::vector<Tri> &tris) {
    const MachoHatFit &fit = kMachoHatFit;
    auto               hat = [](int material) { return std::ranges::find(kMachoHatMaterials, material) != std::end(kMachoHatMaterials); };
    double             lo[2] = {1e9, 1e9}, hi[2] = {-1e9, -1e9};
    for (const Tri &t : tris) {
        if (!hat(t.material)) {
            continue;
        }
        for (const auto &r : t.rec) {
            const auto &q = head.positions.at(std::size_t(r[0]));
            lo[0] = std::min(lo[0], q[0]), hi[0] = std::max(hi[0], q[0]);
            lo[1] = std::min(lo[1], q[2]), hi[1] = std::max(hi[1], q[2]);
        }
    }
    double cx = (lo[0] + hi[0]) / 2, cz = (lo[1] + hi[1]) / 2;
    // Tilted back about the side (z), then turned about the up (y), as rows: a point (row) times them.
    double a = fit.tilt * std::numbers::pi / 180, b = fit.turn * std::numbers::pi / 180;
    double tilt[3][3] = {
        {std::cos(a),  std::sin(a), 0},
        {-std::sin(a), std::cos(a), 0},
        {0,            0,           1}
    };
    double turn[3][3] = {
        {std::cos(b), 0, -std::sin(b)},
        {0,           1, 0           },
        {std::sin(b), 0, std::cos(b) }
    };
    double m[3][3] = {};
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            for (int k = 0; k < 3; k++) {
                m[r][c] += tilt[r][k] * turn[k][c];
            }
        }
    }
    auto times = [&](const Vec3 &v) {
        Vec3 out{};
        for (int c = 0; c < 3; c++) {
            out[std::size_t(c)] = v[0] * m[0][c] + v[1] * m[1][c] + v[2] * m[2][c];
        }
        return out;
    };
    std::vector<HatTri> out;
    for (const Tri &t : tris) {
        if (!hat(t.material)) {
            continue;
        }
        HatTri h{t.material, {}, {}, {}};
        for (int k = 0; k < 3; k++) {
            const auto &q = head.positions.at(std::size_t(t.rec[std::size_t(k)][0]));
            Vec3        r = times({q[2] - cz, q[1] - kMachoHatSeat, -(q[0] - cx)});
            h.p[std::size_t(k)] = {r[0] * fit.size + fit.forward + kSteveHeadTop[0], r[1] * fit.size + fit.lift + kSteveHeadTop[1],
                                   r[2] * fit.size + fit.side + kSteveHeadTop[2]};
            Vec3 n = Normal(head, t.rec[std::size_t(k)]);
            h.n[std::size_t(k)] = Unit(times({n[2], n[1], -n[0]}));
            h.uv[std::size_t(k)] = Uv(head, t.rec[std::size_t(k)]);
        }
        out.push_back(h);
    }
    return out;
}

} // namespace

namespace {

// Macho's arms, in Steve's body mesh's coordinates (+y up, the face +x; a side's sign is its z's).
Vec3 Pos(const MdtMesh &m, int i) {
    const auto &p = m.positions.at(std::size_t(i));
    return {p[0], p[1], p[2]};
}

// The sword's coordinates into Steve's: its out x to his out z, its up z to his up y, its depth y to his depth x.
Vec3 SwordTurn(const Vec3 &v) {
    return {v[1], v[2], v[0]};
}

bool SteveArm(const MdtMesh &body, int i, double side) {
    Vec3 p = Pos(body, i);
    return side * p[2] > kSteveArmOut && p[0] > kSteveBandBack && std::ranges::find(kSteveBandEnds, i) == std::end(kSteveBandEnds);
}

bool SteveArm(const MdtMesh &body, int i) {
    return SteveArm(body, i, 1.0) || SteveArm(body, i, -1.0);
}

Vec3 Mean(const std::vector<Vec3> &vs) {
    Vec3 c{};
    for (const Vec3 &v : vs) {
        c = Add(c, v);
    }
    return Scale(c, 1.0 / double(vs.size()));
}

// Steve's shoulder, elbow and wrist on a side, as a look moves them.
std::array<Vec3, 3> SteveMarks(const MdtMesh &body, const MdtMesh &glove, double side, const MachoArmsLook &look) {
    std::vector<Vec3> ring, elbow, glove_side;
    for (int i = 0; i < int(body.positions.size()); i++) {
        if (SteveArm(body, i, side)) {
            Vec3 p = Pos(body, i);
            if (p[1] < kSteveElbowFrom) {
                ring.push_back(p);
            } else if (p[1] < kSteveWristFrom) {
                elbow.push_back(p);
            }
        }
    }
    Vec3   ring0 = Mean(ring);
    double width = -1e9;
    for (int i = 0; i < int(body.positions.size()); i++) {
        Vec3 p = Pos(body, i);
        if (!SteveArm(body, i) && std::abs(p[1] - ring0[1]) < 0.15 && side * p[2] > 0) {
            width = std::max(width, side * p[2]);
        }
    }
    double low = 1e9;
    for (int i = 0; i < int(glove.positions.size()); i++) {
        Vec3 p = Pos(glove, i);
        if (side * p[2] > 0) {
            glove_side.push_back(p);
            low = std::min(low, p[1]);
        }
    }
    std::vector<Vec3> opening;
    for (const Vec3 &p : glove_side) {
        if (p[1] < low + 0.15) {
            opening.push_back(p);
        }
    }
    return {
        Vec3{ring0[0], ring0[1] + look.shoulder_up, side * (width + look.shoulder_out)},
        Add(Mean(elbow), {0.0,      look.elbow_up,               side * look.elbow_out             }
        ),
        Mean(opening),
    };
}

// The sword's arm on a side (its fist left off): its triangles and its shoulder, elbow and wrist.
struct SwordArm {
    std::vector<int>    tris;
    std::array<Vec3, 3> marks;
};

SwordArm SwordArmOf(const MdtMesh &sword, const std::vector<Tri> &tris, double side) {
    auto              fist = [&](const Vec3 &p) { return p[2] > kSwordFistFrom && side * p[0] < kSwordFistIn; };
    SwordArm          arm;
    std::vector<Vec3> corners;
    for (int t = 0; t < int(tris.size()); t++) {
        std::array<Vec3, 3> p;
        for (int k = 0; k < 3; k++) {
            p[std::size_t(k)] = Pos(sword, tris[std::size_t(t)].rec[std::size_t(k)][0]);
        }
        Vec3 c = Scale(Add(Add(p[0], p[1]), p[2]), 1.0 / 3);
        if (side * c[0] > kSwordArmOut && kSwordArmLow < c[2] && c[2] < kSwordArmHigh && std::ranges::none_of(p, fist)) {
            arm.tris.push_back(t);
            corners.insert(corners.end(), p.begin(), p.end());
        }
    }
    double in = 1e9, out = -1e9;
    for (const Vec3 &v : corners) {
        in = std::min(in, side * v[0]), out = std::max(out, side * v[0]);
    }
    std::vector<Vec3> shoulder, elbow, wrist;
    for (const Vec3 &v : corners) {
        if (side * v[0] < in + 0.15) {
            shoulder.push_back(v);
        }
        if (side * v[0] > out - 0.25) {
            elbow.push_back(v);
        }
        if (v[2] > kSwordWristFrom) {
            wrist.push_back(v);
        }
    }
    arm.marks = {SwordTurn(Mean(shoulder)), SwordTurn(Mean(elbow)), SwordTurn(Mean(wrist))};
    return arm;
}

// Across a part of the arm line (a to b): its unit normal in the y-z plane.
Vec3 Across(const Vec3 &ab) {
    double l = std::hypot(ab[1], ab[2]);
    return {0.0, -ab[2] / l, ab[1] / l};
}

double Length(const Vec3 &v) {
    return std::sqrt(Dot(v, v));
}

// A point of the sword's arm (turned) as its place on the arm line: the part (0 shoulder to elbow, 1 elbow to wrist), how
// far along it, and its offsets off it (across, in depth x) in that part's lengths.
struct ArmPlace {
    int    part;
    double along, across, depth;
};

ArmPlace PlaceOn(const std::array<Vec3, 3> &marks, const Vec3 &v) {
    ArmPlace best{};
    double   nearest = 1e18;
    for (int k = 0; k < 2; k++) {
        Vec3   a = marks[std::size_t(k)], ab = Sub(marks[std::size_t(k) + 1], a);
        double l = Length(ab);
        double t = Dot(Sub(v, a), ab) / (l * l);
        t = k == 0 ? std::clamp(t, 0.0, 1.0) : std::max(t, 0.0);
        Vec3   off = Sub(v, Add(a, Scale(ab, t)));
        double d = Length(off);
        if (d < nearest) {
            nearest = d;
            best = {k, t, Dot(off, Across(ab)) / l, off[0] / l};
        }
    }
    return best;
}

Vec3 LayOn(const std::array<Vec3, 3> &marks, const ArmPlace &at, double muscle) {
    Vec3   a = marks[std::size_t(at.part)], ab = Sub(marks[std::size_t(at.part) + 1], a);
    double l = Length(ab);
    return Add(Add(a, Scale(ab, at.along)), Scale(Add(Scale(Across(ab), at.across), Vec3{at.depth, 0.0, 0.0}), muscle * l));
}

// The sword's arms laid on Steve, in his sleeve: the drawn vertices (thin), then the thin look's and the buff's.
Bytes MachoArms(const MdtMesh &body, const MdtMesh &glove, const std::vector<Tri> &body_tris, const MdtMesh &sword, const std::vector<Tri> &sword_tris) {
    // His arm's corners' texture points and its material (the one most of its corners have).
    std::map<int, int> arm_uv, arm_material;
    for (const Tri &t : body_tris) {
        for (const auto &r : t.rec) {
            if (SteveArm(body, r[0])) {
                arm_uv[r[0]] = r[2];
                arm_material[t.material]++;
            }
        }
    }
    int material = std::ranges::max_element(arm_material, {}, [](const auto &e) { return e.second; })->first;

    NewMesh             arms(glove);
    int                 mat = arms.Material(body.materials.at(std::size_t(material)));
    std::map<int, Vec3> buff_of; // a drawn vertex's buff place
    for (double side : {1.0, -1.0}) {
        SwordArm            arm = SwordArmOf(sword, sword_tris, side);
        std::array<Vec3, 3> thin = SteveMarks(body, glove, side, kMachoArmsThin), buff = SteveMarks(body, glove, side, kMachoArmsBuff);
        for (int t : arm.tris) {
            const Tri          &tri = sword_tris[std::size_t(t)];
            std::array<Vec3, 3> p, q, n;
            std::array<Vec2, 3> uv;
            for (int k = 0; k < 3; k++) {
                ArmPlace at = PlaceOn(arm.marks, SwordTurn(Pos(sword, tri.rec[std::size_t(k)][0])));
                p[std::size_t(k)] = LayOn(thin, at, kMachoArmsThin.muscle);
                q[std::size_t(k)] = LayOn(buff, at, kMachoArmsBuff.muscle);
                n[std::size_t(k)] = SwordTurn(Normal(sword, tri.rec[std::size_t(k)]));
                auto        near = std::ranges::min_element(arm_uv, {}, [&](const auto &e) {
                    Vec3 d = Sub(Pos(body, e.first), p[std::size_t(k)]);
                    return Dot(d, d);
                });
                const auto &u = body.uvs.at(std::size_t(near->second));
                uv[std::size_t(k)] = {u[0], u[1]};
            }
            arms.Add(mat, p, n, uv);
            for (int k = 0; k < 3; k++) {
                const Vec3 &v = p[std::size_t(k)];
                buff_of[arms.positions.at({float(v[0]), float(v[1]), float(v[2]), 1.0f})] = q[std::size_t(k)];
            }
        }
    }
    std::size_t drawn = arms.m.positions.size();
    for (std::size_t i = 0; i < drawn; i++) {
        arms.m.positions.push_back(arms.m.positions[i]);
    }
    for (std::size_t i = 0; i < drawn; i++) {
        const Vec3 &q = buff_of.at(int(i));
        arms.m.positions.push_back({q[0], q[1], q[2], 1.0});
    }
    return arms.Build();
}

} // namespace

Bytes BuildWolfSteve(std::span<const unsigned char> steve_chr, std::span<const unsigned char> lamb_chr, std::span<const unsigned char> macho_chr,
                     std::span<const unsigned char> macho_sword_chr) {
    const WolfSteveFit &fit = kWolfSteveFit;
    ChrPack             pack = ChrPack::Parse(steve_chr);
    Bytes               mds = pack.Require(kSteveMds).Payload();
    auto                frames = ReadMdsSkeleton(mds);
    const MdsFrame     &body_frame = RequireFrame(frames, kBody, kSteveMds), &glove_frame = RequireFrame(frames, kGlove, kSteveMds);
    MdtMesh             body = MdtMesh::Parse(MdtAt(mds, std::size_t(body_frame.mesh)));
    MdtMesh             glove = MdtMesh::Parse(MdtAt(mds, std::size_t(glove_frame.mesh)));

    ChrPack         lamb = ChrPack::Parse(lamb_chr);
    Bytes           lmds = lamb.Require(kLambMds).Payload();
    auto            lframes = ReadMdsSkeleton(lmds);
    const MdsFrame &wolf_frame = RequireFrame(lframes, kWolfMesh, kLambMds);
    MdtMesh         wolf = MdtMesh::Parse(MdtAt(lmds, std::size_t(wolf_frame.mesh)));

    ImgBank steve_bank = ImgBank::Parse(pack.Require(kSteveImg).Payload());
    ImgBank lamb_bank = ImgBank::Parse(lamb.Require(kLambImg).Payload());
    Sheet   steve_sheet{tim8::Read(steve_bank.Block(kSteveSheet), true)};
    Sheet   wolf_sheet{tim8::Read(lamb_bank.Block(kWolfSheet), true)};

    // Steve: his body (the biggest piece of his body mesh), his feet (its pieces below his knees), his head (the body past
    // the neck), his hair (the head in its yellow); his gloves, each with its green band.
    auto                          bt = Triangles(body, body_frame);
    auto                          bp = Pieces(bt);
    auto                          torso = *std::ranges::max_element(bp, {}, &std::vector<int>::size);
    std::vector<std::vector<int>> feet;
    for (const auto &p : bp) {
        if (Bounds(bt, p).hi[2] < -0.5) {
            feet.push_back(p);
        }
    }
    // His arms: the triangles with a corner on one.
    std::vector<bool> arm(bt.size());
    for (std::size_t t = 0; t < bt.size(); t++) {
        arm[t] = std::ranges::any_of(bt[t].rec, [&](const std::vector<int> &r) { return SteveArm(body, r[0]); });
    }
    std::erase_if(feet, [&](const std::vector<int> &f) { return std::ranges::any_of(f, [&](int t) { return arm[std::size_t(t)]; }); });
    std::vector<int> head, hair;
    for (int t : torso) {
        if (arm[std::size_t(t)]) {
            continue;
        }
        if (bt[std::size_t(t)].Centroid()[2] >= kNeck) {
            head.push_back(t);
            auto [r, g, b] = MiddleColour(steve_sheet, body, bt[std::size_t(t)]);
            if (r > 180 && g > 180 && b < 120) {
                hair.push_back(t);
            }
        }
    }
    auto              gt = Triangles(glove, glove_frame);
    auto              gloves = Pieces(gt);
    std::vector<bool> cuff(gt.size());
    for (std::size_t t = 0; t < gt.size(); t++) {
        auto [r, g, b] = MiddleColour(steve_sheet, glove, gt[t]);
        cuff[t] = g > r + 20 && g > b + 20;
    }

    // The wolf: its head (the biggest piece between the grip and the blade) with its tongue (the small pieces on it), its
    // two paws (below the grip), each with the way its claws (its white) point.
    auto                          wt = Triangles(wolf, wolf_frame);
    auto                          wp = Pieces(wt);
    std::vector<int>              wolf_head;
    std::vector<std::vector<int>> paws;
    std::vector<int>              biggest;
    for (const auto &p : wp) {
        Box b = Bounds(wt, p);
        if (b.hi[2] < -1.0) {
            paws.push_back(p);
        } else if (b.lo[2] > 0.5 && b.hi[2] < 5.0 && p.size() > biggest.size()) {
            biggest = p;
        }
    }
    for (const auto &p : wp) {
        Box b = Bounds(wt, p);
        if (p == biggest || (b.lo[2] > 0.5 && b.hi[2] < 5.0 && p.size() <= 8)) {
            wolf_head.insert(wolf_head.end(), p.begin(), p.end());
        }
    }
    if (paws.size() != 2 || biggest.empty() || feet.size() != 2 || gloves.size() != 2 || hair.empty()) {
        throw std::runtime_error("Super Steve's or the Lamb's Sword's pieces are not as built for");
    }
    std::ranges::sort(paws, {}, [&](const std::vector<int> &p) { return Bounds(wt, p).C()[0]; }); // the sword's -x one first

    // The neck: the middle of the body's ring at the cut.
    Vec3 neck{};
    int  ring = 0;
    for (int t : torso) {
        for (const Vec3 &v : bt[std::size_t(t)].world) {
            if (std::abs(v[2] - kNeckRing) < 0.02) {
                neck[0] += v[0], neck[1] += v[1], ring++;
            }
        }
    }
    neck[0] /= ring, neck[1] /= ring;

    // The head: centred on the neck, its bottom (the collar) on the cut, stretched front to back.
    Box  wh = Bounds(wt, wolf_head);
    auto head_at = [&](const Vec3 &v) {
        Vec3   r = Turn(Sub(v, {wh.C()[0], wh.C()[1], wh.lo[2]}));
        double s = fit.head_scale;
        return Vec3{neck[0] + fit.head_forward + r[0] * s * fit.head_depth, neck[1] + r[1] * s, kNeck + fit.head_lift + r[2] * s};
    };
    auto head_normal = [&](const Vec3 &n) {
        Vec3 r = Turn(n);
        return Unit({r[0] / fit.head_depth, r[1], r[2]});
    };

    NewMesh wolf_parts(glove), wolf_hands(glove), steve_parts(glove), steve_hands(glove);
    int     wolf_mat = wolf_parts.Material(wolf.materials.at(std::size_t(wt[std::size_t(wolf_head.front())].material)));
    int     hair_mat = wolf_parts.Material(body.materials.at(std::size_t(bt[std::size_t(hair.front())].material)));
    Mat4    to_body = RigidInv(body_frame.world), to_glove = RigidInv(glove_frame.world);
    auto    local_point = [](const Mat4 &to, const Vec3 &v) { return XformPt(to, v); };
    auto    local_dir = [](const Mat4 &to, const Vec3 &v) { return Unit(MatVec(Transpose(RotOf(to)), v)); };

    for (int t : wolf_head) {
        const Tri          &tri = wt[std::size_t(t)];
        std::array<Vec3, 3> p, n;
        std::array<Vec2, 3> uv;
        for (int k = 0; k < 3; k++) {
            p[std::size_t(k)] = local_point(to_body, head_at(tri.world[std::size_t(k)]));
            n[std::size_t(k)] = local_dir(to_body, head_normal(MatVec(Transpose(RotOf(wolf_frame.world)), Normal(wolf, tri.rec[std::size_t(k)]))));
            uv[std::size_t(k)] = Uv(wolf, tri.rec[std::size_t(k)]);
        }
        wolf_parts.Add(wolf_mat, p, n, uv);
    }

    // The hair, closing the head's top hole: the four corners of its open edge on the hole's four corners (front, back,
    // sides), the rest stretched to the hole, each of the hole's points between corners joined to them by a triangle.
    std::vector<Vec3> rim;
    {
        auto loops = OpenLoops(wt, wolf_head);
        auto top = std::ranges::max_element(loops, {}, [](const std::vector<Vec3> &l) { return std::ranges::max(l, {}, [](const Vec3 &v) { return v[2]; })[2]; });
        for (const Vec3 &v : *top) {
            rim.push_back(head_at(v));
        }
    }
    auto edge_loops = OpenLoops(bt, hair);
    auto edge = *std::ranges::max_element(edge_loops, {}, &std::vector<Vec3>::size);
    auto pick = [](const std::vector<Vec3> &vs, int axis, bool most) {
        Vec3 best = vs.front();
        for (const Vec3 &v : vs) {
            if (most ? v[std::size_t(axis)] > best[std::size_t(axis)] : v[std::size_t(axis)] <= best[std::size_t(axis)]) {
                best = v;
            }
        }
        return best;
    };

    enum {
        kFront,
        kBack,
        kLeft,
        kRight
    };

    std::array<Vec3, 4>         rc = {pick(rim, 0, true), pick(rim, 0, false), pick(rim, 1, false), pick(rim, 1, true)};
    std::array<Vec3, 4>         hc = {pick(edge, 0, true), pick(edge, 0, false), pick(edge, 1, false), pick(edge, 1, true)};
    std::map<std::string, Vec2> edge_uv;
    for (int t : hair) {
        for (int k = 0; k < 3; k++) {
            edge_uv[Key(bt[std::size_t(t)].world[std::size_t(k)])] = Uv(body, bt[std::size_t(t)].rec[std::size_t(k)]);
        }
    }
    double base = 0, rim_top = 0;
    for (const Vec3 &v : edge) {
        base += v[2] / double(edge.size());
    }
    for (const Vec3 &v : rc) {
        rim_top += v[2] / 4;
    }
    double rise = fit.hair_height * (rc[kRight][1] - rc[kLeft][1]) / (hc[kRight][1] - hc[kLeft][1]);
    auto   hair_at = [&](const Vec3 &v) {
        for (int k = 0; k < 4; k++) {
            if (Key(v) == Key(hc[std::size_t(k)])) {
                return rc[std::size_t(k)];
            }
        }
        return Vec3{Lerp(hc[kBack][0], hc[kFront][0], rc[kBack][0], rc[kFront][0], v[0]), Lerp(hc[kLeft][1], hc[kRight][1], rc[kLeft][1], rc[kRight][1], v[1]),
                    rim_top + (v[2] - base) * rise};
    };
    for (int t : hair) {
        const Tri          &tri = bt[std::size_t(t)];
        std::array<Vec3, 3> p, n;
        std::array<Vec2, 3> uv;
        for (int k = 0; k < 3; k++) {
            p[std::size_t(k)] = local_point(to_body, hair_at(tri.world[std::size_t(k)]));
            n[std::size_t(k)] = Normal(body, tri.rec[std::size_t(k)]);
            uv[std::size_t(k)] = Uv(body, tri.rec[std::size_t(k)]);
        }
        wolf_parts.Add(hair_mat, p, n, uv);
    }
    std::map<std::string, Vec2> corner_uv;
    for (int k = 0; k < 4; k++) {
        corner_uv[Key(rc[std::size_t(k)])] = edge_uv.at(Key(hc[std::size_t(k)]));
    }
    for (std::size_t i = 0; i < rim.size(); i++) {
        const Vec3 &a = rim[(i + rim.size() - 1) % rim.size()], &m = rim[i], &b = rim[(i + 1) % rim.size()];
        if (!corner_uv.contains(Key(a)) || !corner_uv.contains(Key(b)) || corner_uv.contains(Key(m))) {
            continue;
        }
        Vec2 ua = corner_uv[Key(a)], ub = corner_uv[Key(b)];
        Vec3 up = local_dir(to_body, {0, 0, 1});
        wolf_parts.Add(hair_mat, {
                                     local_point(to_body, a), local_point(to_body, m), local_point(to_body, b)
        },
                       {up, up, up}, {ua, Vec2{(ua[0] + ub[0]) / 2, (ua[1] + ub[1]) / 2}, ub});
    }

    // The paws: each foot's hangs from its ankle as it hangs from the pommel (claws down and out, its top face forward),
    // each hand's stands straight up on the green band, claws up.
    auto paw_for = [&](double y) -> const std::vector<int> & { return y >= 0 ? paws[0] : paws[1]; };
    for (const auto &f : feet) {
        Box                     at = Bounds(bt, f);
        const std::vector<int> &paw = paw_for(at.C()[1]);
        Box                     pb = Bounds(wt, paw);
        for (int t : paw) {
            const Tri          &tri = wt[std::size_t(t)];
            std::array<Vec3, 3> p, n;
            std::array<Vec2, 3> uv;
            for (int k = 0; k < 3; k++) {
                Vec3   r = Turn(Sub(tri.world[std::size_t(k)], {pb.C()[0], pb.C()[1], pb.hi[2]}));
                double s = fit.foot_scale;
                p[std::size_t(k)] = local_point(to_body, {at.C()[0] + r[0] * s, at.C()[1] + Sign(at.C()[1]) * fit.foot_out + r[1] * s, at.hi[2] + fit.foot_lift + r[2] * s});
                n[std::size_t(k)] = local_dir(to_body, Turn(MatVec(Transpose(RotOf(wolf_frame.world)), Normal(wolf, tri.rec[std::size_t(k)]))));
                uv[std::size_t(k)] = Uv(wolf, tri.rec[std::size_t(k)]);
            }
            wolf_parts.Add(wolf_mat, p, n, uv);
        }
    }
    int hand_mat = wolf_hands.Material(wolf.materials.at(std::size_t(wt[std::size_t(paws[0].front())].material)));
    for (const auto &g : gloves) {
        std::vector<int> band;
        for (int t : g) {
            if (cuff[std::size_t(t)]) {
                band.push_back(t);
            }
        }
        Box                     cb = Bounds(gt, band);
        const std::vector<int> &paw = paw_for(Bounds(gt, g).C()[1]);
        // The paw turned about y so its claws (from its middle to its white) point straight down.
        Vec3 mid{}, tip{};
        int  corners = 0, claws = 0;
        for (int t : paw) {
            for (const Vec3 &v : wt[std::size_t(t)].world) {
                mid = Add(mid, v), corners++;
            }
            auto c = MiddleColour(wolf_sheet, wolf, wt[std::size_t(t)]);
            if (std::min({c[0], c[1], c[2]}) > 170) {
                tip = Add(tip, wt[std::size_t(t)].Centroid()), claws++;
            }
        }
        mid = Scale(mid, 1.0 / corners);
        tip = Scale(tip, 1.0 / claws);
        double a = std::atan2(tip[0] - mid[0], -(tip[2] - mid[2]));
        auto   straight = [&](const Vec3 &v) {
            Vec3 d = Sub(v, mid);
            return Vec3{d[0] * std::cos(a) + d[2] * std::sin(a), d[1], -d[0] * std::sin(a) + d[2] * std::cos(a)};
        };
        double lo = 1e9;
        for (int t : paw) {
            for (const Vec3 &v : wt[std::size_t(t)].world) {
                lo = std::min(lo, -straight(v)[2]);
            }
        }
        double s = fit.hand_scale;
        for (int t : paw) {
            const Tri          &tri = wt[std::size_t(t)];
            std::array<Vec3, 3> p, n;
            std::array<Vec2, 3> uv;
            for (int k = 0; k < 3; k++) {
                Vec3 w = straight(tri.world[std::size_t(k)]);
                Vec3 r = {w[1], w[0], -w[2]};
                p[std::size_t(k)] = local_point(to_glove, {cb.C()[0] + r[0] * s, cb.C()[1] + Sign(cb.C()[1]) * fit.hand_out + r[1] * s, cb.hi[2] + fit.hand_lift + (r[2] - lo) * s});
                Vec3 nw = MatVec(Transpose(RotOf(wolf_frame.world)), Normal(wolf, tri.rec[std::size_t(k)]));
                Vec3 nr = {nw[0] * std::cos(a) + nw[2] * std::sin(a), nw[1], -nw[0] * std::sin(a) + nw[2] * std::cos(a)};
                n[std::size_t(k)] = local_dir(to_glove, {nr[1], nr[0], -nr[2]});
                uv[std::size_t(k)] = Uv(wolf, tri.rec[std::size_t(k)]);
            }
            wolf_hands.Add(hand_mat, p, n, uv);
        }
    }

    // Steve's head and feet, and his white hands, as frames of their own: the same corners, in the same frames' space.
    std::vector<bool> body_kept(bt.size(), true), glove_kept(gt.size(), true);
    auto              moved = [](NewMesh &into, std::map<int, int> &mats, const MdtMesh &from, const Tri &tri) {
        auto [it, fresh] = mats.emplace(tri.material, 0);
        if (fresh) {
            it->second = into.Material(from.materials.at(std::size_t(tri.material)));
        }
        std::array<Vec3, 3> p, n;
        std::array<Vec2, 3> uv;
        for (int k = 0; k < 3; k++) {
            const auto &q = from.positions.at(std::size_t(tri.rec[std::size_t(k)][0]));
            p[std::size_t(k)] = {q[0], q[1], q[2]};
            n[std::size_t(k)] = Normal(from, tri.rec[std::size_t(k)]);
            uv[std::size_t(k)] = Uv(from, tri.rec[std::size_t(k)]);
        }
        into.Add(it->second, p, n, uv);
    };
    std::map<int, int> steve_mats, hand_mats, hair_mats;
    NewMesh            steve_hair(glove);
    std::vector<int>   off;
    for (int t : head) {
        if (std::ranges::find(hair, t) == hair.end()) {
            off.push_back(t);
        }
    }
    for (int t : hair) {
        body_kept[std::size_t(t)] = false;
        moved(steve_hair, hair_mats, body, bt[std::size_t(t)]);
    }
    for (const auto &f : feet) {
        off.insert(off.end(), f.begin(), f.end());
    }
    for (int t : off) {
        body_kept[std::size_t(t)] = false;
        moved(steve_parts, steve_mats, body, bt[std::size_t(t)]);
    }
    for (std::size_t t = 0; t < gt.size(); t++) {
        if (!cuff[t]) {
            glove_kept[t] = false;
            moved(steve_hands, hand_mats, glove, gt[t]);
        }
    }
    std::map<int, int> arm_mats;
    NewMesh            steve_arms(glove);
    for (std::size_t t = 0; t < bt.size(); t++) {
        if (arm[t]) {
            body_kept[t] = false;
            moved(steve_arms, arm_mats, body, bt[t]);
        }
    }

    // The Macho Sword's arms, in his sleeve.
    ChrPack         macho_sword = ChrPack::Parse(macho_sword_chr);
    Bytes           smds = macho_sword.Require(kMachoSwordMds).Payload();
    auto            sframes = ReadMdsSkeleton(smds);
    const MdsFrame &sword_frame = RequireFrame(sframes, kMachoSwordMesh, kMachoSwordMds);
    MdtMesh         sword = MdtMesh::Parse(MdtAt(smds, std::size_t(sword_frame.mesh)));
    Bytes           macho_arms = MachoArms(body, glove, bt, sword, Triangles(sword, sword_frame));

    // Macho's hat, its materials his.
    ChrPack            macho = ChrPack::Parse(macho_chr);
    Bytes              mmds = macho.Require(kMachoMds).Payload();
    auto               mframes = ReadMdsSkeleton(mmds);
    const MdsFrame    &mhead_frame = RequireFrame(mframes, kMachoHead, kMachoMds);
    MdtMesh            mhead = MdtMesh::Parse(MdtAt(mmds, std::size_t(mhead_frame.mesh)));
    ImgBank            macho_bank = ImgBank::Parse(macho.Require(kMachoImg).Payload());
    NewMesh            hat_mesh(glove);
    std::map<int, int> hat_mats;
    for (const HatTri &h : MachoHat(mhead, Triangles(mhead, mhead_frame))) {
        auto [it, fresh] = hat_mats.try_emplace(h.material, 0);
        if (fresh) {
            it->second = hat_mesh.Material(mhead.materials.at(std::size_t(h.material)));
        }
        hat_mesh.Add(it->second, h.p, h.n, h.uv);
    }

    // The frames: Steve's and the wolf's on his body and glove frames, rest-posed there.
    int n = int(frames.size());
    pack.Require(kSteveMds).ReplacePayload(ReplaceMeshes(mds, {
                                                                  {kBody,  Keep(body,  bt, body_kept) },
                                                                  {kGlove, Keep(glove, gt, glove_kept)}
    },
                                                         {
                                                             {FrameRecord(n, kSteveParts, body_frame.index), steve_parts.Build()},
                                                             {FrameRecord(n + 1, kSteveHands, glove_frame.index), steve_hands.Build()},
                                                             {FrameRecord(n + 2, kWolfParts, body_frame.index), wolf_parts.Build()},
                                                             {FrameRecord(n + 3, kWolfHands, glove_frame.index), wolf_hands.Build()},
                                                             {FrameRecord(n + 4, kSteveHair, body_frame.index), steve_hair.Build()},
                                                             {FrameRecord(n + 5, kMachoHat, body_frame.index), hat_mesh.Build()},
                                                             {FrameRecord(n + 6, kSteveArms, body_frame.index), steve_arms.Build()},
                                                             {FrameRecord(n + 7, kMachoArms, body_frame.index), macho_arms},
                                                         }));
    AppendBindPoses(pack, kSteveBbp, 8);
    // The Macho arms' vertices kept, for the game to lay them between their looks.
    Bytes             cfg = pack.Require(kSteveCfg).Payload();
    std::string       text(cfg.begin(), cfg.end());
    const std::string body_buffer = std::string("ALLOC_DBUFF \"") + kBody + "\"";
    std::size_t       at = text.find(body_buffer);
    if (at == std::string::npos) {
        throw std::runtime_error(std::string(kSteveCfg) + " has no " + body_buffer);
    }
    text.insert(at + body_buffer.size(), std::string("\r\nALLOC_DBUFF \"") + kMachoArms + "\"");
    pack.Require(kSteveCfg).ReplacePayload(Bytes(text.begin(), text.end()));
    steve_bank.entries.emplace_back(kWolfSheet, lamb_bank.Block(kWolfSheet));
    steve_bank.entries.emplace_back(kMachoHatSheet, tim8::Swizzled(macho_bank.Block(kMachoHatSheet)));
    pack.Require(kSteveImg).ReplacePayload(steve_bank.Build());
    return pack.Rebuild();
}

std::vector<DataBuild> WolfSteveBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Wolf Steve", std::string(kRebuiltDiscFolder) + kSuperSteveFile,
                               {kSuperSteveFile, kLambSwordFile, kMachoFile, kMachoSwordFile}, [](Files s) { return BuildWolfSteve(s[0], s[1], s[2], s[3]); }),
        DataBuild::FromSources("Wolf Steve in Xiao's weapon menu", std::string(kRebuiltDiscFolder) + kXiaoMenuWeaponsFile,
                               {kXiaoMenuWeaponsFile, kLambSwordFile, kMachoFile, kMachoSwordFile}, [](Files s) {
                                   ChrPack    bundle = ChrPack::Parse(s[0]);
                                   ChrRecord &steve = bundle.Require("c04w13.chr");
                                   steve.ReplacePayload(BuildWolfSteve(steve.Payload(), s[1], s[2], s[3]));
                                   return bundle.Rebuild();
                               }),
    };
}

} // namespace expanded
