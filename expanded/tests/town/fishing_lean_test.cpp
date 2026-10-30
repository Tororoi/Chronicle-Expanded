#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "expanded/data/arm_reach.hpp"
#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/fishing_poses.hpp"
#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/rig_math.hpp"
#include "expanded/town/fishing_lean.hpp"

using namespace expanded;

namespace {

namespace fs = std::filesystem;

float Sum(const LeanShares &s) {
    return s.left + s.centre + s.right + s.relaxed;
}

// The motion's locals at a frame, on the model's frames (the root's place left out, as the game's mix does).
Skeleton At(const std::vector<MdsFrame> &frames, const MotFile &mot, double frame) {
    Skeleton s;
    for (const MdsFrame &f : frames) {
        Mat4 local = f.local;
        for (const MotTrack &track : mot.tracks) {
            if (track.w0 != std::uint32_t(f.index)) {
                continue;
            }
            Vec4 v = TrackAt(track, frame);
            if (track.w2 == mot::kRotation) {
                Mat3 r = QuatToMat(v);
                for (std::size_t i = 0; i < 3; i++) {
                    for (std::size_t j = 0; j < 3; j++) {
                        local[i][j] = r[i][j];
                    }
                }
            } else if (track.w2 == mot::kTranslation) {
                local[3] = {v[0], v[1], v[2], 1.0};
            }
        }
        s.parent.push_back(f.parent);
        s.local.push_back(f.parent < 0 ? MatFromRT(QuatToMat({1, 0, 0, 0}), {0, 0, 0}) : local);
    }
    return s;
}

// a t of the way to b, joint by joint, as the game's mix blends locals.
Skeleton Blend(const Skeleton &a, const Skeleton &b, double t) {
    Skeleton out = a;
    for (std::size_t j = 0; j < a.local.size(); j++) {
        Vec4 q = SlerpExact(MatToQuat(RotOf(a.local[j])), MatToQuat(RotOf(b.local[j])), t);
        Vec3 p = TransOf(a.local[j]), r = TransOf(b.local[j]);
        out.local[j] = MatFromRT(QuatToMat(q), {p[0] + (r[0] - p[0]) * t, p[1] + (r[1] - p[1]) * t, p[2] + (r[2] - p[2]) * t});
    }
    return out;
}

Vec3 WristOnRod(const Skeleton &s, const FishingGrip &grip) {
    return TransOf(MatMulExact(s.World(grip.right.wrist), RigidInv(s.World(grip.rod))));
}

double Gap(const Vec3 &a, const Vec3 &b) {
    return std::hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}

} // namespace

TEST(FishingLean, TheStickLeansTheWayItPointsOnScreen) {
    auto            near = [](LeanStick got, float x, float y) { return std::fabs(got.x - x) < 1e-4f && std::fabs(got.y - y) < 1e-4f; };
    constexpr float kPi = 3.14159265f;
    // Toan faces +z (his right -x). The camera behind him: the stick is his own (right is his right, down is back).
    EXPECT_TRUE(near(LeanStickFor(0.0f, 0.0f, 1.0f, 0.0f), 1.0f, 0.0f));
    EXPECT_TRUE(near(LeanStickFor(0.0f, 0.0f, 0.0f, 1.0f), 0.0f, 1.0f));
    EXPECT_TRUE(near(LeanStickFor(0.0f, 0.0f, -1.0f, 1.0f), -1.0f, 1.0f));
    // Behind him whichever way he faces.
    EXPECT_TRUE(near(LeanStickFor(2.0f, 2.0f, 0.0f, 1.0f), 0.0f, 1.0f));
    // The camera turned to look +x across him (the screen's right is his forward, up his left): left on the stick is
    // back, down is his right, right is forward (pushed away, no haul).
    EXPECT_TRUE(near(LeanStickFor(kPi / 2.0f, 0.0f, -1.0f, 0.0f), 0.0f, 1.0f));
    EXPECT_TRUE(near(LeanStickFor(kPi / 2.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.0f));
    EXPECT_TRUE(near(LeanStickFor(kPi / 2.0f, 0.0f, 1.0f, 0.0f), 0.0f, -1.0f));
    // The camera facing him: down (toward the camera) is forward, right on screen is his left.
    EXPECT_TRUE(near(LeanStickFor(kPi, 0.0f, 0.0f, 1.0f), 0.0f, -1.0f));
    EXPECT_TRUE(near(LeanStickFor(kPi, 0.0f, 1.0f, 0.0f), -1.0f, 0.0f));
    // Let go, nothing; pushed part way, that far.
    EXPECT_TRUE(near(LeanStickFor(1.0f, 0.3f, 0.0f, 0.0f), 0.0f, 0.0f));
    EXPECT_TRUE(near(LeanStickFor(0.0f, 0.0f, 0.0f, 0.5f), 0.0f, 0.5f));
    // A diagonal read as the game stretches it points its own way: down and right, as far back as aside.
    const LeanStick diagonal = LeanStickFor(0.0f, 0.0f, 0.6f, 0.6f);
    EXPECT_NEAR(diagonal.x, diagonal.y, 1e-4f);
    EXPECT_GT(diagonal.x, 0.0f);
}

TEST(FishingLean, TheStickSharesThePoses) {
    // At rest relaxed; pulled straight back the disc's pull; back and to a side, that side's lean.
    LeanShares rest = LeanSharesFor(0.0f, 0.0f);
    EXPECT_FLOAT_EQ(rest.relaxed, 1.0f);
    LeanShares back = LeanSharesFor(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(back.centre, 1.0f);
    EXPECT_FLOAT_EQ(LeanSharesFor(-1.0f, 1.0f).left, 1.0f);
    EXPECT_FLOAT_EQ(LeanSharesFor(1.0f, 1.0f).right, 1.0f);
    // Halfway each way: shares of both; pushed forward or past the edge, as far as the edge.
    LeanShares half = LeanSharesFor(-0.5f, 0.5f);
    EXPECT_FLOAT_EQ(half.left, 0.25f);
    EXPECT_FLOAT_EQ(half.centre, 0.25f);
    EXPECT_FLOAT_EQ(half.relaxed, 0.5f);
    EXPECT_FLOAT_EQ(LeanSharesFor(0.3f, -1.0f).relaxed, 1.0f);
    EXPECT_FLOAT_EQ(LeanSharesFor(2.0f, 3.0f).right, 1.0f);
    for (float x : {-1.0f, -0.4f, 0.0f, 0.7f, 1.0f}) {
        for (float y : {-0.5f, 0.0f, 0.3f, 1.0f}) {
            EXPECT_FLOAT_EQ(Sum(LeanSharesFor(x, y)), 1.0f) << x << ", " << y;
        }
    }
}

// DC_TEST_DATA, an extracted data directory: the poses mixed as the stick mixes them hold the rod once the right hand is
// put back, where a plain mix lets it slide.
TEST(FishingLean, TheRightHandHoldsTheRodAfterAMix) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    ChrPack               pack = ChrPack::Parse(BuildFishingPoses(ReadDataFile(fs::path(data) / kFishingModelFile)));
    std::vector<MdsFrame> frames = ReadMdsSkeleton(pack.Require("c01d_turi.mds").Payload());
    MotFile               mot = MotFile::FromRecord(pack.Require("c01d_turi.mot"));
    auto                  at = [&](const char *name) { return FindFrame(frames, name)->index; };
    const FishingGrip     grip = {
        .rod = at(kFishingRod),
        .right = {at(kFishingRightArm.collarbone), at(kFishingRightArm.shoulder), at(kFishingRightArm.elbow),
                  at(kFishingRightArm.wrist)}
    };
    const Skeleton left = At(frames, mot, 380), centre = At(frames, mot, kFishingPullFrame), right = At(frames, mot, 390);
    const Skeleton relaxed = At(frames, mot, 400);
    const Vec3     held = WristOnRod(centre, grip);
    double         slid = 0.0;
    for (auto [pulled, side] : {
             std::pair{0.5, 0.5 },
             {0.7, -0.6},
             {1.0, 0.5 },
             {0.3, -1.0}
    }) {
        Skeleton leant = Blend(centre, side < 0 ? left : right, std::abs(side));
        Skeleton mixed = Blend(relaxed, leant, pulled);
        slid = std::max(slid, Gap(WristOnRod(mixed, grip), held));
        ASSERT_TRUE(HoldTheRod(mixed, centre, grip));
        EXPECT_LT(Gap(WristOnRod(mixed, grip), held), 0.01) << pulled << ", " << side;
    }
    EXPECT_GT(slid, 0.05);
}
