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
    // Corners: the foot (y 0) then the top (y h), each round from -x -z.
    std::array<V, 8> corner = {
        {{-l, 0, -fw, 1}, {l, 0, -fw, 1}, {l, 0, fw, 1}, {-l, 0, fw, 1}, {-tl, h, -tw, 1}, {tl, h, -tw, 1}, {tl, h, tw, 1}, {-tl, h, tw, 1}}
    };
    for (V &v : corner) {
        v[0] += centre[0];
        v[1] += centre[1];
        v[2] += centre[2];
    }

    // Each face its corners (wound outward) and its normal; each corner's UV leans its face's toward the corner.
    struct Face {
        std::array<int, 4> c;
        V                  n;
    };

    double     lean = (fw - tw) / h, ends = (l - tl) / h;
    std::array faces = {
        Face{{4, 5, 6, 7}, Unit(0,  1,    0) },
        Face{{3, 2, 1, 0}, Unit(0,  -1,   0) },
        Face{{0, 1, 5, 4}, Unit(0,  lean, -1)},
        Face{{2, 3, 7, 6}, Unit(0,  lean, 1) },
        Face{{1, 2, 6, 5}, Unit(1,  ends, 0) },
        Face{{3, 0, 4, 7}, Unit(-1, ends, 0) },
    };
    mesh.positions.assign(corner.begin(), corner.end());
    mesh.normals.clear();
    mesh.uvs.clear();
    mesh.colours = false;
    mesh.colour_values.clear();
    MdtMesh::Submesh sub;
    sub.prim = 3;
    sub.material = 0;
    for (const Face &face : faces) {
        int normal = static_cast<int>(mesh.normals.size());
        mesh.normals.push_back(face.n);
        int first_uv = static_cast<int>(mesh.uvs.size());
        for (int k = 0; k < 4; k++) {
            const V &p = corner[static_cast<std::size_t>(face.c[static_cast<std::size_t>(k)])];
            V        towards = Unit(face.n[0] + 0.35 * (p[0] - centre[0]) / l, face.n[1] + 0.35 * (p[1] - centre[1] - h / 2) / h,
                                    face.n[2] + 0.35 * (p[2] - centre[2]) / fw);
            mesh.uvs.push_back(BallAt(towards));
        }
        for (int k : {0, 1, 2, 0, 2, 3}) {
            sub.records.push_back({face.c[static_cast<std::size_t>(k)], normal, first_uv + k});
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
