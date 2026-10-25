#include "expanded/data/spirit_cat.hpp"

#include <stdexcept>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"

namespace expanded {

namespace {

struct Key {
    int         first;
    int         last;
    const char *speed;
    const char *name;
};

// From kCatStand on.
constexpr Key kKeys[] = {
    {10,                   20,                       "0.1",  "stand"   },
    {95,                   105,                      "0.4",  "ready"   },
    {120,                  136,                      "0.5",  "run"     },
    {190,                  204,                      "0.5",  "take-off"},
    {205,                  214,                      "0.5",  "leap"    },
    {215,                  227,                      "0.36", "land"    },
    {60,                   80,                       "1.0",  "walk"    },
    {kSpiritCatFloatFrame, kSpiritCatFloatFrame + 9, "0.75", "float-up"},
    {30,                   40,                       "0.1",  "sit"     },
};

std::string Cfg() {
    std::string cfg = "//Divine Beast Title spirit cat\r\n"
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
                      "KEY_START " +
                      std::to_string(kCatStand) + "\r\n";
    int         id = kCatStand;
    for (const Key &key : kKeys) {
        cfg += "\tKEY\t" + std::to_string(key.first) + ",\t" + std::to_string(key.last) + ",\t" + key.speed + ",\t\t//" +
               std::to_string(id++) + " cat " + key.name + "\r\n";
    }
    return cfg + "MOTION_END\r\n";
}

} // namespace

Bytes BuildSpiritCatModel(std::span<const unsigned char> cat, std::span<const unsigned char> town) {
    ChrPack source = ChrPack::Parse(cat);
    ChrPack float_source = ChrPack::Parse(town);
    MotFile mot = MotFile::FromRecord(source.Require("c04cat.mot"));
    MotFile float_mot = MotFile::FromRecord(float_source.Require("e04c04cat.mot"));
    SpliceByJoint(mot, float_mot, ReadMdsFrames(float_source.Require("e04c04cat.mds").Payload()),
                  ReadMdsFrames(source.Require("c04cat.mds").Payload()), kSpiritCatFloatFirst, kSpiritCatFloatLast,
                  kSpiritCatFloatFrame, kSpiritCatFloatFrame + (kSpiritCatFloatLast - kSpiritCatFloatFirst));
    bool grafted = false;
    for (const MotTrack &track : mot.tracks) {
        for (const MotKeyframe &key : track.keys) {
            grafted = grafted || (key.Frame() >= static_cast<std::uint32_t>(kSpiritCatFloatFrame) &&
                                  key.Frame() <= static_cast<std::uint32_t>(kSpiritCatFloatFrame + kSpiritCatFloatLast - kSpiritCatFloatFirst));
        }
    }
    if (!grafted) {
        throw std::runtime_error("the float-up grafted no keys");
    }
    ChrPack     pack;
    std::string cfg = Cfg();
    pack.records.push_back(ChrRecord::Make(kSpiritCatCfg, Bytes(cfg.begin(), cfg.end())));
    for (const char *name : {"c04cat.mds", "c04cat.img", "c04cat.bbp", "c04cat.wgt"}) {
        pack.records.push_back(source.Require(name));
    }
    pack.records.push_back(ChrRecord::Make("c04cat.mot", mot.BuildPayload()));
    pack.trailer.assign(0x50, 0);
    return pack.Rebuild();
}

int BuildSpiritCatModels(const std::filesystem::path &data, std::FILE *log) {
    DataBuild build{
        "Divine Beast Title", kSpiritCatModelFile, {kSpiritCatSource, kSpiritCatFloatSource},
          [](const DiscReader &read) {
            return BuildSpiritCatModel(read(kSpiritCatSource), read(kSpiritCatFloatSource));
                                                    }
    };
    return RunDataBuilds(data, "spirit cat", {build}, log);
}

} // namespace expanded
