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
    {"chara/spirit_cat.chr",      437152,  0x6d1be3bc3aff86c5ULL},
    {"effect/atla_s.chr",         228304,  0xf08713b17be8b175ULL},
    {"effect/bomb_throw.chr",     46544,   0x141e8686c98c4655ULL},
    {"enemies/script_damage.txt", 2475,    0x95ea0a4a8b6351fdULL},
    {"monstor/c16a.chr",          1137920, 0x30b9ff47a953e4bcULL},
    {"monstor/c16a.stb",          31684,   0x806ea29199a14993ULL},
    {"monstor/c21a.chr",          1697792, 0xd4e57e4710287ae2ULL},
    {"monstor/e109a.stb",         23564,   0xac42985cc3c4de08ULL},
    {"monstor/e110a.stb",         19100,   0x7210fc495ea3cb3fULL},
    {"monstor/e167a.chr",         387072,  0xad2e6550523247a7ULL},
    {"monstor/e168a.chr",         784256,  0xeff7da0918a3607bULL},
    {"monstor/e168a.stb",         31744,   0x7d87d8c6235412abULL},
    {"monstor/e169a.chr",         468320,  0xc756bcac04dca439ULL},
    {"monstor/e169a.stb",         32176,   0xdd5cf026fee70375ULL},
    {"monstor/e34a.stb",          19580,   0xc2127fa8e11f9e44ULL},
    {"monstor/e35a.stb",          24204,   0x187f0fc8d84f0ba7ULL},
    {"monstor/e36a.stb",          19644,   0x85160f3033dbfed5ULL},
    {"monstor/e37a.stb",          23564,   0x4c60708f5d7ccefcULL},
    {"monstor/e38a.stb",          19788,   0x601fee9d3b117491ULL},
    {"monstor/e39a.stb",          23740,   0x92c96f3d2480031cULL},
    {"monstor/e65a.stb",          15580,   0x61a3ddf26d83073bULL},
    {"monstor/e69a.stb",          23280,   0x052212afe5fc7237ULL},
    {"monstor/e78a.stb",          17164,   0x5e66e84e71c27256ULL},
    {"monstor/e79a.stb",          24028,   0x90ee5c8637131c7dULL},
    {"monstor/e80a.stb",          19196,   0x32a41ed5aa003fa7ULL},
    {"monstor/e81a.stb",          23740,   0x348ca2492c6c2565ULL},
    {"monstor/e82a.stb",          19836,   0x5d86a4c08d9f716bULL},
    {"monstor/e83a.stb",          24364,   0x5fa3f5a7343b9b14ULL},
    {"monstor/e86a.stb",          23936,   0x479b69e5dfaa9912ULL},
    {"town/goro.chr",             749872,  0x5becc581c0882fbfULL},
    {"town/osmond.chr",           802656,  0xbe9fdd77fd5b663dULL},
    {"town/ruby.chr",             783776,  0x5ec46375b0d3e4ecULL},
    {"town/ungaga.chr",           796160,  0xdc82671c2936b419ULL},
    {"town/xiao.chr",             710128,  0xd1f9fe06cb7888a2ULL},
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
        if (entry.is_regular_file()) {
            EXPECT_TRUE(listed.contains(fs::relative(entry.path(), scratch / "expanded").generic_string()))
                << entry.path() << " is built but not checked";
        }
    }
}
