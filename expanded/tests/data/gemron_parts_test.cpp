#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/data/gemron_parts.hpp"

namespace {

using expanded::Bytes;

std::string Text(const expanded::ChrRecord &record) {
    Bytes payload = record.Payload();
    return std::string(payload.begin(), payload.end());
}

expanded::ChrPack CfgPack(const std::string &cfg) {
    expanded::ChrPack pack;
    pack.records.push_back(expanded::ChrRecord::Make("info.cfg", Bytes(cfg.begin(), cfg.end())));
    return pack;
}

// KEY 0 to KEY 12, each with its comment.
std::string Keys() {
    std::string cfg = "MATERIAL_ANIME 0\r\nKEY_START 0\r\n";
    for (int k = 0; k <= 12; k++) {
        cfg += "KEY\t1,\t2,\t0.5,\t// " + std::to_string(k) + "\r\n";
    }
    return cfg + "MOTION_END\r\n";
}

expanded::MotTrack Track(std::initializer_list<std::pair<std::uint32_t, float>> keys) {
    expanded::MotTrack track;
    for (auto [frame, value] : keys) {
        track.keys.push_back(expanded::MakeKey(frame, {value, 0, 0, 0}));
    }
    track.w2 = 2;
    return track;
}

} // namespace

TEST(GemronParts, TheDeathKeysMoveToTheCopyAndMaterialAnimationTurnsOn) {
    expanded::ChrPack pack = CfgPack(Keys());
    expanded::DeathCfg(pack, 200, 229, nullptr);
    std::string cfg = Text(pack.Require("info.cfg"));
    EXPECT_NE(cfg.find("MATERIAL_ANIME 1\r"), std::string::npos);
    EXPECT_NE(cfg.find("KEY\t200,\t229,\t0.35,\t// 11\r"), std::string::npos);
    EXPECT_NE(cfg.find("KEY\t229,\t229,\t0.0,\t// 12\r"), std::string::npos);
    EXPECT_NE(cfg.find("KEY\t1,\t2,\t0.5,\t// 10\r"), std::string::npos); // the others as they were
}

TEST(GemronParts, AScrollNeedsATextureAnimationToFollow) {
    expanded::ChrPack pack = CfgPack(Keys());
    EXPECT_THROW(expanded::DeathCfg(pack, 200, 229, "TEX_SCROLL_DATA"), std::runtime_error);
}

TEST(GemronParts, TheDeathIsCopiedFrameByFrameAndHeld) {
    expanded::MotFile mot;
    mot.tracks.push_back(Track({
        {105, 0.0f },
        {125, 20.0f}
    }));
    expanded::DeathClip(mot, 200, 105, 125, 229);
    const auto &keys = mot.tracks[0].keys;
    ASSERT_EQ(keys.size(), 2u + 21u + 2u);
    EXPECT_EQ(keys[2].Frame(), 200u);
    EXPECT_FLOAT_EQ(keys[2 + 10].Value(0), 10.0f); // frame 210 is the vanilla 115
    EXPECT_EQ(keys[keys.size() - 2].Frame(), 229u);
    EXPECT_FLOAT_EQ(keys.back().Value(0), 20.0f);
    EXPECT_EQ(keys.back().Frame(), 230u);
}

TEST(GemronParts, ATrackAlreadyPastTheCopyIsRefused) {
    expanded::MotFile mot;
    mot.tracks.push_back(Track({
        {0,   0.0f},
        {250, 1.0f}
    }));
    EXPECT_THROW(expanded::DeathClip(mot, 200, 105, 125, 229), std::runtime_error);
}

TEST(GemronScripts, AScriptNotHolyGemronsIsRefused) {
    Bytes stb(0x40, 0);
    expanded::Set32(stb, 8, 0x20);   // code base
    expanded::Set32(stb, 0xC, 0x20); // label table
    expanded::Set32(stb, 0x10, 0);   // no labels
    EXPECT_THROW(expanded::BuildCrystalGemronScript(stb), std::runtime_error);
    EXPECT_THROW(expanded::BuildAtlaGemronScript(stb), std::runtime_error);
}
