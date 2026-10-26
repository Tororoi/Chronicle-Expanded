#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/prop_models.hpp"

using namespace expanded;

namespace {

namespace fs = std::filesystem;

// DC_TEST_DATA, an extracted data directory, or skipped.
#define DATA_OR_SKIP(data)                          \
    const char *data = std::getenv("DC_TEST_DATA"); \
    if (data == nullptr) {                          \
        GTEST_SKIP() << "DC_TEST_DATA is not set";  \
    }

bool HasRecord(const ChrPack &pack, const std::string &name) {
    return pack.Find(name) != nullptr;
}

} // namespace

TEST(PropModels, QueensTreesAreTheirDisplayModelWithoutTheGrassSquare) {
    DATA_OR_SKIP(data);
    ChrPack pack = ChrPack::Parse(BuildQueensTrees(ReadDataFile(fs::path(data) / kQueensSceneFile), ReadDataFile(fs::path(data) / kQueensImagesFile)));
    ASSERT_TRUE(HasRecord(pack, kQueensTreesCfg));
    Bytes mds = pack.Require("e03t01.mds").Payload();
    ASSERT_EQ(Get32(mds, 8), 16u);
    for (std::uint32_t i = 0; i < 16; i++) {
        std::size_t node = Get32(mds, 0xC) + i * 0x70;
        std::string name = NameAt(mds, node + 8, 0x20);
        bool        meshed = Get32(mds, node + 0x28) != 0;
        EXPECT_EQ(meshed, name != "null4" && name != "grid__a7f") << name;
    }
    ImgBank bank = ImgBank::Parse(pack.Require("e03t01.img").Payload());
    ASSERT_EQ(bank.entries.size(), 1u);
    EXPECT_EQ(bank.entries[0].first, "e03b04");
    tim8::Picture picture = tim8::Read(bank.entries[0].second, bank.magic[2] == '2');
    EXPECT_EQ(picture.width, 256);
    EXPECT_EQ(picture.height, 256);
}

TEST(PropModels, TheShadowDiscIsAFlatUnitDiscCastAsAShadow) {
    DATA_OR_SKIP(data);
    ChrPack     pack = ChrPack::Parse(BuildShadowDisc(ReadDataFile(fs::path(data) / kShadowDiscSource)));
    Bytes       cfg = pack.Require(kShadowDiscCfg).Payload();
    std::string text(cfg.begin(), cfg.end());
    EXPECT_NE(text.find("MODEL \"disc.mds\""), std::string::npos);
    EXPECT_NE(text.find("SHADOW_MODEL \"disc_shadow.mds\""), std::string::npos);
    Bytes mds = pack.Require("disc.mds").Payload();
    EXPECT_EQ(mds, pack.Require("disc_shadow.mds").Payload());
    MdtMesh mesh = MdtMesh::Parse(std::span<const unsigned char>(mds).subspan(0x80));
    ASSERT_EQ(mesh.positions.size(), static_cast<std::size_t>(kShadowDiscSegments + 1));
    for (std::size_t i = 1; i < mesh.positions.size(); i++) {
        EXPECT_NEAR(std::hypot(mesh.positions[i][0], mesh.positions[i][2]), 1.0, 1e-6);
        EXPECT_EQ(mesh.positions[i][1], 0.0);
    }
    ASSERT_EQ(mesh.submeshes.size(), 1u);
    EXPECT_EQ(mesh.submeshes[0].prim, 3);
    EXPECT_EQ(mesh.submeshes[0].records.size(), static_cast<std::size_t>(kShadowDiscSegments * 6));
    ImgBank bank = ImgBank::Parse(pack.Require("disc.img").Payload());
    ASSERT_EQ(bank.entries.size(), 1u);
    EXPECT_EQ(tim8::Read(bank.entries[0].second, false).width, kShadowDiscStandIn);
}
