#include "expanded/data/recolour_pack.hpp"

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

Bytes RecolourPackPicture(std::span<const unsigned char> source, const char *bank_name, const char *picture_name, const char *mds_name,
                          const char *renamed, const std::function<std::array<int, 3>(const std::array<int, 3> &)> &colour_of) {
    ChrPack pack = ChrPack::Parse(source);

    ImgBank bank = ImgBank::Parse(pack.Require(bank_name).Payload());
    bool    swizzled = std::memcmp(bank.magic.data(), "IM2", 3) == 0;
    auto    picture = std::find_if(bank.entries.begin(), bank.entries.end(), [&](const auto &e) { return e.first == picture_name; });
    if (picture == bank.entries.end()) {
        throw std::runtime_error(std::string(bank_name) + " has no " + picture_name);
    }
    picture->second = RecolourSheet(picture->second, swizzled, colour_of);
    picture->first = renamed;
    pack.Require(bank_name).ReplacePayload(bank.Build());

    // Every mesh's materials drawn from the renamed picture.
    Bytes                                       mds = pack.Require(mds_name).Payload();
    std::map<std::string, std::optional<Bytes>> meshes;
    for (const MdsFrame &frame : ReadMdsSkeleton(mds)) {
        if (frame.mesh == 0) {
            continue;
        }
        MdtMesh mesh = MdtMesh::Parse(MdtAt(mds, std::size_t(frame.mesh)));
        for (Bytes &material : mesh.materials) {
            char *name = reinterpret_cast<char *>(material.data() + MdtMesh::kMaterialTexture);
            if (std::strcmp(name, picture_name) == 0) {
                std::memset(name, 0, std::strlen(picture_name));
                std::memcpy(name, renamed, std::strlen(renamed));
            }
        }
        meshes[frame.name] = mesh.Build();
    }
    pack.Require(mds_name).ReplacePayload(ReplaceMeshes(mds, meshes));
    return pack.Rebuild();
}

} // namespace expanded
