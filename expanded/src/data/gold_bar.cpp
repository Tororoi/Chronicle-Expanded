#include "expanded/data/gold_bar.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "expanded/data/gold_statue.hpp"
#include "expanded/data/model_parts.hpp"

namespace expanded {

namespace {

using V = std::array<double, 4>;

V Unit(double x, double y, double z) {
    double l = std::sqrt(x * x + y * y + z * z);
    return {x / l, y / l, z / l, 0.0};
}

// Where on the gold ball a direction n reflects to, seen from in front and kGoldBarTilt above.
V BallAt(const V &n) {
    double c = std::sqrt(1.0 - kGoldBarTilt * kGoldBarTilt), s = kGoldBarTilt;
    // The view's up and right: right is x; up leans back toward -z as the view rises.
    double vx = n[0], vy = n[1] * c - n[2] * s;
    return {0.5 + 0.45 * vx, 0.5 - 0.45 * vy, 0.0, 0.0};
}

} // namespace

Bytes BuildGoldBarModel(std::span<const unsigned char> mds) {
    std::vector<MdsFrame> frames = ReadMdsSkeleton(mds);
    auto                  with_mesh = std::find_if(frames.begin(), frames.end(), [](const MdsFrame &f) { return f.mesh != 0; });
    if (with_mesh == frames.end()) {
        throw std::runtime_error("the Saving Book has no mesh");
    }
    MdtMesh mesh = MdtMesh::Parse(MdtAt(mds, std::size_t(with_mesh->mesh)));
    // Its size: the largest extent of its own.
    double low[3] = {1e9, 1e9, 1e9}, high[3] = {-1e9, -1e9, -1e9};
    for (const V &p : mesh.positions) {
        for (int c = 0; c < 3; c++) {
            low[c] = std::min(low[c], p[c]);
            high[c] = std::max(high[c], p[c]);
        }
    }
    double size = std::max({high[0] - low[0], high[1] - low[1], high[2] - low[2]});
    double centre[3] = {(low[0] + high[0]) / 2, low[1], (low[2] + high[2]) / 2};
    double l = kGoldBarLength * size / 2, fw = kGoldBarFoot * size / 2, tw = kGoldBarTop * size / 2, h = kGoldBarHeight * size;
    double tl = l - (fw - tw); // the top's ends lean in as its sides do
    double r = kGoldBarRound * h;

    // The bevelled box (half extents l, h/2, fw about its middle): each corner of the inner box (the box less r) stands
    // as three points, r out along each axis; a face is its four, an edge's bevel the four of its two faces at its ends, a
    // corner the triangle of its three. Then narrowed toward the top, its ends to tl and its sides to tw.
    double                                  half[3] = {l, h / 2, fw};
    std::vector<V>                          positions;
    std::map<std::array<long long, 3>, int> welded;
    auto                                    point = [&](double x, double y, double z) {
        double q[3] = {x, y, z}, in[3], out[3], length = 0.0;
        for (int c = 0; c < 3; c++) {
            in[c] = std::clamp(q[c], -(half[c] - r), half[c] - r);
            out[c] = q[c] - in[c];
            length += out[c] * out[c];
        }
        length = std::sqrt(length);
        V p{};
        for (int c = 0; c < 3; c++) {
            p[static_cast<std::size_t>(c)] = in[c] + (length > 0.0 ? out[c] / length * r : 0.0);
        }
        double t = (p[1] + h / 2) / h; // 0 at the foot, 1 at the top
        p[0] *= 1.0 - t * (1.0 - tl / l);
        p[2] *= 1.0 - t * (1.0 - tw / fw);
        p = {p[0] + centre[0], p[1] + h / 2 + centre[1], p[2] + centre[2], 1.0};
        std::array<long long, 3> key = {std::llround(p[0] * 1e4), std::llround(p[1] * 1e4), std::llround(p[2] * 1e4)};
        auto [found, added] = welded.try_emplace(key, static_cast<int>(positions.size()));
        if (added) {
            positions.push_back(p);
        }
        return found->second;
    };
    std::vector<std::array<int, 3>> triangles;
    std::vector<bool>               flat; // each triangle's: on a face, not a bevel
    // The point of the corner with signs sign, pushed out along axis.
    auto at = [&](const int(&sign)[3], int axis) {
        double q[3];
        for (int c = 0; c < 3; c++) {
            q[c] = sign[c] * (c == axis ? half[c] : half[c] - r);
        }
        return point(q[0], q[1], q[2]);
    };
    auto quad = [&](int p0, int p1, int p2, int p3, bool face) {
        triangles.push_back({p0, p1, p2});
        triangles.push_back({p0, p2, p3});
        flat.push_back(face);
        flat.push_back(face);
    };
    for (int axis = 0; axis < 3; axis++) {
        int a = (axis + 1) % 3, b = (axis + 2) % 3;
        for (int side : {-1, 1}) {
            // The face: its four corners in turn about it.
            int c[4][3];
            int turn[4][2] = {
                {-1, -1},
                {1,  -1},
                {1,  1 },
                {-1, 1 }
            };
            for (int k = 0; k < 4; k++) {
                c[k][axis] = side;
                c[k][a] = turn[k][0];
                c[k][b] = turn[k][1];
            }
            quad(at(c[0], axis), at(c[1], axis), at(c[2], axis), at(c[3], axis), true);
        }
        // The four edges running along axis, each between an a face and a b face, from one end to the other.
        for (int sa : {-1, 1}) {
            for (int sb : {-1, 1}) {
                int lo[3], hi[3];
                lo[axis] = -1;
                hi[axis] = 1;
                lo[a] = hi[a] = sa;
                lo[b] = hi[b] = sb;
                quad(at(lo, a), at(hi, a), at(hi, b), at(lo, b), false);
            }
        }
    }
    for (int sx : {-1, 1}) {
        for (int sy : {-1, 1}) {
            for (int sz : {-1, 1}) {
                int c[3] = {sx, sy, sz};
                triangles.push_back({at(c, 0), at(c, 1), at(c, 2)});
                flat.push_back(false);
            }
        }
    }

    // Each corner's normal the area-weighted mean of its triangles', for the bevels; a face's flat middle keeps its own,
    // so it reads flat. The triangles wound as the game's are (their cross product inward, as the Saving Book's), the way
    // out from the bar's middle telling which way is out.
    std::vector<std::array<double, 3>> sums(positions.size(), {0, 0, 0});
    std::vector<V>                     faces(triangles.size());
    double                             middle[3] = {centre[0], centre[1] + h / 2, centre[2]};
    for (std::size_t i = 0; i < triangles.size(); i++) {
        std::array<int, 3> &t = triangles[i];
        const V            &p0 = positions[static_cast<std::size_t>(t[0])], &p1 = positions[static_cast<std::size_t>(t[1])],
                &p2 = positions[static_cast<std::size_t>(t[2])];
        double e1[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]}, e2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
        double n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        double out = n[0] * (p0[0] - middle[0]) + n[1] * (p0[1] - middle[1]) + n[2] * (p0[2] - middle[2]);
        if (out > 0.0) {
            std::swap(t[1], t[2]);
        } else {
            for (double &c : n) {
                c = -c;
            }
        }
        faces[i] = Unit(n[0], n[1], n[2]);
        for (int k : t) {
            for (int c = 0; c < 3; c++) {
                sums[static_cast<std::size_t>(k)][static_cast<std::size_t>(c)] += n[c];
            }
        }
    }

    mesh.positions = positions;
    mesh.normals.clear();
    mesh.uvs.clear();
    mesh.colours = false;
    mesh.colour_values.clear();
    for (const auto &sum : sums) {
        V n = Unit(sum[0], sum[1], sum[2]);
        mesh.normals.push_back(n);
        mesh.uvs.push_back(BallAt(n));
    }
    MdtMesh::Submesh sub;
    sub.prim = 3;
    sub.material = 0;
    for (std::size_t i = 0; i < triangles.size(); i++) {
        int face = -1;
        if (flat[i]) {
            face = static_cast<int>(mesh.normals.size());
            mesh.normals.push_back(faces[i]);
        }
        for (int k : triangles[i]) {
            sub.records.push_back({k, face >= 0 ? face : k, k});
        }
    }
    mesh.submeshes = {sub};
    mesh.materials.resize(1);
    char *name = reinterpret_cast<char *>(mesh.materials[0].data() + MdtMesh::kMaterialTexture);
    std::memset(name, 0, std::strlen(name));
    std::memcpy(name, kGoldBarPicture, std::strlen(kGoldBarPicture));
    mesh.AlignDisplayList();

    std::map<std::string, std::optional<Bytes>> meshes;
    for (const MdsFrame &frame : frames) {
        if (frame.mesh != 0) {
            meshes[frame.name] = &frame == &*with_mesh ? std::optional<Bytes>(mesh.Build()) : std::nullopt;
        }
    }
    return ReplaceMeshes(mds, meshes);
}

Bytes BuildGoldBarPictures(std::span<const unsigned char> img) {
    ImgBank source = ImgBank::Parse(img);
    if (source.entries.empty()) {
        throw std::runtime_error("the Saving Book has no picture");
    }
    bool    swizzled = std::memcmp(source.magic.data(), "IM2", 3) == 0;
    ImgBank bank;
    bank.magic = source.magic;
    bank.entries.emplace_back(kGoldBarPicture, GoldBall(source.entries.front().second, swizzled));
    return bank.Build();
}

std::vector<DataBuild> GoldBarBuilds() {
    using Files = const std::vector<Bytes> &;
    std::string base = kGoldBarSource;
    return {
        DataBuild::FromSources("the Gold Bullion's model", kGoldBarModel, {base + ".mds"}, [](Files s) { return BuildGoldBarModel(s[0]); }),
        DataBuild::FromSources("the Gold Bullion's pictures", kGoldBarPictures, {base + ".img"}, [](Files s) { return BuildGoldBarPictures(s[0]); }),
    };
}

} // namespace expanded
