#include <gtest/gtest.h>

#include <bit>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/town_model.hpp"
#include "test_support.hpp"

using namespace expanded;

namespace {

namespace fs = std::filesystem;

struct Key {
    std::uint32_t frame;
    float         value;
};

struct Track {
    std::uint32_t    joint;
    std::uint32_t    channel;
    std::vector<Key> keys;
};

// A pack record as the disc's: name, payload offset 0x50, size, stride padded to 16, tag.
Bytes Record(const std::string &name, const Bytes &payload) {
    std::size_t stride = (0x50 + payload.size() + 15) & ~std::size_t{15};
    Bytes       record(stride);
    std::copy(name.begin(), name.end(), record.begin());
    Set32(record, 0x40, 0x50);
    Set32(record, 0x44, static_cast<std::uint32_t>(payload.size()));
    Set32(record, 0x48, static_cast<std::uint32_t>(stride));
    Set32(record, 0x4C, 0x12EDD4);
    std::copy(payload.begin(), payload.end(), record.begin() + 0x50);
    return record;
}

Bytes Mds(const std::vector<std::string> &frames) {
    Bytes mds(0x18 + frames.size() * 0x70);
    Set32(mds, 0x08, static_cast<std::uint32_t>(frames.size()));
    Set32(mds, 0x14, 0x70);
    for (std::size_t i = 0; i < frames.size(); i++) {
        std::copy(frames[i].begin(), frames[i].end(), mds.begin() + 0x18 + i * 0x70);
    }
    return mds;
}

Bytes Mot(const std::vector<Track> &tracks) {
    MotFile mot;
    for (const Track &track : tracks) {
        MotTrack &added = mot.tracks.emplace_back(MotTrack{.w0 = track.joint, .w2 = track.channel, .w3 = 0x20});
        for (Key key : track.keys) {
            MotKeyframe &frame = added.keys.emplace_back();
            frame.SetFrame(key.frame);
            for (int i = 0; i < 4; i++) {
                frame.SetValue(i, key.value);
            }
        }
    }
    return mot.BuildPayload();
}

Bytes Cfg(const std::string &name) {
    std::string text = std::format("MODEL \"{0}.mds\"\r\n\r\nMOTION 0, \"{0}.mot\", \"{0}.bbp\", \"{0}.wgt\"\r\n"
                                   "KEY_START 0\r\nKEY\t10,\t20,\t0.1, //old\r\nMOTION_END\r\n",
                                   name);
    return Bytes(text.begin(), text.end());
}

Bytes Pack(const std::string &name, const std::vector<std::string> &frames, const std::vector<Track> &tracks) {
    Bytes pack;
    for (const Bytes &record : {Record(name + ".mds", Mds(frames)), Record(name + ".mot", Mot(tracks)),
                                Record("info.cfg", Cfg(name))}) {
        pack.insert(pack.end(), record.begin(), record.end());
    }
    pack.resize(pack.size() + 0x50);
    return pack;
}

// The keys of a joint's channel as (frame, first value).
std::vector<std::pair<std::uint32_t, float>> KeysOf(MotFile &mot, std::uint32_t joint, std::uint32_t channel) {
    std::vector<std::pair<std::uint32_t, float>> keys;
    for (const MotKeyframe &key : mot.TrackBy(joint, channel)->keys) {
        keys.push_back({key.Frame(), key.Value(0)});
    }
    return keys;
}

std::string Text(const Bytes &bytes) {
    return std::string(bytes.begin(), bytes.end());
}

} // namespace

TEST(TownModelPack, RebuildsByteForByte) {
    Bytes   blob = Pack("a", {
                                 "root", "hip"
    },
                        {{0, 0, {{1, 0.5f}, {9, 1.0f}}}, {1, 2, {{1, 2.0f}}}});
    ChrPack pack = ChrPack::Parse(blob);
    ASSERT_EQ(pack.records.size(), 3u);
    EXPECT_EQ(pack.Require("chara\\A.MOT").name, "a.mot");
    EXPECT_EQ(pack.Rebuild(), blob);

    ChrRecord &mot = pack.Require("a.mot");
    EXPECT_EQ(MotFile::FromRecord(mot).BuildPayload(), mot.Payload());
    EXPECT_EQ(Get32(mot.Payload(), 0x14), MotTrack::kHeaderSize + 2 * MotKeyframe::kSize);
    EXPECT_EQ(Get32(mot.Payload(), 0x60 + 0x14), 0u);
}

TEST(TownModelPack, GrownPayloadKeepsTheStride16Aligned) {
    ChrPack pack = ChrPack::Parse(Pack("a", {
                                                "root"
    },
                                       {{0, 0, {{1, 0.0f}}}}));
    pack.Require("info.cfg").ReplacePayload(Bytes(37, 'x'));
    ChrRecord &cfg = pack.Require("info.cfg");
    EXPECT_EQ(cfg.size, 37u);
    EXPECT_EQ(cfg.stride, 0x80u);
    EXPECT_EQ(Get32(cfg.raw, 0x48), 0x80u);
    EXPECT_EQ(Get32(cfg.raw, 0x4C), 0x12EDD4u);
}

TEST(TownModelPack, SpliceRemapsJointsByName) {
    MotFile dest = MotFile::FromRecord(ChrPack::Parse(Pack("d", {
                                                                    "root", "hip", "head"
    },
                                                           {{1, 0, {{10, 1.0f}, {35, 1.0f}, {90, 1.0f}}}, {2, 0, {{10, 2.0f}, {90, 2.0f}}}}))
                                           .Require("d.mot"));
    MotFile src = MotFile::FromRecord(
        ChrPack::Parse(Pack("s", {
                                     "root2", "head", "hip", "tail"
    },
                            {{1, 0, {{99, 7.0f}, {100, 8.0f}, {110, 9.0f}}}, {3, 0, {{100, 5.0f}}}}))
            .Require("s.mot"));
    SpliceByJoint(dest, src, {"root2", "head", "hip", "tail"}, {"root", "hip", "head"}, 100, 110, 30, 40);
    // The source's head (1) lands on the dest's head (2), shifted to 30; the hip keeps its keys
    // outside the window and the tail, which the dest lacks, is dropped.
    EXPECT_EQ(KeysOf(dest, 2, 0), (std::vector<std::pair<std::uint32_t, float>>{
                                      {10, 2.0f},
                                      {30, 8.0f},
                                      {40, 9.0f},
                                      {90, 2.0f}
    }));
    EXPECT_EQ(KeysOf(dest, 1, 0), (std::vector<std::pair<std::uint32_t, float>>{
                                      {10, 1.0f},
                                      {35, 1.0f},
                                      {90, 1.0f}
    }));
}

TEST(TownModelPack, RewritesTheKeyTable) {
    std::vector<TownSlot> slots = {
        {.idx = 1, .lo = 60, .hi = 80, .speed = 0.5, .name = "a-very-long-name"},
        {.idx = 0, .lo = 10, .hi = 20, .speed = 0.1, .name = "idle"            },
    };
    EXPECT_EQ(Text(RewriteTownKeys(Cfg("a"), slots)),
              "MODEL \"a.mds\"\r\n\r\nMOTION 0, \"a.mot\", \"a.bbp\", \"a.wgt\"\r\nKEY_START 0\r\n"
              "KEY\t10,\t20,\t0.10, //idle\r\nKEY\t60,\t80,\t0.50, //a-very-long-\r\nMOTION_END\r\n");
}

TEST(TownModel, GraftsReversesAndOffsetsSlots) {
    std::map<std::string, Bytes, std::less<>> disc = {
        {"base.chr", Pack("b", {"root", "hip", "head"},
         {{0, 2, {{10, 0.0f}, {60, 1.0f}, {80, 3.0f}}},
                           {1, 0, {{10, 1.0f}, {60, 1.0f}, {80, 1.0f}}},
                           {2, 0, {{10, 2.0f}, {60, 2.0f}, {70, 4.0f}, {80, 2.0f}}}})                        },
        {"src.chr",  Pack("s", {"root2", "head", "hip"}, {{1, 0, {{100, 0.25f}, {104, 0.5f}, {110, 0.75f}}}})},
    };
    TownAlly ally = {
        .name = "Test",
        .file = "test.chr",
        .base = "base.chr",
        .slots = {
                  {.idx = 0, .lo = 10, .hi = 20, .speed = 0.1, .name = "idle"},
                  {.idx = 1, .lo = 30, .hi = 40, .speed = 0.5, .name = "run", .src = "src.chr", .win = {100, 110}},
                  {.idx = 2, .lo = 60, .hi = 80, .speed = 0.3, .name = "walk", .root_offset = std::array{0.0, 0.0, -2.0}},
                  {.idx = 3, .lo = 60, .hi = 80, .speed = 0.3, .name = "back", .reverse = true},
                  },
    };
    ChrPack model = ChrPack::Parse(BuildTownModel(ally, [&](std::string_view path) { return disc.at(std::string(path)); }));
    MotFile mot = MotFile::FromRecord(model.Require("b.mot"));
    using Keys = std::vector<std::pair<std::uint32_t, float>>;
    EXPECT_EQ(KeysOf(mot, 2, 0), (Keys{
                                     {10, 2.0f },
                                     {30, 0.25f},
                                     {34, 0.5f },
                                     {40, 0.75f},
                                     {60, 2.0f },
                                     {70, 4.0f },
                                     {80, 2.0f }
    }));
    EXPECT_EQ(KeysOf(mot, 1, 0), (Keys{
                                     {10, 1.0f},
                                     {60, 1.0f},
                                     {80, 1.0f}
    }));
    // The root's translation, moved back 2 on z in the window, then played backwards.
    MotTrack *root = mot.TrackBy(0, 2);
    ASSERT_EQ(root->keys.size(), 3u);
    EXPECT_EQ(root->keys[1].Frame(), 60u);
    EXPECT_EQ(root->keys[1].Value(0), 3.0f);
    EXPECT_EQ(root->keys[1].Value(2), 1.0f);
    EXPECT_EQ(root->keys[2].Frame(), 80u);
    EXPECT_EQ(root->keys[2].Value(2), -1.0f);
    EXPECT_EQ(Text(model.Require("info.cfg").Payload()).substr(Text(model.Require("info.cfg").Payload()).find("KEY_START")),
              "KEY_START 0\r\nKEY\t10,\t20,\t0.10, //idle\r\nKEY\t30,\t40,\t0.50, //run\r\n"
              "KEY\t60,\t80,\t0.30, //walk\r\nKEY\t60,\t80,\t0.30, //back\r\nMOTION_END\r\n");
    EXPECT_EQ(model.Require("info.cfg").stride % 16, 0u);
}

TEST(TownModel, SkipsAnAllyWhoseBaseIsMissing) {
    expanded::test::ScratchDir dir("town_model");
    EXPECT_EQ(RunDataBuilds(dir.path(), "town model", TownModelBuilds(), nullptr), 0);
    EXPECT_FALSE(fs::exists(dir.path() / "expanded"));
}
