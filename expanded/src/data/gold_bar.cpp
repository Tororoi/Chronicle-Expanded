#include "expanded/data/gold_bar.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <numbers>
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

    // The rounded box (half extents l, h/2, fw about its middle): each point of a box face's grid pulled to the inner box
    // (the box less r) plus r along the way out from it, the grid's rows at even turns of the rounding (kGoldBarRoundSteps
    // between a face and the next) and none between. Then narrowed toward the top, its ends to tl and its sides to tw.
    double half[3] = {l, h / 2, fw};
    auto   rows = [&](int axis) {
        std::vector<double> at;
        double              inner = half[axis] - r;
        for (int k = kGoldBarRoundSteps; k >= 0; k--) {
            at.push_back(-inner - r * std::tan(std::numbers::pi / 4 * k / kGoldBarRoundSteps));
        }
        for (int k = 0; k <= kGoldBarRoundSteps; k++) {
            at.push_back(inner + r * std::tan(std::numbers::pi / 4 * k / kGoldBarRoundSteps));
        }
        return at;
    };
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
    std::vector<bool>               flat; // each triangle's: on a face's flat middle, not a bevel
    for (int axis = 0; axis < 3; axis++) {
        int                 a = (axis + 1) % 3, b = (axis + 2) % 3;
        std::vector<double> along_a = rows(a), along_b = rows(b);
        for (double side : {-1.0, 1.0}) {
            auto corner = [&](std::size_t i, std::size_t j) {
                double q[3];
                q[axis] = side * half[axis];
                q[a] = along_a[i];
                q[b] = along_b[j];
                return point(q[0], q[1], q[2]);
            };
            for (std::size_t i = 0; i + 1 < along_a.size(); i++) {
                for (std::size_t j = 0; j + 1 < along_b.size(); j++) {
                    int  q[4] = {corner(i, j), corner(i + 1, j), corner(i + 1, j + 1), corner(i, j + 1)};
                    bool middle_cell = i == kGoldBarRoundSteps && j == kGoldBarRoundSteps;
                    for (const std::array<int, 3> &t : {
                             std::array<int, 3>{q[0], q[1], q[2]},
                             std::array<int, 3>{q[0], q[2], q[3]}
                    }) {
                        if (t[0] != t[1] && t[1] != t[2] && t[0] != t[2]) {
                            triangles.push_back(t);
                            flat.push_back(middle_cell);
                        }
                    }
                }
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
