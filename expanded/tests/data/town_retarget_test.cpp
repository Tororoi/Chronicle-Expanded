#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/town_model.hpp"

using namespace expanded;

namespace {

namespace fs = std::filesystem;

const TownAlly &Ally(std::string_view name) {
    for (const TownAlly &ally : TownAllies()) {
        if (ally.name == name) {
            return ally;
        }
    }
    throw std::runtime_error("no ally " + std::string(name));
}

// The ally's model built from DC_TEST_DATA, an extracted data directory.
ChrPack Built(const TownAlly &ally, const char *data) {
    DiscReader read = [data](std::string_view path) {
        std::ifstream file(fs::path(data) / path, std::ios::binary);
        return Bytes(std::istreambuf_iterator<char>(file), {});
    };
    return ChrPack::Parse(BuildTownModel(ally, read));
}

MotFile Motion(const ChrPack &pack, std::string_view name) {
    return MotFile::FromRecord(pack.Require(name));
}

double Turned(const std::array<double, 4> &a, const std::array<double, 4> &b) {
    double d = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
    return 2.0 * std::acos(std::min(1.0, d));
}

// Each boarding jump starts on the ally's idle: Toan's turns are measured from his first frame, and a borrowed joint is
// still at rest there.
TEST(TownRetarget, TheBoardingJumpStartsOnTheIdle) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    for (std::string_view name : {"Ruby", "Ungaga"}) {
        const TownAlly &ally = Ally(name);
        ASSERT_TRUE(ally.retarget) << name;
        ChrPack     pack = Built(ally, data);
        std::string stem = name == "Ruby" ? "c05a" : "e323_2c10a";
        MotFile     body = Motion(pack, stem + ".mot");
        int         start = ally.retarget->dest_lo;
        for (const MotTrack &track : body.tracks) {
            if (track.keys.empty() || (track.w2 != mot::kRotation && track.w2 != mot::kTranslation)) {
                continue;
            }
            auto idle = TrackAt(track, ally.retarget->ref), first = TrackAt(track, start);
            if (track.w2 == mot::kRotation) {
                EXPECT_LT(Turned(idle, first), 1e-3) << name << " joint " << track.w0;
            } else {
                for (int c = 0; c < 3; c++) {
                    EXPECT_NEAR(idle[std::size_t(c)], first[std::size_t(c)], 1e-3) << name << " joint " << track.w0;
                }
            }
        }
    }
}

// The jump's crouch: Toan's hips drop 5.59 at his frame 33, the ally's by that times the hip scale.
TEST(TownRetarget, TheHipsCrouchAsToansDo) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    for (auto [name, stem, hip] : {
             std::tuple{"Ruby",   "c05a.mds",       "hip"   },
             std::tuple{"Ungaga", "e323_2c10a.mds", "null84"}
    }) {
        const TownAlly &ally = Ally(name);
        ChrPack         pack = Built(ally, data);
        auto            frames = ReadMdsSkeleton(pack.Require(stem).Payload());
        int             joint = FindFrame(frames, hip)->index;
        MotFile         body = Motion(pack, std::string(stem).replace(std::string(stem).size() - 3, 3, "mot"));
        const MotTrack *track = body.TrackBy(std::uint32_t(joint), mot::kTranslation);
        ASSERT_NE(track, nullptr) << name;
        double drop = TrackAt(*track, ally.retarget->ref)[1] - TrackAt(*track, ally.retarget->dest_lo + 8)[1];
        EXPECT_NEAR(drop, 5.59 * ally.retarget->hip_scale, 0.01) << name;
    }
}

} // namespace
