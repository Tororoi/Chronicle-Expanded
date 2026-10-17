#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "expanded/data/boss_model.hpp"
#include "expanded/data/chr_pack.hpp"

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
Bytes Record(const std::string &name, const std::string &payload) {
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
        for (Key key : track.keys) {
            MotKeyframe &frame = added.keys.emplace_back();
            frame.SetFrame(key.frame);
            for (int i = 0; i < 4; i++) {
                frame.SetValue(i, key.value);
            }
        }
    }
    return Str(mot.BuildPayload());
}

std::string Cfg(const std::string &name, const std::string &keys) {
    return std::format("MODEL \"{0}.mds\"\r\n\r\nMOTION 0, \"{0}.mot\", \"{0}.bbp\", \"{0}.wgt\"\r\nKEY_START 0\r\n{1}"
                       "MOTION_END\r\n",
                       name, keys);
}

Bytes Pack(const std::vector<std::pair<std::string, std::string>> &records) {
    Bytes pack;
    for (const auto &[name, payload] : records) {
        Bytes record = Record(name, payload);
        pack.insert(pack.end(), record.begin(), record.end());
    }
    pack.resize(pack.size() + 0x50);
    return pack;
}

// Joe's eleven KEYs, up to frame 356.
std::string BattleKeys() {
    std::string keys;
    for (int i = 0; i < 11; i++) {
        keys += std::format("KEY\t{},\t{},\t0.2,\t//{}\r\n", 10 + 30 * i, 30 + 30 * i, i);
    }
    return keys;
}

Bytes Battle() {
    return Pack({
        {"info.cfg", Cfg("c16a", BattleKeys())},
        {"c16a.mds", Mds({"dummyRoot", "hip", "head", "dcol0"})},
        {"c16a.mot", Mot({{1, 0, {{1, 1.0f}, {356, 1.0f}}},
                          {2, 2, {{1, 4.0f}, {200, 5.0f}, {356, 4.0f}}},
                          {3, 2, {{1, 6.0f}, {356, 6.0f}}}})},
        {"c16a.wgt", "weights"},
    });
}

// The cutscene: the announcer's body first, then Joe's, whose fall is frames 90 to 140.
Bytes Cutscene() {
    return Pack({
        {"e406p76a.cfg", Cfg("e406p76a", "KEY\t10,\t20,\t0.2,\t//p\r\n")},
        {"e406p76a.mds", Mds({"dummyRoot", "hip"})},
        {"e406p76a.mot", Mot({{1, 0, {{90, 9.0f}, {140, 9.0f}}}})},
        {"e406c16a.cfg", Cfg("e406c16a", "KEY\t90,\t140,\t0.2,\t//fall 203\r\n")},
        {"e406c16a.mds", Mds({"dummyRoot", "head", "hip"})},
        {"e406c16a.mot", Mot({{1, 2, {{1, 0.0f}, {80, 1.0f}, {100, 2.0f}, {140, 3.0f}}},
                              {2, 0, {{1, 0.5f}, {90, 0.25f}, {115, 0.5f}, {140, 0.75f}}}})},
    });
}

using Keys = std::vector<std::pair<std::uint32_t, float>>;

Keys KeysOf(const ChrPack &pack, std::uint32_t joint, std::uint32_t channel) {
    MotFile mot = MotFile::FromRecord(pack.Require("c16a.mot"));
    Keys    keys;
    for (const MotKeyframe &key : mot.TrackBy(joint, channel)->keys) {
        keys.push_back({key.Frame(), key.Value(0)});
    }
    return keys;
}

} // namespace

TEST(JoeModel, AppendsTheFallAndItsLoopAfterHisOwnKeys) {
    Bytes   battle = Battle();
    ChrPack before = ChrPack::Parse(battle);
    ChrPack after = ChrPack::Parse(BuildJoeModel(battle, Cutscene()));

    // KEYs 0-10 stay as they were; 11 plays the fall past his last frame, 12 holds its last frame.
    std::string cfg = Str(after.Require("info.cfg").Payload());
    EXPECT_EQ(cfg, Cfg("c16a", BattleKeys() + "KEY\t360,\t410,\t0.2,\t//stagger and fall 11\r\n"
                                              "KEY\t410,\t410,\t0.0,\t//death loop 12\r\n"));

    // The mesh and weights are untouched, and every joint keeps its keys up to frame 356.
    EXPECT_EQ(after.Require("c16a.mds").raw, before.Require("c16a.mds").raw);
    EXPECT_EQ(after.Require("c16a.wgt").raw, before.Require("c16a.wgt").raw);
    EXPECT_EQ(KeysOf(after, 3, 2), KeysOf(before, 3, 2));

    // The cutscene's hip lands on the battle hip by name, shifted from 90 to 360.
    EXPECT_EQ(KeysOf(after, 1, 0), (Keys{
                                       {1,   1.0f },
                                       {356, 1.0f },
                                       {360, 0.25f},
                                       {385, 0.5f },
                                       {410, 0.75f}
    }));
    // The head, keyed at the window's edges from the cutscene's pose there.
    EXPECT_EQ(KeysOf(after, 2, 2), (Keys{
                                       {1,   4.0f},
                                       {200, 5.0f},
                                       {356, 4.0f},
                                       {360, 1.5f},
                                       {370, 2.0f},
                                       {410, 3.0f}
    }));
}

TEST(JoeModel, AppendsKeysBeforeTheMotionEnd) {
    std::string cfg = Cfg("a", "KEY\t1,\t2,\t0.1,\t//a\r\n") + "EVENT 1,1,1\r\n";
    EXPECT_EQ(Str(AppendKeys(Bytes(cfg.begin(), cfg.end()), "KEY\t3,\t4,\t0.1,\t//b\r\n")),
              Cfg("a", "KEY\t1,\t2,\t0.1,\t//a\r\nKEY\t3,\t4,\t0.1,\t//b\r\n") + "EVENT 1,1,1\r\n");
    std::string none = "MODEL \"a.mds\"\r\n";
    EXPECT_THROW(AppendKeys(Bytes(none.begin(), none.end()), "KEY\r\n"), std::runtime_error);
}

TEST(JoeModel, SkipsDataWithoutHisPacks) {
    fs::path dir = fs::temp_directory_path() / std::format("dc_boss_model_{}", std::rand());
    fs::create_directories(dir);
    EXPECT_EQ(BuildBossModels(dir, nullptr), 0);
    EXPECT_FALSE(fs::exists(dir / "expanded"));
    fs::remove_all(dir);
}
