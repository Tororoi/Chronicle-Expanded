#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <system_error>

#include "expanded/data/data.hpp"
#include "test_support.hpp"

namespace fs = std::filesystem;

namespace {

struct Built {
    const char   *path; // under expanded/
    std::size_t   size;
    std::uint64_t hash; // FNV-1a
};

// What `dcdata expanded` builds, as the PS2 mod's tools build it, from PAL and NTSC data alike.
constexpr Built kBuilt[] = {
    {"chara/queens_trees.chr",       83520,   0x8e1dda13dad52c72ULL},
    {"chara/shadow_disc.chr",        5584,    0xd148f318c2a885c2ULL},
    {"chara/spirit_cat.chr",         437152,  0x6d1be3bc3aff86c5ULL},
    {"disc/gedit/e03/e03_1.mes",     5598,    0x7f7a938dd5735e38ULL},
    {"disc/gedit/e03/e03talk_1.mes", 47122,   0xfb8f96324c158b40ULL},
    {"disc/gedit/e03/img.pak",       3111888, 0x665f33dfe67d12b8ULL},
    {"disc/gedit/e03/mapinfo.cfg",   15427,   0x09669cfc29f0372bULL},
    {"disc/gedit/e03/scene.scn",     3282400, 0x9473617e19b976dcULL},
    {"disc/gedit/s04/mapinfo.cfg",   7653,    0x2f1185371e128bcfULL},
    {"disc/gedit/s04/s04_1.mes",     1260,    0xa48ffd2028d644a4ULL},
    {"disc/gedit/s04/s04talk_1.mes", 11874,   0x827f90eaeb74b82aULL},
    {"disc/gedit/s04/scene.scn",     1193296, 0x231f034901f34c3fULL},
    {"disc/gedit/s13/mapinfo.cfg",   12176,   0x6a1338770386d08cULL},
    {"disc/gedit/s13/s13_1.mes",     3812,    0x52b0942755be4eabULL},
    {"disc/gedit/s13/s13talk_1.mes", 9190,    0x60f27e4734bcb712ULL},
    {"disc/gedit/s13/scene.scn",     1092800, 0xdcc553521ecc789fULL},
    {"effect/atla_s.chr",            228304,  0xf08713b17be8b175ULL},
    {"effect/babel_beam.chr",        59744,   0xc8e2a64f097d90a3ULL},
    {"effect/bomb_throw.chr",        46544,   0x141e8686c98c4655ULL},
    {"enemies/script_damage.txt",    2475,    0x95ea0a4a8b6351fdULL},
    {"fishing/e03.col",              25684,   0x88799f90c4c174bdULL},
    {"fishing/s04.col",              7404,    0xcb314e0a7bd3084eULL},
    {"fishing/s13.col",              2392,    0xe85b3ce26bce2af9ULL},
    {"fishing/sign.img",             84208,   0xdc52ecd257cf0008ULL},
    {"monstor/c16a.chr",             1137920, 0x30b9ff47a953e4bcULL},
    {"monstor/c16a.stb",             31684,   0x806ea29199a14993ULL},
    {"monstor/c21a.chr",             1697792, 0xd4e57e4710287ae2ULL},
    {"monstor/e109a.stb",            23564,   0xac42985cc3c4de08ULL},
    {"monstor/e110a.stb",            19100,   0x7210fc495ea3cb3fULL},
    {"monstor/e167a.chr",            387072,  0xad2e6550523247a7ULL},
    {"monstor/e168a.chr",            784256,  0xeff7da0918a3607bULL},
    {"monstor/e168a.stb",            31744,   0x7d87d8c6235412abULL},
    {"monstor/e169a.chr",            468320,  0xc756bcac04dca439ULL},
    {"monstor/e169a.stb",            32176,   0xdd5cf026fee70375ULL},
    {"monstor/e34a.stb",             19580,   0xc2127fa8e11f9e44ULL},
    {"monstor/e35a.stb",             24204,   0x187f0fc8d84f0ba7ULL},
    {"monstor/e36a.stb",             19644,   0x85160f3033dbfed5ULL},
    {"monstor/e37a.stb",             23564,   0x4c60708f5d7ccefcULL},
    {"monstor/e38a.stb",             19788,   0x601fee9d3b117491ULL},
    {"monstor/e39a.stb",             23740,   0x92c96f3d2480031cULL},
    {"monstor/e65a.stb",             15580,   0x61a3ddf26d83073bULL},
    {"monstor/e69a.stb",             23280,   0x052212afe5fc7237ULL},
    {"monstor/e78a.stb",             17164,   0x5e66e84e71c27256ULL},
    {"monstor/e79a.stb",             24028,   0x90ee5c8637131c7dULL},
    {"monstor/e80a.stb",             19196,   0x32a41ed5aa003fa7ULL},
    {"monstor/e81a.stb",             23740,   0x348ca2492c6c2565ULL},
    {"monstor/e82a.stb",             19836,   0x5d86a4c08d9f716bULL},
    {"monstor/e83a.stb",             24364,   0x5fa3f5a7343b9b14ULL},
    {"monstor/e86a.stb",             23936,   0x479b69e5dfaa9912ULL},
    {"town/goro.chr",                749872,  0x5becc581c0882fbfULL},
    {"town/osmond.chr",              802656,  0xbe9fdd77fd5b663dULL},
    {"town/ruby.chr",                783776,  0x5ec46375b0d3e4ecULL},
    {"town/ungaga.chr",              796160,  0xdc82671c2936b419ULL},
    {"town/xiao.chr",                710128,  0xd1f9fe06cb7888a2ULL},
};

// Built from a disc's own scripts and text, which differ from PAL to NTSC: each disc's version, PAL's first.
constexpr Built kDiscVersions[] = {
    {"disc/gedit/e03/e03_2.mes",     5512,   0x5b6b0bf6f492fba9ULL},
    {"disc/gedit/e03/e03_3.mes",     6260,   0xb9d87f896511f40aULL},
    {"disc/gedit/e03/e03_3.mes",     6348,   0x4ecc072932abab41ULL},
    {"disc/gedit/e03/e03_4.mes",     6630,   0x94583662af3175e7ULL},
    {"disc/gedit/e03/e03_4.mes",     6630,   0x96a439f13844ba4dULL},
    {"disc/gedit/e03/e03_5.mes",     6168,   0xa8a4a807a2941574ULL},
    {"disc/gedit/e03/e03_5.mes",     6752,   0xcfde690c2332033bULL},
    {"disc/gedit/e03/e03_6.mes",     5388,   0xbb1f42dbaabedb93ULL},
    {"disc/gedit/e03/e03_6.mes",     5406,   0x75996f760ac29e85ULL},
    {"disc/gedit/e03/e03talk_2.mes", 47004,  0xf60d6c2fe194463cULL},
    {"disc/gedit/e03/e03talk_3.mes", 49178,  0x7de03cc5990d7b7eULL},
    {"disc/gedit/e03/e03talk_3.mes", 49052,  0x76ad899836042dedULL},
    {"disc/gedit/e03/e03talk_4.mes", 49350,  0x5946ab35473e8771ULL},
    {"disc/gedit/e03/e03talk_4.mes", 49162,  0x4bac0abf656e5f6bULL},
    {"disc/gedit/e03/e03talk_5.mes", 46266,  0x8fa72b2ab926d678ULL},
    {"disc/gedit/e03/e03talk_5.mes", 45994,  0x179a5c9f7b554087ULL},
    {"disc/gedit/e03/e03talk_6.mes", 42880,  0x8d743ce36c84fa49ULL},
    {"disc/gedit/e03/e03talk_6.mes", 42802,  0x4e36b32ba1631828ULL},
    {"disc/gedit/e03/event.stb",     167520, 0x787181f53bd8b1b7ULL},
    {"disc/gedit/e03/event.stb",     161728, 0x690062298cc69095ULL},
    {"disc/gedit/s04/s04_2.mes",     1190,   0xce8196fbc7aaab62ULL},
    {"disc/gedit/s04/s04_3.mes",     1232,   0xddbdf6bc464385bfULL},
    {"disc/gedit/s04/s04_3.mes",     1328,   0x1d31e184de6af4faULL},
    {"disc/gedit/s04/s04_4.mes",     1312,   0xa8b856d8cda7d18bULL},
    {"disc/gedit/s04/s04_4.mes",     1326,   0xc45714cad46405a8ULL},
    {"disc/gedit/s04/s04_5.mes",     1296,   0x41178ccdcd376ed6ULL},
    {"disc/gedit/s04/s04_5.mes",     1300,   0xf9059993c9ad5732ULL},
    {"disc/gedit/s04/s04_6.mes",     1270,   0xef912cc920013785ULL},
    {"disc/gedit/s04/s04_6.mes",     1302,   0xf4f2da232925de9dULL},
    {"disc/gedit/s04/s04talk_2.mes", 11802,  0x9569460e01d910e0ULL},
    {"disc/gedit/s04/s04talk_3.mes", 13010,  0x2d678f521cb11247ULL},
    {"disc/gedit/s04/s04talk_3.mes", 13088,  0x7e88a85081f6b897ULL},
    {"disc/gedit/s04/s04talk_4.mes", 13264,  0xc6160d6e4d2721b7ULL},
    {"disc/gedit/s04/s04talk_4.mes", 13326,  0x38dc0f22f4d87fa6ULL},
    {"disc/gedit/s04/s04talk_5.mes", 13752,  0x6ec7a93896c5f3d5ULL},
    {"disc/gedit/s04/s04talk_5.mes", 13936,  0xb4e5fc7bbd4e4e61ULL},
    {"disc/gedit/s04/s04talk_6.mes", 11880,  0x87594eb0d7ff9da3ULL},
    {"disc/gedit/s04/s04talk_6.mes", 11922,  0xb17bc7b41ed9c744ULL},
    {"disc/gedit/s04/event.stb",     151096, 0x6effac3174cf6fddULL},
    {"disc/gedit/s04/event.stb",     135960, 0x53dd364b8c5e01c2ULL},
    {"disc/gedit/s09/event.stb",     125992, 0x48a770c8ad3a10cdULL},
    {"disc/gedit/s09/event.stb",     125752, 0xab996baa7b102929ULL},
    {"disc/gedit/s13/s13_2.mes",     3734,   0x0755d3655ebc5057ULL},
    {"disc/gedit/s13/s13_3.mes",     3836,   0xfcb0e8cb0c4ae1ceULL},
    {"disc/gedit/s13/s13_3.mes",     3936,   0xd7dbeabd5c236f8fULL},
    {"disc/gedit/s13/s13_4.mes",     3902,   0x1a2d35ad24e911bdULL},
    {"disc/gedit/s13/s13_4.mes",     3910,   0x4a8f0ceb59143575ULL},
    {"disc/gedit/s13/s13_5.mes",     3810,   0xadcd0d9cf52f3ac7ULL},
    {"disc/gedit/s13/s13_5.mes",     3776,   0x021e26a9acabcfcaULL},
    {"disc/gedit/s13/s13_6.mes",     3608,   0x081dd6f589c1f5e1ULL},
    {"disc/gedit/s13/s13_6.mes",     3622,   0x90cb482dde9e765dULL},
    {"disc/gedit/s13/s13talk_2.mes", 9118,   0x62e089a8326b0080ULL},
    {"disc/gedit/s13/s13talk_3.mes", 9646,   0xeef624bac6e5ef30ULL},
    {"disc/gedit/s13/s13talk_3.mes", 9726,   0x167f895b71aec431ULL},
    {"disc/gedit/s13/s13talk_4.mes", 9914,   0xff68da1fefb19f94ULL},
    {"disc/gedit/s13/s13talk_4.mes", 9812,   0x26ad8dc6e7a90cc7ULL},
    {"disc/gedit/s13/s13talk_5.mes", 9738,   0x6a90490176e5cfa5ULL},
    {"disc/gedit/s13/s13talk_5.mes", 9892,   0x4495b09273e03e74ULL},
    {"disc/gedit/s13/s13talk_6.mes", 8268,   0x31f3c674c417c21fULL},
    {"disc/gedit/s13/s13talk_6.mes", 8290,   0x5ea5eb448b68223aULL},
    {"disc/gedit/s13/event.stb",     133072, 0xa1d9f2aba0a089f8ULL},
    {"disc/gedit/s13/event.stb",     132560, 0xcec084b81d94458fULL},
};

std::uint64_t Fnv1a(const std::string &bytes) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (unsigned char byte : bytes) {
        hash = (hash ^ byte) * 0x100000001b3ULL;
    }
    return hash;
}

} // namespace

// DC_TEST_DATA, an extracted data directory: everything is built into a scratch directory that links to its disc
// folders, so the data itself is left as it is. A full build takes a few seconds; run it with a longer test timeout.
TEST(ExpandedData, BuildsWhatTheModsToolsBuild) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    expanded::test::ScratchDir folder("expanded_data");
    const fs::path            &scratch = folder.path();
    std::error_code            error;
    for (const fs::directory_entry &entry : fs::directory_iterator(data)) {
        if (entry.path().filename() != "expanded") {
            fs::create_symlink(fs::absolute(entry.path()), scratch / entry.path().filename(), error);
            if (error) {
                GTEST_SKIP() << "cannot link the data here: " << error.message();
            }
        }
    }

    EXPECT_EQ(expanded::BuildExpandedData(scratch, nullptr), 0);
    std::set<std::string> listed;
    for (const Built &built : kBuilt) {
        listed.insert(built.path);
        std::ifstream stream(scratch / "expanded" / built.path, std::ios::binary);
        ASSERT_TRUE(stream) << built.path << " was not built";
        std::string bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        EXPECT_EQ(bytes.size(), built.size) << built.path;
        EXPECT_EQ(Fnv1a(bytes), built.hash) << built.path;
    }
    for (const fs::directory_entry &entry : fs::recursive_directory_iterator(scratch / "expanded")) {
        std::string path = fs::relative(entry.path(), scratch / "expanded").generic_string();
        if (!entry.is_regular_file() || listed.contains(path)) {
            continue;
        }
        std::ifstream stream(entry.path(), std::ios::binary);
        std::string   bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        bool          known = false, version = false;
        for (const Built &built : kDiscVersions) {
            known = known || path == built.path;
            version = version || (path == built.path && bytes.size() == built.size && Fnv1a(bytes) == built.hash);
        }
        EXPECT_TRUE(known) << entry.path() << " is built but not checked";
        EXPECT_TRUE(!known || version) << entry.path() << " is no disc's version";
    }
}
