#include "expanded/data/goro_gold.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

namespace {

constexpr const char *kGoroMds = "c06a.mds", *kGoroBbp = "c06a.bbp", *kGoroWgt = "c06a.wgt", *kGoroCfg = "base.cfg";
constexpr const char *kSkinBuffer = "ALLOC_DBUFF \"skin\"";
constexpr int         kSamples = 10; // a triangle's texels sampled on a barycentric grid this fine

bool Swizzled(const ImgBank &bank) {
    return std::memcmp(bank.magic.data(), "IM2", 3) == 0;
}

std::array<int, 3> At(const tim8::Picture &picture, int index) {
    std::size_t j = static_cast<std::size_t>(tim8::Csm1(index)) * 4;
    return {picture.clut[j], picture.clut[j + 1], picture.clut[j + 2]};
}

// Each texel's region.
std::vector<GoroRegion> Regions(const char *sheet, const tim8::Picture &picture) {
    std::vector<GoroRegion> out(picture.pixels.size());
    for (int y = 0; y < picture.height; y++) {
        for (int x = 0; x < picture.width; x++) {
            std::size_t i = static_cast<std::size_t>(y) * picture.width + x;
            out[i] = GoroRegionOf(sheet, x, y, At(picture, picture.pixels[i]));
        }
    }
    return out;
}

// The sheet's gold copy: each texel's colour in its region's copy, merged to 256 and indexed again.
Bytes GoldCopy(std::span<const unsigned char> tim, const char *sheet, bool swizzled) {
    tim8::Picture                   picture = tim8::Read(tim, swizzled);
    std::vector<GoroRegion>         regions = Regions(sheet, picture);
    std::vector<int>                colour(picture.pixels.size());
    std::unordered_map<int, double> weight;
    std::vector<int>                order;
    for (std::size_t i = 0; i < colour.size(); i++) {
        auto c = GoroGoldColour(regions[i], At(picture, picture.pixels[i]));
        colour[i] = (c[0] << 16) | (c[1] << 8) | c[2];
        if (weight[colour[i]]++ == 0) {
            order.push_back(colour[i]);
        }
    }
    std::unordered_map<int, int> merged = MergeColours(order, weight);
    std::unordered_map<int, int> slot;
    Bytes                        index(colour.size()), clut(256 * 4, 0);
    int                          used = 0;
    for (std::size_t i = 0; i < colour.size(); i++) {
        int v = colour[i];
        if (auto m = merged.find(v); m != merged.end()) {
            v = m->second;
        }
        auto [found, added] = slot.try_emplace(v, used);
        if (added) {
            if (used >= 256) {
                throw std::runtime_error(std::string(sheet) + ": more than 256 colours after merging");
            }
            std::size_t j = static_cast<std::size_t>(tim8::Csm1(used)) * 4;
            clut[j] = static_cast<unsigned char>(v >> 16);
            clut[j + 1] = static_cast<unsigned char>(v >> 8);
            clut[j + 2] = static_cast<unsigned char>(v);
            clut[j + 3] = 0x80;
            used++;
        }
        index[i] = static_cast<unsigned char>(found->second);
    }
    Bytes pixels = swizzled ? tim8::Swizzle(index, picture.width, picture.height) : index;
    return tim8::Build(tim, picture.width, picture.height, pixels, clut);
}

// A display list record's texture coordinate (u, v) in a mesh.
std::array<double, 2> Uv(const MdtMesh &m, const std::vector<int> &record) {
    const auto &t = m.uvs.at(static_cast<std::size_t>(record[2]));
    return {t[0], t[1]};
}

// Whether a triangle's texels are mostly gold.
bool GoldTriangle(const MdtMesh &m, const std::vector<int> *corner[3], const std::vector<GoroRegion> &regions, int width, int height) {
    std::array<std::array<double, 2>, 3> uv = {Uv(m, *corner[0]), Uv(m, *corner[1]), Uv(m, *corner[2])};
    int                                  gold = 0, all = 0;
    for (int a = 0; a <= kSamples; a++) {
        for (int b = 0; a + b <= kSamples; b++) {
            int    c = kSamples - a - b;
            double u = (a * uv[0][0] + b * uv[1][0] + c * uv[2][0]) / kSamples, v = (a * uv[0][1] + b * uv[1][1] + c * uv[2][1]) / kSamples;
            int    x = ((static_cast<int>(std::floor(u * width)) % width) + width) % width;
            int    y = ((static_cast<int>(std::floor(v * height)) % height) + height) % height;
            gold += regions[static_cast<std::size_t>(y) * width + x] == GoroRegion::Gold;
            all++;
        }
    }
    return gold * 2 > all;
}

// The body's triangles split by region into two meshes (each its submeshes as lists, the windings kept), the same
// vertices in both.
std::pair<MdtMesh, MdtMesh> Split(const MdtMesh &body, const std::unordered_map<std::string, std::vector<GoroRegion>> &regions,
                                  const std::unordered_map<std::string, std::pair<int, int>> &sizes) {
    MdtMesh kept = body, gold = body;
    kept.submeshes.clear();
    gold.submeshes.clear();
    for (const MdtMesh::Submesh &sub : body.submeshes) {
        const Bytes &material = body.materials.at(static_cast<std::size_t>(sub.material));
        std::string  texture(reinterpret_cast<const char *>(material.data() + MdtMesh::kMaterialTexture));
        texture = texture.substr(0, texture.find('\0'));
        auto             region = regions.find(texture);
        MdtMesh::Submesh k{3, sub.material, {}}, g{3, sub.material, {}};
        const auto      &r = sub.records;
        auto             add = [&](std::size_t a, std::size_t b, std::size_t c) {
            if (r[a][0] == r[b][0] || r[b][0] == r[c][0] || r[a][0] == r[c][0]) {
                return; // a strip's degenerate
            }
            const std::vector<int> *corner[3] = {&r[a], &r[b], &r[c]};
            bool                    is_gold = region != regions.end() &&
                           GoldTriangle(body, corner, region->second, sizes.at(texture).first, sizes.at(texture).second);
            MdtMesh::Submesh &to = is_gold ? g : k;
            to.records.push_back(r[a]);
            to.records.push_back(r[b]);
            to.records.push_back(r[c]);
        };
        if (sub.prim == 3) {
            for (std::size_t i = 0; i + 2 < r.size(); i += 3) {
                add(i, i + 1, i + 2);
            }
        } else if (sub.prim == 4) {
            for (std::size_t i = 0; i + 2 < r.size(); i++) {
                if (i % 2 == 0) {
                    add(i, i + 1, i + 2);
                } else {
                    add(i + 1, i, i + 2); // a strip turns its winding every other triangle
                }
            }
        } else {
            throw std::runtime_error("an MDT primitive that is neither a list nor a strip");
        }
        if (!k.records.empty()) {
            kept.submeshes.push_back(std::move(k));
        }
        if (!g.records.empty()) {
            gold.submeshes.push_back(std::move(g));
        }
    }
    if (gold.submeshes.empty()) {
        throw std::runtime_error(std::string(kGoroBody) + " has no gold triangles");
    }
    kept.AlignDisplayList();
    gold.AlignDisplayList();
    return {std::move(kept), std::move(gold)};
}

} // namespace

GoroRegion GoroRegionOf(const char *sheet, int x, int y, const std::array<int, 3> &rgb) {
    if (std::strcmp(sheet, kGoroWrapsSheet) == 0 && x >= kGoroWraps[0] && y >= kGoroWraps[1] && x < kGoroWraps[2] && y < kGoroWraps[3]) {
        return GoroRegion::Kept;
    }
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    if (h >= 60 && h < 180 && s > 12) {
        return GoroRegion::Turquoise;
    }
    if (s < 10 || l > 85 || l < 2) { // bone, buckles, highlights, the blackest lines
        return GoroRegion::Kept;
    }
    if (h < 8 || h >= 330) { // the red strip, the pink
        return GoroRegion::Kept;
    }
    if (h < 30 && l >= 50 && s >= 35) { // flesh
        return GoroRegion::Kept;
    }
    if (h >= 25 && h < 60 && l >= 62 && s < 35) { // teeth and claws
        return GoroRegion::Kept;
    }
    return h < 70 ? GoroRegion::Gold : GoroRegion::Kept;
}

std::array<int, 3> GoroGoldColour(GoroRegion region, const std::array<int, 3> &rgb) {
    if (region == GoroRegion::Kept) {
        return rgb;
    }
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return HslToRgb(region == GoroRegion::Gold ? kGoroGoldHue : kGoroTurquoiseHue, s, l);
}

Bytes BuildGoroGold(std::span<const unsigned char> goro) {
    ChrPack pack = ChrPack::Parse(goro);

    // The copies, in a bank of their own.
    ImgBank                                                  bank = ImgBank::Parse(pack.Require(kGoroBank).Payload());
    ImgBank                                                  gold_bank = bank;
    std::unordered_map<std::string, std::vector<GoroRegion>> regions;
    std::unordered_map<std::string, std::pair<int, int>>     sizes;
    gold_bank.entries.clear();
    for (const char *sheet : kGoroGoldSheets) {
        const Bytes  &tim = bank.Block(sheet);
        tim8::Picture picture = tim8::Read(tim, Swizzled(bank));
        regions[sheet] = Regions(sheet, picture);
        sizes[sheet] = {picture.width, picture.height};
        gold_bank.entries.emplace_back(sheet, GoldCopy(tim, sheet, Swizzled(bank)));
    }
    pack.records.push_back(ChrRecord::Make(kGoroGoldBank, gold_bank.Build()));

    // The gold frame: the body's gold triangles, a frame beside it with its record, bind row and weights.
    Bytes           mds = pack.Require(kGoroMds).Payload();
    auto            frames = ReadMdsSkeleton(mds);
    const MdsFrame *body = FindFrame(frames, kGoroBody);
    if (body == nullptr || body->mesh == 0) {
        throw std::runtime_error(std::string(kGoroMds) + " has no " + kGoroBody + " mesh");
    }
    auto [kept, gold] = Split(MdtMesh::Parse(MdtAt(mds, std::size_t(body->mesh))), regions, sizes);
    const int index = static_cast<int>(frames.size());
    Bytes     record = body->record;
    Set32(record, mds::kIndex, static_cast<std::uint32_t>(index));
    std::fill(record.begin() + mds::kName, record.begin() + mds::kName + mds::kNameSize, 0);
    std::memcpy(record.data() + mds::kName, kGoroGoldBody, std::strlen(kGoroGoldBody));
    pack.Require(kGoroMds).ReplacePayload(ReplaceMeshes(mds, {
                                                                 {kGoroBody, kept.Build()}
    },
                                                        {{record, gold.Build()}}));

    Bytes bbp = pack.Require(kGoroBbp).Payload();
    if (bbp.size() < static_cast<std::size_t>(index) * bbp::kEntry) {
        bbp.resize(static_cast<std::size_t>(index) * bbp::kEntry, 0);
    }
    std::size_t row = static_cast<std::size_t>(body->index) * bbp::kEntry;
    Bytes       body_row(bbp.begin() + static_cast<std::ptrdiff_t>(row), bbp.begin() + static_cast<std::ptrdiff_t>(row + bbp::kEntry));
    bbp.resize(static_cast<std::size_t>(index) * bbp::kEntry);
    bbp.insert(bbp.end(), body_row.begin(), body_row.end());
    pack.Require(kGoroBbp).ReplacePayload(bbp);

    MotFile               wgt = MotFile::FromRecord(pack.Require(kGoroWgt));
    std::vector<MotTrack> copies;
    for (const MotTrack &track : wgt.tracks) {
        if (track.w0 == static_cast<std::uint32_t>(body->index)) {
            MotTrack copy = track;
            copy.w0 = static_cast<std::uint32_t>(index);
            copies.push_back(std::move(copy));
        }
    }
    if (copies.empty()) {
        throw std::runtime_error(std::string(kGoroWgt) + " has no weights for " + kGoroBody);
    }
    wgt.tracks.insert(wgt.tracks.end(), copies.begin(), copies.end());
    pack.Require(kGoroWgt).ReplacePayload(wgt.BuildPayload());

    // Skinned as the body is, from a vertex buffer of its own.
    Bytes       cfg = pack.Require(kGoroCfg).Payload();
    std::string text(cfg.begin(), cfg.end());
    std::size_t at = text.find(kSkinBuffer);
    if (at == std::string::npos) {
        throw std::runtime_error(std::string(kGoroCfg) + " has no " + kSkinBuffer);
    }
    text.insert(at + std::strlen(kSkinBuffer), std::string("\r\nALLOC_DBUFF \"") + kGoroGoldBody + "\"");
    pack.Require(kGoroCfg).ReplacePayload(Bytes(text.begin(), text.end()));
    return pack.Rebuild();
}

std::vector<DataBuild> GoroGoldBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Goro in gold", std::string(kRebuiltDiscFolder) + kGoroDungeonFile, {kGoroDungeonFile},
                               [](Files s) { return BuildGoroGold(s[0]); }),
    };
}

} // namespace expanded
