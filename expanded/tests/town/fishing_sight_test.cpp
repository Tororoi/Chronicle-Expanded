#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include "expanded/town/fishing_sight.hpp"

namespace {

namespace fs = std::filesystem;

struct Span {
    float low = 1e9f, high = -1e9f;
};

// The extent of a part along one of its axes.
Span Along(const std::vector<expanded::SightPoint> &part, int axis) {
    Span span;
    for (const expanded::SightPoint &p : part) {
        span.low = std::min(span.low, p[static_cast<std::size_t>(axis)]);
        span.high = std::max(span.high, p[static_cast<std::size_t>(axis)]);
    }
    return span;
}

std::vector<unsigned char> Read(const fs::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}

// DC_TEST_DATA, an extracted data directory: the pointer is the Aquamarine's gem.
TEST(FishingSight, IsTheAquamarine) {
    const char *data = std::getenv("DC_TEST_DATA");
    if (data == nullptr) {
        GTEST_SKIP() << "DC_TEST_DATA is not set";
    }
    const std::vector<unsigned char> gem = Read(fs::path(data) / "dun/item/main_data/juelaqua.mds");
    ASSERT_FALSE(gem.empty());
    const std::vector<expanded::SightPoint> pointer = expanded::BuildSightPointer(gem);
    // The gem whole, from its round end to its point, round its middle.
    EXPECT_EQ(pointer.size(), 60u * 3u);
    const Span length = Along(pointer, 0), wide = Along(pointer, 2);
    EXPECT_FLOAT_EQ(length.low, 0.0f);
    EXPECT_NEAR(length.high, expanded::kSightPointerLength, 0.01f);
    EXPECT_NEAR(wide.high + wide.low, 0.0f, 0.01f);
}

// Without its mesh, no pointer.
TEST(FishingSight, NeedsTheMesh) {
    const std::vector<unsigned char> empty(0x10, 0);
    EXPECT_TRUE(expanded::BuildSightPointer(empty).empty());
}

} // namespace
