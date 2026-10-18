#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "expanded/core/options.hpp"
#include "options/rows.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"

namespace {

const options::Page *ExpandedPage() {
    for (const options::Page &page : options::Pages()) {
        if (std::string_view(page.name) == "Expanded") {
            return &page;
        }
    }
    return nullptr;
}

} // namespace

TEST(ExpandedOptions, AnUnknownToggleIsOffAndNotKept) {
    EXPECT_FALSE(expanded::OptionOn("test_missing"));
    EXPECT_EQ(ConfigParse(R"({"expanded": {"test_missing": true}})"), Config{});
}

TEST(ExpandedOptions, ToggleRoundTripsThroughConfig) {
    ASSERT_TRUE(expanded::AddToggle({.key = "test_toggle", .label = "Test Toggle", .help = "\"Test Toggle\"\nA test."}));
    ASSERT_TRUE(expanded::AddToggle({.key = "test_default_on", .label = "Test On", .on = true}));
    EXPECT_FALSE(expanded::AddToggle({.key = "test_toggle", .label = "Again"}));

    // The defaults are written, and a default value keeps the settings equal to Config{}.
    Config config = ConfigParse("");
    EXPECT_EQ(config, Config{});
    EXPECT_FALSE(expanded::OptionOn(config.expanded_options, "test_toggle"));
    EXPECT_TRUE(expanded::OptionOn(config.expanded_options, "test_default_on"));
    EXPECT_NE(ConfigSerialize(config).find(R"("expanded": {)"), std::string::npos);
    EXPECT_EQ(ConfigParse(ConfigSerialize(config)), Config{});

    ASSERT_TRUE(expanded::SetOption(config.expanded_options, "test_toggle", true));
    ASSERT_TRUE(expanded::SetOption(config.expanded_options, "test_default_on", false));
    EXPECT_FALSE(expanded::SetOption(config.expanded_options, "missing", true));
    Config restored = ConfigParse(ConfigSerialize(config));
    EXPECT_EQ(restored, config);
    EXPECT_TRUE(expanded::OptionOn(restored.expanded_options, "test_toggle"));
    EXPECT_FALSE(expanded::OptionOn(restored.expanded_options, "test_default_on"));

    // Bad values and unknown keys keep the defaults.
    Config bad = ConfigParse(R"({"expanded": {"test_toggle": "yes", "missing": true}})");
    EXPECT_EQ(bad, Config{});

    // Through config.json and back.
    std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("dc_expanded_options_" + std::to_string(dc::test::ProcessId()));
    std::filesystem::create_directories(root);
    PathsSetSaveRoot(root);
    ASSERT_TRUE(ConfigChange(config));
    std::ostringstream text;
    text << std::ifstream(root / "config.json").rdbuf();
    EXPECT_NE(text.str().find(R"("test_toggle": true)"), std::string::npos);
    ASSERT_TRUE(ConfigLoad());
    EXPECT_TRUE(expanded::OptionOn("test_toggle"));
    EXPECT_FALSE(expanded::OptionOn("test_default_on"));
    std::filesystem::remove_all(root);
}

TEST(ExpandedOptions, PageShowsAToggleAsARow) {
    ASSERT_TRUE(expanded::AddToggle({.key = "test_toggle", .label = "Test Toggle", .help = "\"Test Toggle\"\nA test."}));
    const options::Page *page = ExpandedPage();
    ASSERT_NE(page, nullptr);
    EXPECT_EQ(&options::Pages().back(), page);
    ASSERT_EQ(page->rows.size(), expanded::Toggles().size());
    const options::Row &row = page->rows.back();
    EXPECT_STREQ(row.key, "expanded.test_toggle");

    Config config;
    EXPECT_EQ(options::RowValue(row, config), "Off");
    EXPECT_TRUE(options::StepRow(row, config, -1, false));
    EXPECT_TRUE(expanded::OptionOn(config.expanded_options, "test_toggle"));
    EXPECT_EQ(options::RowValue(row, config), "On");
    options::ResetPage(*page, config);
    EXPECT_EQ(config, Config{});
}

TEST(ExpandedOptions, AChoiceRoundTripsThroughConfigByName) {
    ASSERT_TRUE(expanded::AddToggle({.key = "test_choice", .label = "Test Choice", .choices = "Off|Some|All"}));

    Config config = ConfigParse("");
    EXPECT_EQ(expanded::OptionChoice(config.expanded_options, "test_choice"), 0);
    EXPECT_FALSE(expanded::OptionOn(config.expanded_options, "test_choice"));
    EXPECT_FALSE(expanded::SetOption(config.expanded_options, "test_choice", true));
    EXPECT_FALSE(expanded::SetOptionChoice(config.expanded_options, "test_choice", 3));
    EXPECT_FALSE(expanded::SetOptionChoice(config.expanded_options, "test_choice", "most"));

    // By name in any case, written as the row names it.
    ASSERT_TRUE(expanded::SetOptionChoice(config.expanded_options, "test_choice", "some"));
    EXPECT_EQ(expanded::OptionChoice(config.expanded_options, "test_choice"), 1);
    EXPECT_TRUE(expanded::OptionOn(config.expanded_options, "test_choice"));
    EXPECT_NE(ConfigSerialize(config).find(R"("test_choice": "Some")"), std::string::npos);
    EXPECT_EQ(ConfigParse(ConfigSerialize(config)), config);

    // Back to the default is no change; a boolean is no choice.
    ASSERT_TRUE(expanded::SetOptionChoice(config.expanded_options, "test_choice", 0));
    EXPECT_EQ(config, Config{});
    EXPECT_EQ(ConfigParse(R"({"expanded": {"test_choice": true}})"), Config{});
}

TEST(ExpandedOptions, PageShowsAChoiceAsARowOfItsNames) {
    ASSERT_TRUE(expanded::AddToggle({.key = "test_choice", .label = "Test Choice", .choices = "Off|Some|All"}));
    const options::Page *page = ExpandedPage();
    ASSERT_NE(page, nullptr);
    const options::Row &row = page->rows.back();
    EXPECT_STREQ(row.key, "expanded.test_choice");

    Config config;
    EXPECT_EQ(options::RowValue(row, config), "Off");
    EXPECT_TRUE(options::StepRow(row, config, 1, false));
    EXPECT_EQ(options::RowValue(row, config), "Some");
    EXPECT_TRUE(options::StepRow(row, config, 1, false));
    EXPECT_EQ(expanded::OptionChoice(config.expanded_options, "test_choice"), 2);
    EXPECT_EQ(options::RowValue(row, config), "All");
}
