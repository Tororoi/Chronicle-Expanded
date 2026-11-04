#include "expanded/data/colossus_pop.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

namespace {

constexpr const char *kExplosionBank = "explosion.img", *kExplosionPicture = "explosion", *kExplosionMds = "explosion.mds";

std::array<int, 3> Turquoise(const std::array<int, 3> &rgb) {
    auto [h, s, l] = RgbToHsl(rgb[0], rgb[1], rgb[2]);
    return HslToRgb(kColossusPopHue, s, l);
}

} // namespace

Bytes BuildColossusPop(std::span<const unsigned char> explosion) {
    ChrPack pack = ChrPack::Parse(explosion);

    ImgBank bank = ImgBank::Parse(pack.Require(kExplosionBank).Payload());
    bool    swizzled = std::memcmp(bank.magic.data(), "IM2", 3) == 0;
    auto    picture = std::find_if(bank.entries.begin(), bank.entries.end(), [](const auto &e) { return e.first == kExplosionPicture; });
    if (picture == bank.entries.end()) {
        throw std::runtime_error(std::string(kExplosionBank) + " has no " + kExplosionPicture);
    }
    picture->second = RecolourSheet(picture->second, swizzled, Turquoise);
    picture->first = kColossusPopPicture;
    pack.Require(kExplosionBank).ReplacePayload(bank.Build());

    // Every mesh's materials drawn from the renamed picture.
    Bytes                                       mds = pack.Require(kExplosionMds).Payload();
    std::map<std::string, std::optional<Bytes>> meshes;
    for (const MdsFrame &frame : ReadMdsSkeleton(mds)) {
        if (frame.mesh == 0) {
            continue;
        }
        MdtMesh mesh = MdtMesh::Parse(MdtAt(mds, std::size_t(frame.mesh)));
        for (Bytes &material : mesh.materials) {
            char *name = reinterpret_cast<char *>(material.data() + MdtMesh::kMaterialTexture);
            if (std::strcmp(name, kExplosionPicture) == 0) {
                std::memset(name, 0, std::strlen(kExplosionPicture));
                std::memcpy(name, kColossusPopPicture, std::strlen(kColossusPopPicture));
            }
        }
        meshes[frame.name] = mesh.Build();
    }
    pack.Require(kExplosionMds).ReplacePayload(ReplaceMeshes(mds, meshes));
    return pack.Rebuild();
}

std::vector<DataBuild> ColossusPopBuilds() {
    using Files = const std::vector<Bytes> &;
    return {
        DataBuild::FromSources("Colossus's pop", kColossusPopFile, {kColossusPopSource}, [](Files s) { return BuildColossusPop(s[0]); }),
    };
}

} // namespace expanded
