#include <gtest/gtest.h>

#include <vector>

#include "clsmes.hpp"
#include "expanded/enemies/steve.hpp"

namespace {

// A .mes bank: the count, the byte offset past the text, (id, offset) pairs and the text.
std::vector<short> Bank() {
    // Two messages: 4010 ("A") and an empty placeholder 4020 at the end of the text. Offsets count from the
    // count'th word after the first.
    return {2, 16, 4010, 3, 4020, 5, 'A', MES_CODE_END, 0};
}

} // namespace

TEST(SteveLines, FilesByDungeonAndDemonShaftBand) {
    EXPECT_EQ(expanded::SteveFile(100, 1), "dun/message/ww_mes/steve01_1.mes");
    EXPECT_EQ(expanded::SteveFile(105, 2), "dun/message/ww_mes/steve06_2.mes");
    EXPECT_EQ(expanded::SteveFile(106, 1), "dun/message/ww_mes/steve07_1_1.mes");
    EXPECT_EQ(expanded::SteveFile(110, 3), "dun/message/ww_mes/steve07_5_3.mes");
}

TEST(SteveLines, AnEmptyPlaceholderIsNoLine) {
    std::vector<short> bank = Bank();
    const short       *text = expanded::SteveMessage(bank.data(), 4010);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text[0], 'A');
    EXPECT_EQ(expanded::SteveMessage(bank.data(), 4020), nullptr);
    EXPECT_EQ(expanded::SteveMessage(bank.data(), 4030), nullptr);
    EXPECT_EQ(expanded::SteveMessage(nullptr, 4010), nullptr);
}
