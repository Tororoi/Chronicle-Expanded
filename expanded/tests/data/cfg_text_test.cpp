#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "expanded/data/cfg_text.hpp"

namespace {

using expanded::Bytes;

Bytes Cfg(const std::string &keys, const std::string &events = "") {
    std::string text = "MODEL \"a.mds\"\r\n\r\nMOTION 0, \"a.mot\", \"a.bbp\", \"a.wgt\"\r\nKEY_START 0\r\n" + keys + "MOTION_END\r\n" + events;
    return Bytes(text.begin(), text.end());
}

std::string Str(const Bytes &bytes) {
    return std::string(bytes.begin(), bytes.end());
}

const std::string kKeys = "KEY\t1,\t2,\t0.1,\t//a\r\n\tKEY\t3,\t4,\t0.2\r\nKEY\t5,\t6,\t0.3,\t//c\r\n";

} // namespace

TEST(CfgText, AppendsKeysBeforeTheMotionEnd) {
    EXPECT_EQ(Str(expanded::AppendCfgKeys(Cfg(kKeys, "EVENT 1,1,1\r\n"), "KEY\t7,\t8,\t0.1,\t//d\r\n")),
              Str(Cfg(kKeys + "KEY\t7,\t8,\t0.1,\t//d\r\n", "EVENT 1,1,1\r\n")));
    std::string none = "MODEL \"a.mds\"\r\n";
    EXPECT_THROW(expanded::AppendCfgKeys(Bytes(none.begin(), none.end()), "KEY\r\n"), std::runtime_error);
}

TEST(CfgText, AppendsEventsAfterTheMotionEnd) {
    EXPECT_EQ(Str(expanded::AppendCfgEvents(Cfg(kKeys, "EVENT 1,1,1\r\n"), "EVENT 2,1,2\r\n")),
              Str(Cfg(kKeys, "EVENT 2,1,2\r\nEVENT 1,1,1\r\n")));
}

TEST(CfgText, ReplacesTheKeys) {
    EXPECT_EQ(Str(expanded::ReplaceCfgKeys(Cfg(kKeys, "EVENT 1,1,1\r\n"), "KEY\t9,\t9,\t0.0\r\n")),
              Str(Cfg("KEY\t9,\t9,\t0.0\r\n", "EVENT 1,1,1\r\n")));
}

TEST(CfgText, SetsAKeyKeepingItsComment) {
    EXPECT_EQ(Str(expanded::SetCfgKey(Cfg(kKeys), 0, "KEY\t10,\t20,\t0.35,\t")),
              Str(Cfg("KEY\t10,\t20,\t0.35,\t//a\r\n\tKEY\t3,\t4,\t0.2\r\nKEY\t5,\t6,\t0.3,\t//c\r\n")));
    // An indented key without a comment keeps its \r.
    EXPECT_EQ(Str(expanded::SetCfgKey(Cfg(kKeys), 1, "KEY\t0,\t0,\t0.0,\t")),
              Str(Cfg("KEY\t1,\t2,\t0.1,\t//a\r\nKEY\t0,\t0,\t0.0,\t\r\nKEY\t5,\t6,\t0.3,\t//c\r\n")));
    EXPECT_THROW(expanded::SetCfgKey(Cfg(kKeys), 3, "KEY"), std::runtime_error);
}
