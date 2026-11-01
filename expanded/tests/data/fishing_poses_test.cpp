#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/fishing_poses.hpp"
#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/rig_math.hpp"

using namespace expanded;

namespace {

namespace fs = std::filesystem;

// DC_TEST_DATA, an extracted data directory, or skipped.
#define DATA_OR_SKIP(data)                          \
    const char *data = std::getenv("DC_TEST_DATA"); \
    if (data == nullptr) {                          \
        GTEST_SKIP() << "DC_TEST_DATA is not set";  \
    }

const MotTrack &Track(const MotFile &mot, const std::vector<MdsFrame> &frames, const char *joint, std::uint32_t channel) {
    const MdsFrame *frame = FindFrame(frames, joint);
    EXPECT_NE(frame, nullptr) << joint;
    for (const MotTrack &track : mot.tracks) {
        if (track.w0 == std::uint32_t(frame->index) && track.w2 == channel) {
            return track;
        }
    }
    ADD_FAILURE() << joint << " has no track on channel " << channel;
    return mot.tracks.front();
}

// Every frame's world matrix at a frame of the motion.
std::vector<Mat4> World(const std::vector<MdsFrame> &frames, const MotFile &mot, double at) {
    std::vector<Mat4> world;
    for (const MdsFrame &f : frames) {
        Mat4 local = f.local;
        for (const MotTrack &track : mot.tracks) {
            if (track.w0 != std::uint32_t(f.index) || (track.w2 != mot::kRotation && track.w2 != mot::kTranslation)) {
                continue;
            }
            Vec4 v = TrackAt(track, at);
            if (track.w2 == mot::kRotation) {
                Mat3 r = QuatToMat(v);
                for (std::size_t i = 0; i < 3; i++) {
                    for (std::size_t j = 0; j < 3; j++) {
                        local[i][j] = r[i][j];
                    }
                }
            } else {
                local[3] = {v[0], v[1], v[2], 1.0};
            }
        }
        world.push_back(f.parent < 0 ? local : MatMulExact(local, world[std::size_t(f.parent)]));
    }
    return world;
}

// Where a joint is in the rod's frame.
Vec3 OnRod(const std::vector<MdsFrame> &frames, const std::vector<Mat4> &world, const char *joint) {
    return XformPt(RigidInv(world[std::size_t(FindFrame(frames, "turizao")->index)]),
                   TransOf(world[std::size_t(FindFrame(frames, joint)->index)]));
}

// The rod's tip at a frame: the tip (sao) through the rod (turizao), which hangs from the unposed root.
Vec3 RodTip(const MotFile &mot, const std::vector<MdsFrame> &frames, double frame) {
    Vec4 rotation = TrackAt(Track(mot, frames, "turizao", mot::kRotation), frame);
    Vec4 position = TrackAt(Track(mot, frames, "turizao", mot::kTranslation), frame);
    Vec4 tip = TrackAt(Track(mot, frames, "sao", mot::kTranslation), frame);
    return XformPt(MatFromRT(QuatToMat(rotation), {position[0], position[1], position[2]}), {tip[0], tip[1], tip[2]});
}

} // namespace

TEST(FishingPoses, TheDiscsKeysStayAndThePosesFollowThem) {
    DATA_OR_SKIP(data);
    Bytes   disc = ReadDataFile(fs::path(data) / kFishingModelFile);
    ChrPack before = ChrPack::Parse(disc);
    ChrPack after = ChrPack::Parse(BuildFishingPoses(disc));
    ASSERT_EQ(before.records.size(), after.records.size());
    for (std::size_t i = 0; i < before.records.size(); i++) {
        const ChrRecord &a = before.records[i], &b = after.records[i];
        if (a.name.ends_with(".mot")) {
            MotFile ma = MotFile::FromRecord(a), mb = MotFile::FromRecord(b);
            ASSERT_EQ(ma.tracks.size(), mb.tracks.size()) << a.name;
            for (std::size_t t = 0; t < ma.tracks.size(); t++) {
                ASSERT_EQ(mb.tracks[t].keys.size(), ma.tracks[t].keys.size() + 6) << a.name;
                for (std::size_t k = 0; k < ma.tracks[t].keys.size(); k++) {
                    EXPECT_EQ(ma.tracks[t].keys[k].raw, mb.tracks[t].keys[k].raw) << a.name;
                }
            }
        } else if (!a.name.ends_with(".cfg")) {
            EXPECT_EQ(a.raw, b.raw) << a.name;
        }
    }
    std::string cfg = ToText(after.Require("c01d_turi.cfg").Payload());
    EXPECT_NE(cfg.find("KEY\t325,\t370,\t0.3,"), std::string::npos);
    EXPECT_NE(cfg.find("KEY\t380,\t380,\t0.0,\t//pull left 16\r\n"
                       "KEY\t390,\t390,\t0.0,\t//pull right 17\r\n"
                       "KEY\t400,\t400,\t0.0,\t//relaxed 18\r\n"
                       "MOTION_END"),
              std::string::npos);
}

TEST(FishingPoses, TheRodLeansLeftAndRightAndRisesRelaxed) {
    DATA_OR_SKIP(data);
    ChrPack               pack = ChrPack::Parse(BuildFishingPoses(ReadDataFile(fs::path(data) / kFishingModelFile)));
    std::vector<MdsFrame> frames = ReadMdsSkeleton(pack.Require("c01d_turi.mds").Payload());
    MotFile               mot = MotFile::FromRecord(pack.Require("c01d_turi.mot"));
    // Toan faces +Z; his left is +X.
    Vec3 pull = RodTip(mot, frames, kFishingPullFrame), left = RodTip(mot, frames, 380), right = RodTip(mot, frames, 390);
    EXPECT_GT(left[0] - pull[0], 10.0);
    EXPECT_LT(right[0] - pull[0], -10.0);
    EXPECT_GT(left[1], 20.0);
    EXPECT_GT(right[1], 20.0);
    // Held: the frame after each pose is the pose.
    EXPECT_EQ(RodTip(mot, frames, 381), left);
    Vec3 wait = RodTip(mot, frames, kFishingWaitFrame), relaxed = RodTip(mot, frames, 400);
    EXPECT_GT(relaxed[1] - wait[1], 10.0);
}

// The angle in degrees the wrist bends: between the forearm (elbow to wrist) and the hand (wrist to its end).
double WristBend(const std::vector<MdsFrame> &frames, const std::vector<Mat4> &world, const char *elbow, const char *wrist, const char *end) {
    auto at = [&](const char *joint) { return TransOf(world[std::size_t(FindFrame(frames, joint)->index)]); };
    Vec3 forearm = Unit(Add(at(wrist), Scale(at(elbow), -1.0))), hand = Unit(Add(at(end), Scale(at(wrist), -1.0)));
    return std::acos(std::clamp(Dot(forearm, hand), -1.0, 1.0)) * 180.0 / 3.14159265358979;
}

TEST(FishingPoses, TheRightHandKeepsItsGripAndTheLeftArmIsThePulls) {
    DATA_OR_SKIP(data);
    ChrPack               pack = ChrPack::Parse(BuildFishingPoses(ReadDataFile(fs::path(data) / kFishingModelFile)));
    std::vector<MdsFrame> frames = ReadMdsSkeleton(pack.Require("c01d_turi.mds").Payload());
    MotFile               mot = MotFile::FromRecord(pack.Require("c01d_turi.mot"));
    // The disc keeps the right wrist in one place on the rod, frame to frame.
    std::vector<Mat4> pull = World(frames, mot, kFishingPullFrame), wait = World(frames, mot, kFishingWaitFrame);
    Vec3              right = OnRod(frames, pull, "jnt18_1");
    ASSERT_LT(std::hypot(right[0] - OnRod(frames, wait, "jnt18_1")[0], right[1] - OnRod(frames, wait, "jnt18_1")[1],
                         right[2] - OnRod(frames, wait, "jnt18_1")[2]),
              0.001);
    auto bend = [&](const std::vector<Mat4> &world) { return WristBend(frames, world, "jnt17_2", "jnt18_1", "eff18"); };
    // The pulls: the grip, the wrist bent as in the pull.
    for (int frame : {380, 390}) {
        std::vector<Mat4> world = World(frames, mot, frame);
        Vec3              held = OnRod(frames, world, "jnt18_1");
        EXPECT_LT(std::hypot(held[0] - right[0], held[1] - right[1], held[2] - right[2]), 0.01) << frame;
        EXPECT_NEAR(bend(world), bend(pull), 2.0) << frame;
    }
    // Relaxed: the grip, the wrist bent no further than in the wait, the left arm the pull's in the chest's frame.
    std::vector<Mat4> relaxed = World(frames, mot, 400);
    Vec3              held = OnRod(frames, relaxed, "jnt18_1");
    EXPECT_LT(std::hypot(held[0] - right[0], held[1] - right[1], held[2] - right[2]), 0.01);
    EXPECT_LE(bend(relaxed), bend(wait) + 2.0);
    auto in_chest = [&](const std::vector<Mat4> &world, const char *joint) {
        return XformPt(RigidInv(world[std::size_t(FindFrame(frames, "eff2")->index)]), TransOf(world[std::size_t(FindFrame(frames, joint)->index)]));
    };
    for (const char *joint : {"jnt9_2", "jnt10_1", "eff10"}) {
        Vec3 a = in_chest(relaxed, joint), b = in_chest(pull, joint);
        EXPECT_LT(std::hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]), 0.01) << joint;
    }
}
