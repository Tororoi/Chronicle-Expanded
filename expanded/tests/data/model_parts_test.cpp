#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <stdexcept>

#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace {

using expanded::Bytes;

void Put32(Bytes &data, std::size_t at, std::uint32_t value) {
    expanded::Set32(data, at, value);
}

void PutF(Bytes &data, std::size_t at, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    Put32(data, at, bits);
}

// An 8-bit 16 x 16 TIM2: pixels i % 4, a CLUT of four greys at their CSM1 places.
Bytes Picture() {
    Bytes tim(0x10 + 0x30 + 256 + 1024, 0);
    std::memcpy(tim.data(), "TIM2", 4);
    Put32(tim, 0x10, 0x30 + 256 + 1024);
    Put32(tim, 0x14, 1024);
    Put32(tim, 0x18, 256);
    tim[0x1C] = 0x30;
    tim[0x23] = 5;
    tim[0x24] = 16;
    tim[0x26] = 16;
    Bytes rows(256);
    for (int i = 0; i < 256; i++) {
        rows[std::size_t(i)] = static_cast<unsigned char>(i % 4);
    }
    Bytes blocks = expanded::tim8::Swizzle(rows, 16, 16);
    std::copy(blocks.begin(), blocks.end(), tim.begin() + 0x40);
    for (int k = 0; k < 4; k++) {
        std::size_t at = 0x40 + 256 + std::size_t(expanded::tim8::Csm1(k)) * 4;
        tim[at] = tim[at + 1] = tim[at + 2] = static_cast<unsigned char>(40 * k);
        tim[at + 3] = 0x80;
    }
    return tim;
}

} // namespace

TEST(ModelParts, SwizzleRoundTrips) {
    Bytes rows(128 * 64);
    for (std::size_t i = 0; i < rows.size(); i++) {
        rows[i] = static_cast<unsigned char>(i * 7);
    }
    Bytes blocks = expanded::tim8::Swizzle(rows, 128, 64);
    EXPECT_NE(blocks, rows);
    EXPECT_EQ(expanded::tim8::Unswizzle(blocks, 128, 64), rows);
    EXPECT_EQ(expanded::tim8::Csm1(8), 16);
    EXPECT_EQ(expanded::tim8::Csm1(16), 8);
    EXPECT_EQ(expanded::tim8::Csm1(24), 24);
}

TEST(ModelParts, TextureBankRoundTrips) {
    expanded::ImgBank bank;
    std::memcpy(bank.magic.data(), "IM2", 4);
    bank.entries.emplace_back("one", Picture());
    bank.entries.emplace_back("two", Bytes(48, 7));
    Bytes             built = bank.Build();
    expanded::ImgBank read = expanded::ImgBank::Parse(built);
    ASSERT_EQ(read.entries.size(), 2u);
    EXPECT_EQ(read.entries[0].first, "one");
    EXPECT_EQ(read.Block("two").size(), 48u);
    EXPECT_EQ(read.Build(), built);
    EXPECT_THROW(read.Block("three"), std::runtime_error);
}

TEST(ModelParts, PictureReadsAndResamples) {
    Bytes                   tim = Picture();
    expanded::tim8::Picture p = expanded::tim8::Read(tim, true);
    EXPECT_EQ(p.width, 16);
    EXPECT_EQ(p.pixels[5], 1);
    Bytes                   small = expanded::tim8::Resample(tim, true, 8);
    expanded::tim8::Picture q = expanded::tim8::Read(small, false);
    EXPECT_EQ(q.width, 8);
    EXPECT_EQ(q.pixels.size(), 64u);
    EXPECT_EQ(q.clut, p.clut);
}

TEST(ModelParts, MeshRoundTripsAndKeepsItsBlocks) {
    // One triangle list: 3 positions, 3 normals, 3 coordinates, one material.
    Bytes mdt(0x40, 0);
    std::memcpy(mdt.data(), "MDT", 3);
    auto vectors = [&](int count, float base) {
        std::size_t at = mdt.size();
        mdt.resize(at + std::size_t(count) * 16);
        for (int i = 0; i < count; i++) {
            for (int k = 0; k < 4; k++) {
                PutF(mdt, at + std::size_t(i * 16 + k * 4), base + float(i) + float(k) / 10);
            }
        }
        return at;
    };
    std::size_t pos = vectors(3, 1), normals = vectors(3, 10);
    std::size_t dl = mdt.size();
    mdt.resize(dl + 0x10 + 0xC + 3 * 12, 0);
    Put32(mdt, dl + 8, 1);
    Put32(mdt, dl + 0x10, 3);
    Put32(mdt, dl + 0x14, 3);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            Put32(mdt, dl + 0x1C + std::size_t(r * 12 + c * 4), std::uint32_t(r));
        }
    }
    std::size_t uv = vectors(3, 20);
    std::size_t mat = mdt.size();
    mdt.resize(mat + 0x60, 9);
    Put32(mdt, 8, std::uint32_t(mdt.size()));
    Put32(mdt, 0xC, 3);
    Put32(mdt, 0x10, std::uint32_t(pos));
    Put32(mdt, 0x18, std::uint32_t(normals));
    Put32(mdt, 0x24, std::uint32_t(0x10 + 0xC + 3 * 12));
    Put32(mdt, 0x28, std::uint32_t(dl));
    Put32(mdt, 0x30, std::uint32_t(uv));
    Put32(mdt, 0x38, std::uint32_t(mat));
    expanded::MdtMesh mesh = expanded::MdtMesh::Parse(mdt);
    EXPECT_EQ(mesh.positions.size(), 3u);
    EXPECT_EQ(mesh.uvs.size(), 3u);
    EXPECT_FLOAT_EQ(float(mesh.normals[2][1]), 12.1f);
    EXPECT_EQ(mesh.Build(), mdt);
    mesh.positions[0][0] = 5.0;
    expanded::MdtMesh moved = expanded::MdtMesh::Parse(mesh.Build());
    EXPECT_EQ(moved.positions[0][0], 5.0);
}

TEST(ModelParts, RotationTakesYOntoADirection) {
    expanded::Vec3 d = expanded::Unit({1.0, 1.0, 0.0});
    expanded::Vec3 y = expanded::MatVec(expanded::RotationYTo(d), {0.0, 1.0, 0.0});
    for (int i = 0; i < 3; i++) {
        EXPECT_NEAR(y[std::size_t(i)], d[std::size_t(i)], 1e-12);
    }
}

TEST(SheetLook, SectionsSlideTheirColoursAndSetOnes) {
    // One section: its entry 1 set to red, the rest twice as bright.
    expanded::SheetLook     look = expanded::SheetLook::Parse(R"({"sections": [{"bri": 2, "set": {"1": "#ff0000"}}]})");
    Bytes                   out = look.ApplyToSheet(Picture());
    expanded::tim8::Picture p = expanded::tim8::Read(out, true);
    auto                    colour = [&](int texel) {
        std::size_t at = std::size_t(expanded::tim8::Csm1(p.pixels[std::size_t(texel)])) * 4;
        return std::array<int, 3>{p.clut[at], p.clut[at + 1], p.clut[at + 2]};
    };
    EXPECT_EQ(colour(1), (std::array<int, 3>{255, 0, 0}));
    EXPECT_EQ(colour(2), (std::array<int, 3>{160, 160, 160}));
    EXPECT_EQ(colour(0), (std::array<int, 3>{0, 0, 0}));
}

TEST(ModelParts, DataPastTheEndThrows) {
    Bytes mds(0x20, 0);
    Put32(mds, 0x10 + 8, 0x1000); // a mesh claiming more than there is
    EXPECT_THROW(expanded::MdtAt(mds, 0x10), std::runtime_error);

    Bytes bank(0x40, 0);
    std::memcpy(bank.data(), "IMG", 4);
    Put32(bank, 4, 1);
    Put32(bank, 0x10 + 0x20, 0x400); // an entry past the bank
    EXPECT_THROW(expanded::ImgBank::Parse(bank), std::runtime_error);
}

TEST(SheetLook, AMapNamingAMissingSectionIsRefused) {
    // One section; the map's one run names section 1.
    const char *json = R"({"sections":[{"hue":0,"sat":1,"bri":1,"con":1,"light":0,"keep":[],"set":{}}],"map":{"w":1,"h":1,"rle":"AQE="}})";
    EXPECT_THROW(expanded::SheetLook::Parse(json), std::runtime_error);
}
