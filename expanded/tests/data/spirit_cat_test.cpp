#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "expanded/data/cat_rig.hpp"
#include "expanded/data/chr_pack.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/spirit_cat.hpp"
#include "test_support.hpp"

using namespace expanded;
using namespace expanded::rig;

namespace {

namespace fs = std::filesystem;

} // namespace

// The numbers the cat was authored with are Python's, so the bake's are too (the values below are CPython 3.12's).
TEST(CatRig, SumsAsPythonDoes) {
    EXPECT_EQ(Sum({1e100, 1.0, -1e100}), 1.0);
    EXPECT_EQ(Sum(std::vector<double>(10, 0.1)), 1.0);
    EXPECT_EQ(Sum(std::vector<double>{}), 0.0);
}

TEST(CatRig, RoundsAsPythonDoes) {
    EXPECT_EQ(Round(2.5, 0), 2.0);
    EXPECT_EQ(Round(3.5, 0), 4.0);
    EXPECT_EQ(Round(0.125, 2), 0.12);
    EXPECT_EQ(Round(0.135, 2), 0.14);
    EXPECT_EQ(Round(2.675, 2), 2.67);
    EXPECT_EQ(Round(1.0000005, 6), 1.000001);
    EXPECT_TRUE(std::signbit(Round(-0.4, 0)));
}

TEST(CatRig, MeasuresAsPythonDoes) {
    std::vector<double> p = {1.5, 2.25}, q = {0.1, 7.7};
    EXPECT_EQ(Dist(p, q), 5.626944108483752);
    EXPECT_EQ(Hypot({1e-3, 3.3, 7.1}), 7.8294317162869485);
    EXPECT_EQ(FMod(-1.0, 3.0), 2.0);
    EXPECT_EQ(FMod(7.5, -2.0), -0.5);
    EXPECT_EQ(Mod(-1, 3), 2);
}

TEST(CatRig, RotationsRoundTrip) {
    M3 r = RotAbout({0.3, 1.0, -0.2}, 37.0);
    M3 back = QuatToMat(MatToQuat(r));
    for (std::size_t i = 0; i < 3; i++) {
        for (std::size_t j = 0; j < 3; j++) {
            EXPECT_NEAR(back[i][j], r[i][j], 1e-12);
        }
    }
    M4 m = MatFromRT(r, {4.0, -2.0, 9.0});
    M4 identity = MatMulExact(m, RigidInv(m));
    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            EXPECT_NEAR(identity[i][j], i == j ? 1.0 : 0.0, 1e-12);
        }
    }
    V3 p = XformPt(m, {1.0, 2.0, 3.0});
    V3 local = XformPt(RigidInv(m), p);
    EXPECT_NEAR(local[0], 1.0, 1e-12);
    EXPECT_NEAR(local[2], 3.0, 1e-12);
}

TEST(CatRig, FitsATwoBoneVertexToTwoPoses) {
    ColumnPose a0 = Col(MatFromRT(RotAbout({0, 1, 0}, 0), {0, 0, 0})), a1 = Col(MatFromRT(RotAbout({0, 1, 0}, 30), {1, 0, 0}));
    ColumnPose b0 = Col(MatFromRT(RotAbout({1, 0, 0}, 20), {0, 2, 0})), b1 = Col(MatFromRT(RotAbout({0, 0, 1}, -40), {0, 0, 3}));
    V3         p0 = {0.5, 1.0, -0.25}, p1 = {-0.75, 0.2, 0.6};
    double     w = 0.7;
    V3         ta = Blend(w, a0, a1, p0, p1), tb = Blend(w, b0, b1, p0, p1);
    auto       fit = TwoPoseFit(w, {a0, a1}, {b0, b1}, ta, tb);
    ASSERT_TRUE(fit.has_value());
    V3 fa = Blend(w, a0, a1, fit->first, fit->second), fb = Blend(w, b0, b1, fit->first, fit->second);
    for (std::size_t i = 0; i < 3; i++) {
        EXPECT_NEAR(fa[i], ta[i], 1e-6);
        EXPECT_NEAR(fb[i], tb[i], 1e-6);
    }
}

TEST(CatRig, FindsAPlanesNormal) {
    std::vector<V3> points = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 0, 1},
        {1, 0, 1},
        {2, 0, 3}
    };
    V3 n = PcaNormal(points, {0.8, 0.0, 1.0});
    EXPECT_NEAR(std::fabs(n[1]), 1.0, 1e-9);
}

TEST(CatRig, TriangulatesAndTestsPolygons) {
    std::vector<V2> square = {
        {0, 0},
        {1, 0},
        {1, 1},
        {0, 1}
    };
    std::vector<Tri> tris = Delaunay(square);
    ASSERT_EQ(tris.size(), 2u);
    for (const Tri &t : tris) {
        const V2 &a = square[std::size_t(t[0])], &b = square[std::size_t(t[1])], &c = square[std::size_t(t[2])];
        EXPECT_GT((b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1]), 0.0) << "counter-clockwise";
    }
    EXPECT_TRUE(PtInRing({0.5, 0.5}, square));
    EXPECT_FALSE(PtInRing({1.5, 0.5}, square));
}

TEST(SpiritCatModel, SkipsDataWithoutTheCats) {
    expanded::test::ScratchDir dir("spirit_cat");
    EXPECT_EQ(RunDataBuilds(dir.path(), "spirit cat", SpiritCatModelBuilds(), nullptr), 0);
    EXPECT_FALSE(fs::exists(dir.path() / "expanded"));
}

// DC_TEST_DATA, an extracted data directory: the cat built from it has the mod's rig, wings, mask and cape. (Its bytes
// are checked whole with the other built files.)
TEST(SpiritCatModel, BuildsTheModsCatFromTheDisc) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    auto                     read = [&](const char *path) { return ReadDataFile(fs::path(data) / path); };
    ChrPack                  pack = ChrPack::Parse(BuildSpiritCatModel(read(kSpiritCatSource), read(kSpiritCatFloatSource), read(kSpiritCatWingSource),
                                                                       read("dun/mpd_pack/d06main_a.mpd")));
    std::vector<std::string> records;
    for (const ChrRecord &record : pack.records) {
        records.push_back(record.name);
    }
    EXPECT_EQ(records, (std::vector<std::string>{"info.cfg", "dbcat.mds", "dbcat.img", "dbcat.mot", "dbcat.bbp", "dbcat.wgt", "catcape.clo"}));
    std::vector<std::string> frames = ReadMdsFrames(pack.Require("dbcat.mds").Payload());
    ASSERT_EQ(frames.size(), 49u);
    EXPECT_EQ(frames[0], "catroot");
    EXPECT_EQ(std::vector<std::string>(frames.end() - 12, frames.end()),
              (std::vector<std::string>{"cat_rwing1", "cat_rwing2", "cat_rwing3", "cat_rwing4", "cat_lwing1", "cat_lwing2", "cat_lwing3", "cat_lwing4",
                                        "cat_rwingm", "cat_lwingm", "cat_mask", "cat_cape"}));
    Bytes       payload = pack.Require(kSpiritCatCfg).Payload();
    std::string cfg(payload.begin(), payload.end());
    EXPECT_NE(cfg.find("CLOTH \"catcape.clo\""), std::string::npos);
    EXPECT_NE(cfg.find("\tKEY\t285,\t294,\t0.75,\t\t//71 cat float-up"), std::string::npos);
}
