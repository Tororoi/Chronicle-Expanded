#include "expanded/data/prop_models.hpp"

#include <array>
#include <cctype>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/model_parts.hpp"

namespace expanded {

namespace {

constexpr const char *kTreesSubFile = "e03t01";
constexpr const char *kTreesBank = "e03b01.img";
constexpr const char *kTreesPicture = "e03b04";
constexpr const char *kTreesGround = "grid__a7f";
constexpr const char *kRockModel = "iwa.mds";
constexpr const char *kRockBank = "iwa.img";
constexpr const char *kRockPicture = "d02b10";

constexpr std::size_t kMdsNodeTable = 0xC, kMdsNodeCount = 8, kMdsNode = 0x70, kMdsNodeName = 8, kMdsNodeMesh = 0x28;
constexpr std::size_t kMdsOneNode = 0x80; // the header and one frame; the mesh follows
constexpr std::size_t kPackEnd = 0x50;    // the zero record that ends a pack

Bytes Text(const std::string &text) {
    return Bytes(text.begin(), text.end());
}

// A character's cfg: its one bank, its model (and shadow model, when given), no motion.
std::string Cfg(const std::string &title, const std::string &bank, const std::string &model, const std::string &shadow = "") {
    std::string cfg = "//" + title + "\r\nIMG 0,\"" + bank + "\"\r\nIMG_END\r\nMATERIAL_ANIME 0\r\nVERTEX_ANIME 0\r\n\r\nMODEL \"" + model + "\"\r\n";
    if (!shadow.empty()) {
        cfg += "SHADOW_MODEL \"" + shadow + "\"\r\n";
    }
    return cfg + "BODY_SIZE 17,7,60\r\n\r\nMOTION 0, \"\", \"\", \"\"\r\nSHADOW_MOTION \"\", \"\", \"\"\r\nKEY_START 0\r\nMOTION_END\r\n";
}

Bytes Pack(std::vector<ChrRecord> records) {
    ChrPack pack;
    pack.records = std::move(records);
    pack.trailer.assign(kPackEnd, 0);
    return pack.Rebuild();
}

// The scene's sub-file: its directory has 0x30-byte entries from 0x10, the name then its offset and size at +0x10 / +0x14.
std::span<const unsigned char> SubFile(std::span<const unsigned char> scene, const std::string &name) {
    for (std::size_t o = 0x10; o + 0x30 <= scene.size(); o += 0x30) {
        std::string entry = NameAt(scene, o, 16);
        if (entry.empty() || !std::isalnum(static_cast<unsigned char>(entry[0]))) {
            break;
        }
        if (entry == name) {
            return Slice(scene, Get32(scene, o + 0x10), Get32(scene, o + 0x14), "a scene sub-file");
        }
    }
    throw std::runtime_error("sub-file " + name + " not in the scene directory");
}

// A canonical MDT: the header words of the source's, then positions, the display list (one submesh of triangle lists), the
// blocks at 0x18 and 0x30, and the materials, each from a 16-byte boundary.
Bytes BuildMdt(const std::array<std::uint32_t, 16> &source, const std::array<std::uint32_t, 4> &preamble, int material,
               const std::vector<std::array<float, 4>> &positions, const std::vector<int> &corners, const std::array<float, 4> &at_18,
               const std::array<float, 4> &at_30, std::span<const unsigned char> materials) {
    Bytes out(0x40, 0);
    auto  vectors = [&](const std::vector<std::array<float, 4>> &values) {
        PadTo16(out);
        std::size_t at = out.size();
        for (const auto &v : values) {
            for (float f : v) {
                out.resize(out.size() + 4);
                SetF32(out, out.size() - 4, f);
            }
        }
        return at;
    };
    auto word = [&](std::uint32_t w) {
        out.resize(out.size() + 4);
        Set32(out, out.size() - 4, w);
    };
    std::size_t pos = vectors(positions);
    PadTo16(out);
    std::size_t dl = out.size();
    word(preamble[0]);
    word(preamble[1]);
    word(1);
    word(preamble[3]);
    word(3); // triangle lists
    word(static_cast<std::uint32_t>(corners.size()));
    word(static_cast<std::uint32_t>(material));
    for (int corner : corners) {
        word(static_cast<std::uint32_t>(corner));
        word(0);
        word(0);
    }
    std::size_t dl_size = out.size() - dl;
    std::size_t block_18 = vectors({at_18});
    std::size_t block_30 = vectors({at_30});
    PadTo16(out);
    std::size_t mat = out.size();
    out.insert(out.end(), materials.begin(), materials.end());
    PadTo16(out);
    std::array<std::uint32_t, 16> h = source;
    h[2] = static_cast<std::uint32_t>(out.size());
    h[3] = static_cast<std::uint32_t>(positions.size());
    h[4] = static_cast<std::uint32_t>(pos);
    h[5] = 1;
    h[6] = static_cast<std::uint32_t>(block_18);
    h[9] = static_cast<std::uint32_t>(dl_size);
    h[10] = static_cast<std::uint32_t>(dl);
    h[11] = 1;
    h[12] = static_cast<std::uint32_t>(block_30);
    h[14] = static_cast<std::uint32_t>(mat);
    for (std::size_t i = 0; i < 16; i++) {
        Set32(out, i * 4, h[i]);
    }
    return out;
}

} // namespace

Bytes BuildQueensTrees(std::span<const unsigned char> scene, std::span<const unsigned char> images) {
    // The sub-file's first two 16-aligned MDS blocks are the part's display and collision models; the display block's
    // offsets are its own, so it stands alone.
    std::span<const unsigned char> part = SubFile(scene, kTreesSubFile);
    std::vector<std::size_t>       starts;
    for (std::size_t p = 0; p + 4 <= part.size() && starts.size() < 2; p += 16) {
        if (part[p] == 'M' && part[p + 1] == 'D' && part[p + 2] == 'S' && part[p + 3] == 0) {
            starts.push_back(p);
        }
    }
    if (starts.size() < 2) {
        throw std::runtime_error(std::string(kTreesSubFile) + ": its display and collision models are not there");
    }
    Bytes mds(part.begin() + static_cast<std::ptrdiff_t>(starts[0]), part.begin() + static_cast<std::ptrdiff_t>(starts[1]));
    // The grass square's mesh dropped: its frame stays (every other keeps its index and parent), with no mesh to build.
    bool        dropped = false;
    std::size_t table = Get32(mds, kMdsNodeTable);
    for (std::uint32_t i = 0; i < Get32(mds, kMdsNodeCount); i++) {
        std::size_t node = table + i * kMdsNode;
        if (NameAt(mds, node + kMdsNodeName, 0x20) == kTreesGround) {
            Set32(mds, node + kMdsNodeMesh, 0);
            dropped = true;
        }
    }
    if (!dropped) {
        throw std::runtime_error(std::string(kTreesSubFile) + ": no frame " + kTreesGround);
    }
    ImgBank source = ImgBank::Parse(ChrPack::Parse(images).Require(kTreesBank).Payload());
    ImgBank bank;
    bank.magic = source.magic;
    bank.entries.emplace_back(kTreesPicture, source.Block(kTreesPicture));
    std::string name = kTreesSubFile;
    return Pack({ChrRecord::Make(kQueensTreesCfg, Text(Cfg("queens trees", name + ".img", name + ".mds"))), ChrRecord::Make(name + ".mds", mds),
                 ChrRecord::Make(name + ".img", bank.Build())});
}

Bytes BuildShadowDisc(std::span<const unsigned char> rock) {
    ChrPack pack = ChrPack::Parse(rock);
    Bytes   model = pack.Require(kRockModel).Payload();
    if (model.size() < kMdsOneNode + 0x40 || Get32(model, kMdsNodeCount) != 1) {
        throw std::runtime_error(std::string(kRockModel) + " is not a one-frame model");
    }
    std::span<const unsigned char> mdt = std::span<const unsigned char>(model).subspan(kMdsOneNode);
    std::array<std::uint32_t, 16>  header{};
    for (std::size_t i = 0; i < 16; i++) {
        header[i] = Get32(mdt, i * 4);
    }
    std::array<std::uint32_t, 4> preamble{};
    for (std::size_t i = 0; i < 4; i++) {
        preamble[i] = Get32(mdt, header[10] + i * 4);
    }
    std::span<const unsigned char> materials = Slice(mdt, header[14], header[2] - header[14], "the rock's materials");

    // A centre and the rim, a triangle fan wound both ways; one entry in each of the 0x18 and 0x30 blocks: nothing, and up.
    std::vector<std::array<float, 4>> positions = {
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    for (int i = 0; i < kShadowDiscSegments; i++) {
        double a = 2.0 * std::numbers::pi * i / kShadowDiscSegments;
        positions.push_back({static_cast<float>(std::sin(a)), 0.0f, static_cast<float>(std::cos(a)), 1.0f});
    }
    std::vector<int> corners;
    for (int i = 0; i < kShadowDiscSegments; i++) {
        int a = 1 + i, b = 1 + (i + 1) % kShadowDiscSegments;
        corners.insert(corners.end(), {0, a, b, 0, b, a});
    }
    Bytes disc(model.begin(), model.begin() + static_cast<std::ptrdiff_t>(kMdsOneNode));
    Bytes mesh = BuildMdt(header, preamble, 0, positions, corners, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, materials);
    disc.insert(disc.end(), mesh.begin(), mesh.end());

    ImgBank source = ImgBank::Parse(pack.Require(kRockBank).Payload());
    ImgBank bank;
    bank.magic = {'I', 'M', 'G', 0};
    bank.entries.emplace_back(kRockPicture, tim8::Resample(source.Block(kRockPicture), source.magic[2] == '2', kShadowDiscStandIn));
    return Pack({ChrRecord::Make(kShadowDiscCfg, Text(Cfg("shadow disc", "disc.img", "disc.mds", "disc_shadow.mds"))),
                 ChrRecord::Make("disc.mds", disc), ChrRecord::Make("disc_shadow.mds", disc), ChrRecord::Make("disc.img", bank.Build())});
}

int BuildPropModels(const std::filesystem::path &data, std::FILE *log) {
    std::vector<DataBuild> builds = {
        {"Queens' trees", kQueensTreesFile, {kQueensSceneFile, kQueensImagesFile}, [](const DiscReader &read) { return BuildQueensTrees(read(kQueensSceneFile), read(kQueensImagesFile)); }},
        {"shadow disc",   kShadowDiscFile,  {kShadowDiscSource},                   [](const DiscReader &read) { return BuildShadowDisc(read(kShadowDiscSource)); }                         },
    };
    return RunDataBuilds(data, "prop model", builds, log);
}

} // namespace expanded
