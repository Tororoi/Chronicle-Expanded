#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "expanded/data/data.hpp"
#include "test_support.hpp"

namespace fs = std::filesystem;

namespace {

void Write(const fs::path &file, const std::string &text) {
    fs::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << text;
}

std::string Read(const fs::path &file) {
    std::ifstream stream(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

// A converter that bakes one document a game file, at the file's path, holding the file's bytes; the files it was given.
struct FakeBaker {
    std::vector<std::string> *seen;

    void operator()(const fs::path &raw, const fs::path &out) const {
        for (const fs::directory_entry &entry : fs::recursive_directory_iterator(raw)) {
            if (entry.is_regular_file()) {
                std::string path = fs::relative(entry.path(), raw).generic_string();
                seen->push_back(path);
                Write(out / "data" / "textures" / (path + ".json"), Read(entry.path()));
            }
        }
    }
};

} // namespace

TEST(ExpandedTextures, WhatIsBuiltIsBakedAgainAndTheRestIsLeft) {
    expanded::test::ScratchDir folder("expanded_textures");
    const fs::path            &data = folder.path();
    Write(data / "expanded/disc/chara/c01d_turi.chr", "rebuilt");
    Write(data / "expanded/town/xiao.chr", "xiao");
    Write(data / "chara/c01d.chr", "disc");
    Write(data / "textures/chara/c01d.chr.json", "disc document");
    Write(data / "textures/expanded/town/xiao.chr.json", "stale");
    Write(data / "textures/expanded/town/gone.chr.json", "no longer built");

    std::vector<std::string> seen;
    expanded::BakeExpandedTextures(data, FakeBaker{&seen}, nullptr);

    // Only what is built is converted, and its documents replace the ones baked before.
    EXPECT_EQ(seen.size(), 2u);
    for (const std::string &path : seen) {
        EXPECT_TRUE(path.starts_with("expanded/")) << path;
    }
    EXPECT_EQ(Read(data / "textures/expanded/disc/chara/c01d_turi.chr.json"), "rebuilt");
    EXPECT_EQ(Read(data / "textures/expanded/town/xiao.chr.json"), "xiao");
    EXPECT_FALSE(fs::exists(data / "textures/expanded/town/gone.chr.json"));
    EXPECT_EQ(Read(data / "textures/chara/c01d.chr.json"), "disc document");
}

TEST(ExpandedTextures, DataWithoutBakedTexturesIsLeftAsItIs) {
    expanded::test::ScratchDir folder("expanded_textures_raw");
    const fs::path            &data = folder.path();
    Write(data / "expanded/town/xiao.chr", "xiao");

    std::vector<std::string> seen;
    expanded::BakeExpandedTextures(data, FakeBaker{&seen}, nullptr);
    EXPECT_TRUE(seen.empty());
    EXPECT_FALSE(fs::exists(data / "textures"));
}
