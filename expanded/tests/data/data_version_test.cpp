#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "expanded/data/data.hpp"
#include "expanded/data/data_version.hpp"
#include "test_support.hpp"

namespace expanded::test {

namespace {

namespace fs = std::filesystem;

void WriteStamp(const fs::path &data, const std::string &text) {
    std::ofstream(data / kExpandedDataStamp, std::ios::binary) << text;
}

TEST(ExpandedDataVersion, OnlyThisBuildsStampIsCurrent) {
    ScratchDir folder("expanded_version");
    EXPECT_FALSE(ExpandedDataCurrent(folder.path())); // never built
    WriteStamp(folder.path(), "0123456789abcdef");
    EXPECT_FALSE(ExpandedDataCurrent(folder.path())); // built by other builders
    WriteStamp(folder.path(), std::string(kExpandedDataVersion));
    EXPECT_TRUE(ExpandedDataCurrent(folder.path()));
}

TEST(ExpandedDataVersion, ABuildStampsTheData) {
    // With none of the disc's files every build is skipped, which is no failure.
    ScratchDir folder("expanded_version_build");
    WriteStamp(folder.path(), "0123456789abcdef");
    EXPECT_EQ(BuildExpandedData(folder.path(), nullptr), 0);
    EXPECT_TRUE(ExpandedDataCurrent(folder.path()));
}

} // namespace

} // namespace expanded::test
