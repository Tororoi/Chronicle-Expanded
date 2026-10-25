#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/spirit_cat.hpp"
#include "test_support.hpp"

using namespace expanded;

namespace {

namespace fs = std::filesystem;

struct Track {
    std::uint32_t                                joint;
    std::uint32_t                                channel;
    std::vector<std::pair<std::uint32_t, float>> keys;
};

std::string Str(const Bytes &bytes) {
    return std::string(bytes.begin(), bytes.end());
}

std::string Mds(const std::vector<std::string> &frames) {
    Bytes mds(0x18 + frames.size() * 0x70);
    Set32(mds, 0x08, static_cast<std::uint32_t>(frames.size()));
    Set32(mds, 0x14, 0x70);
    for (std::size_t i = 0; i < frames.size(); i++) {
        std::copy(frames[i].begin(), frames[i].end(), mds.begin() + 0x18 + i * 0x70);
    }
    return Str(mds);
}

std::string Mot(const std::vector<Track> &tracks) {
    MotFile mot;
    for (const Track &track : tracks) {
        MotTrack &added = mot.tracks.emplace_back(MotTrack{.w0 = track.joint, .w2 = track.channel, .w3 = 0x20});
        for (auto [frame, value] : track.keys) {
            MotKeyframe &key = added.keys.emplace_back();
            key.SetFrame(frame);
            key.SetValue(0, value);
        }
    }
    return Str(mot.BuildPayload());
}

Bytes Pack(const std::vector<std::pair<std::string, std::string>> &records) {
    ChrPack pack;
    for (const auto &[name, payload] : records) {
        pack.records.push_back(ChrRecord::Make(name, Bytes(payload.begin(), payload.end())));
    }
    pack.trailer.assign(0x50, 0);
    return pack.Rebuild();
}

// The s86 cat: its head joint keyed through its clips, up to frame 230, and a shadow model.
Bytes Cat() {
    return Pack({
        {"c04cat.bbp",  "bind"                                                                                 },
        {"c04cat.img",  "pictures"                                                                             },
        {"c04cat.mds",  Mds({"root", "kosibone", "kao"})                                                       },
        {"c04cat.mot",  Mot({{0, 0, {{1, 1.0f}, {230, 1.0f}}}, {2, 0, {{1, 2.0f}, {100, 3.0f}, {230, 2.0f}}}}) },
        {"c04cat.wgt",  "weights"                                                                              },
        {"c04cats.mds", Mds({"root"})                                                                          },
        {"c04cats.mot", Mot({{0, 0, {{1, 1.0f}}}})                                                             },
        {"info.cfg",    "MODEL \"c04cat.mds\"\r\nSHADOW_MODEL \"c04cats.mds\"\r\nKEY_START 0\r\nMOTION_END\r\n"},
    });
}

// The e01 cat: its joints in another order, the head keyed over its float-up (160-169) and either side of it.
Bytes Town() {
    return Pack({
        {"e04c04cat.mds", Mds({"dummy", "kao", "kosibone"})                                                             },
        {"e04c04cat.mot", Mot({{1, 0, {{150, 7.0f}, {160, 8.0f}, {165, 9.0f}, {169, 10.0f}, {175, 11.0f}}}, {2, 2, {}}})},
    });
}

std::vector<std::pair<std::uint32_t, float>> KeysOf(const ChrPack &pack, std::uint32_t joint) {
    std::vector<std::pair<std::uint32_t, float>> keys;
    for (const MotKeyframe &key : MotFile::FromRecord(pack.Require("c04cat.mot")).TrackBy(joint, 0)->keys) {
        keys.emplace_back(key.Frame(), key.Value(0));
    }
    return keys;
}

} // namespace

TEST(SpiritCatModel, GraftsTheFloatUpByJointNameAt285) {
    ChrPack cat = ChrPack::Parse(BuildSpiritCatModel(Cat(), Town()));
    EXPECT_EQ(KeysOf(cat, 2), (std::vector<std::pair<std::uint32_t, float>>{
                                  {1,   2.0f },
                                  {100, 3.0f },
                                  {230, 2.0f },
                                  {285, 8.0f },
                                  {290, 9.0f },
                                  {294, 10.0f}
    }));
    EXPECT_EQ(KeysOf(cat, 0), (std::vector<std::pair<std::uint32_t, float>>{
                                  {1,   1.0f},
                                  {230, 1.0f}
    }));
}

TEST(SpiritCatModel, KeepsTheCatWithoutItsShadowAndGivesTheModsKeys) {
    ChrPack                  source = ChrPack::Parse(Cat());
    ChrPack                  cat = ChrPack::Parse(BuildSpiritCatModel(Cat(), Town()));
    std::vector<std::string> names;
    for (const ChrRecord &record : cat.records) {
        names.push_back(record.name);
    }
    EXPECT_EQ(names, (std::vector<std::string>{"info.cfg", "c04cat.mds", "c04cat.img", "c04cat.bbp", "c04cat.wgt", "c04cat.mot"}));
    for (const char *name : {"c04cat.mds", "c04cat.img", "c04cat.bbp", "c04cat.wgt"}) {
        EXPECT_EQ(cat.Require(name).Payload(), source.Require(name).Payload()) << name;
    }
    EXPECT_EQ(Str(cat.Require(kSpiritCatCfg).Payload()), "//Divine Beast Title spirit cat\r\n"
                                                         "IMG 0,\"c04cat.img\"\r\n"
                                                         "IMG_END\r\n"
                                                         "MATERIAL_ANIME 0\r\n"
                                                         "VERTEX_ANIME 1\r\n"
                                                         "SHADOW_VERTEX_ANIME 0\r\n"
                                                         "\r\n"
                                                         "ALLOC_DBUFF \"skin\"\r\n"
                                                         "MODEL \"c04cat.mds\"\r\n"
                                                         "BODY_SIZE 3,7,60\r\n"
                                                         "\r\n"
                                                         "MOTION 0, \"c04cat.mot\", \"c04cat.bbp\", \"c04cat.wgt\"\r\n"
                                                         "KEY_START 64\r\n"
                                                         "\tKEY\t10,\t20,\t0.1,\t\t//64 cat stand\r\n"
                                                         "\tKEY\t95,\t105,\t0.4,\t\t//65 cat ready\r\n"
                                                         "\tKEY\t120,\t136,\t0.5,\t\t//66 cat run\r\n"
                                                         "\tKEY\t190,\t204,\t0.5,\t\t//67 cat take-off\r\n"
                                                         "\tKEY\t205,\t214,\t0.5,\t\t//68 cat leap\r\n"
                                                         "\tKEY\t215,\t227,\t0.36,\t\t//69 cat land\r\n"
                                                         "\tKEY\t60,\t80,\t1.0,\t\t//70 cat walk\r\n"
                                                         "\tKEY\t285,\t294,\t0.75,\t\t//71 cat float-up\r\n"
                                                         "\tKEY\t30,\t40,\t0.1,\t\t//72 cat sit\r\n"
                                                         "MOTION_END\r\n");
    EXPECT_EQ(kCatFloat, 71);
    EXPECT_EQ(kCatSit, 72);
}

TEST(SpiritCatModel, RefusesAFloatUpThatGraftsNothing) {
    Bytes town = Pack({
        {"e04c04cat.mds", Mds({"dummy", "tail"})                   },
        {"e04c04cat.mot", Mot({{1, 0, {{160, 1.0f}, {169, 1.0f}}}})},
    });
    EXPECT_THROW(BuildSpiritCatModel(Cat(), town), std::runtime_error);
}

TEST(SpiritCatModel, SkipsDataWithoutTheCats) {
    expanded::test::ScratchDir dir("spirit_cat");
    EXPECT_EQ(BuildSpiritCatModels(dir.path(), nullptr), 0);
    EXPECT_FALSE(fs::exists(dir.path() / "expanded"));
}
