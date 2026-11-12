#include "expanded/data/steel_hammer_head.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/data/model_parts.hpp"

namespace expanded {

namespace {

constexpr int kList = 3;  // a submesh's records three to a triangle
constexpr int kStrip = 4; // or a strip

// A submesh's triangles, each three records (a strip's every other one turned so it winds as the first).
std::vector<std::array<std::vector<int>, 3>> Triangles(const MdtMesh::Submesh &sub) {
    std::vector<std::array<std::vector<int>, 3>> out;
    const auto                                  &r = sub.records;
    if (sub.prim == kStrip) {
        for (std::size_t i = 0; i + 2 < r.size(); i++) {
            if (i % 2 == 0) {
                out.push_back({r[i], r[i + 1], r[i + 2]});
            } else {
                out.push_back({r[i + 1], r[i], r[i + 2]});
            }
        }
    } else {
        for (std::size_t i = 0; i + 2 < r.size(); i += 3) {
            out.push_back({r[i], r[i + 1], r[i + 2]});
        }
    }
    return out;
}

} // namespace

Bytes SteelHammerHead(std::span<const unsigned char> hammer_mds) {
    std::vector<MdsFrame> frames = ReadMdsSkeleton(hammer_mds);
    auto                  with_mesh = std::find_if(frames.begin(), frames.end(), [](const MdsFrame &f) { return f.mesh != 0; });
    if (with_mesh == frames.end()) {
        throw std::runtime_error("the Steel Hammer has no mesh");
    }
    MdtMesh mesh = MdtMesh::Parse(MdtAt(hammer_mds, std::size_t(with_mesh->mesh)));

    // The triangles past the cut, as lists.
    bool any = false;
    for (MdtMesh::Submesh &sub : mesh.submeshes) {
        std::vector<std::vector<int>> kept;
        for (const auto &triangle : Triangles(sub)) {
            int  p[3] = {triangle[0][0], triangle[1][0], triangle[2][0]};
            bool head = true;
            for (int c : p) {
                head = head && mesh.positions[static_cast<std::size_t>(c)][2] >= kSteelHeadCut;
            }
            if (!head || p[0] == p[1] || p[1] == p[2] || p[0] == p[2]) {
                continue; // the handle, or a strip's turn
            }
            kept.insert(kept.end(), triangle.begin(), triangle.end());
            any = true;
        }
        sub.prim = kList;
        sub.records = std::move(kept);
    }
    if (!any) {
        throw std::runtime_error("the Steel Hammer's head has no triangle");
    }

    // The ball's middle along z.
    double ball_low = std::numeric_limits<double>::max(), ball_high = std::numeric_limits<double>::lowest();
    for (const MdtMesh::Submesh &sub : mesh.submeshes) {
        if (sub.material != kSteelHeadBall) {
            continue;
        }
        for (const auto &corner : sub.records) {
            double z = mesh.positions[static_cast<std::size_t>(corner[0])][2];
            ball_low = std::min(ball_low, z);
            ball_high = std::max(ball_high, z);
        }
    }
    if (ball_low > ball_high) {
        throw std::runtime_error("the Steel Hammer's head has no ball");
    }
    double middle = (ball_low + ball_high) / 2.0;

    // The top spike mirrored through the ball's middle: its corners and normals copied with z turned over, each
    // triangle wound back (the mirror turns it inside out), its texture coordinates as they were.
    std::map<int, int>            moved_positions, moved_normals;
    std::vector<MdtMesh::Submesh> spikes;
    for (const MdtMesh::Submesh &sub : mesh.submeshes) {
        if (sub.material != kSteelHeadTopSpike || sub.records.empty()) {
            continue;
        }
        MdtMesh::Submesh spike = sub;
        for (std::vector<int> &corner : spike.records) {
            auto [p, new_p] = moved_positions.try_emplace(corner[0], static_cast<int>(mesh.positions.size()));
            if (new_p) {
                auto position = mesh.positions[static_cast<std::size_t>(corner[0])];
                position[2] = 2.0 * middle - position[2];
                mesh.positions.push_back(position);
            }
            auto [n, new_n] = moved_normals.try_emplace(corner[1], static_cast<int>(mesh.normals.size()));
            if (new_n) {
                auto normal = mesh.normals[static_cast<std::size_t>(corner[1])];
                normal[2] = -normal[2];
                mesh.normals.push_back(normal);
            }
            corner[0] = p->second;
            corner[1] = n->second;
        }
        for (std::size_t i = 0; i + 2 < spike.records.size(); i += 3) {
            std::swap(spike.records[i + 1], spike.records[i + 2]);
        }
        spikes.push_back(std::move(spike));
    }
    if (spikes.empty()) {
        throw std::runtime_error("the Steel Hammer's head has no top spike");
    }
    mesh.submeshes.insert(mesh.submeshes.end(), spikes.begin(), spikes.end());

    for (auto &position : mesh.positions) {
        position[2] -= middle;
    }
    mesh.AlignDisplayList();
    return mesh.Build();
}

Bytes BuildSteelHeadThrow(std::span<const unsigned char> apple, std::span<const unsigned char> hammer) {
    ChrPack    hammer_pack = ChrPack::Parse(hammer);
    Bytes      head = SteelHammerHead(hammer_pack.Require(kSteelHammerModel).Payload());
    ChrPack    pack = ChrPack::Parse(apple);
    ChrRecord &mds = pack.Require("ringo_ex.mds");
    mds.ReplacePayload(ReplaceMeshes(mds.Payload(), {
                                                        {"dokuring__m", head}
    }));
    ChrRecord &img = pack.Require("dokuring.img");
    ImgBank    bank = ImgBank::Parse(img.Payload());
    ImgBank    pictures = ImgBank::Parse(hammer_pack.Require(kSteelHammerPictures).Payload());
    bool       blocked = std::memcmp(bank.magic.data(), "IM2", 3) == 0;
    bool       was_blocked = std::memcmp(pictures.magic.data(), "IM2", 3) == 0;
    for (const auto &[name, picture] : pictures.entries) {
        bank.entries.emplace_back(name, blocked && !was_blocked ? tim8::Swizzled(picture) : picture);
    }
    img.ReplacePayload(bank.Build());
    return pack.Rebuild();
}

std::vector<DataBuild> SteelHammerHeadBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("the Steel Hammer's thrown head", kSteelHeadThrowFile, {kBombThrowSource, kSteelHammerPack},
                               [](Files s) { return BuildSteelHeadThrow(s[0], s[1]); }),
    };
}

} // namespace expanded
