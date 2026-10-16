#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

#include "expanded/core/options.hpp"
#include "expanded/enemies/faster.hpp"
#include "monstorunit.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"

namespace {

// Turns Faster Enemies on or off in the current settings.
void SetFaster(bool on) {
    std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("dc_expanded_faster_" + std::to_string(dc::test::ProcessId()));
    std::filesystem::create_directories(root);
    PathsSetSaveRoot(root);
    Config config = ConfigGet();
    ASSERT_TRUE(expanded::SetOption(config.expanded_options, "faster_enemies", on));
    ASSERT_TRUE(ConfigChange(config));
}

} // namespace

TEST(FasterEnemies, OffByDefaultLeavesTheSpeed) {
    EXPECT_FALSE(expanded::FasterEnemiesOn());
    EXPECT_EQ(expanded::FasterMoveSpeed(1.5f), 1.5f);
}

TEST(FasterEnemies, OnScalesForwardMoves) {
    SetFaster(true);
    EXPECT_TRUE(expanded::FasterEnemiesOn());
    EXPECT_EQ(expanded::FasterMoveSpeed(1.5f), 1.5f * expanded::kFasterMoveScale);
    EXPECT_EQ(expanded::FasterMoveSpeed(0.0f), 0.0f);
    EXPECT_EQ(expanded::FasterMoveSpeed(-1.0f), -1.0f);
    SetFaster(false);
}

TEST(FasterEnemies, HitWindowTakesTheMargin) {
    auto                   unit = std::make_unique<CMonstorUnit>();
    MONSTOR_EFFECT_STATE2 &hits = unit->effect2[3];

    hits.active[2] = 1;
    hits.motion_start[2] = 130.0f;
    hits.motion_end[2] = 140.0f;
    EXPECT_TRUE(expanded::FasterInHitWindow(*unit, 3, 135.0f));
    EXPECT_TRUE(expanded::FasterInHitWindow(*unit, 3, 129.0f));
    EXPECT_TRUE(expanded::FasterInHitWindow(*unit, 3, 141.0f));
    EXPECT_FALSE(expanded::FasterInHitWindow(*unit, 3, 128.5f));
    EXPECT_FALSE(expanded::FasterInHitWindow(*unit, 3, 141.5f));
    EXPECT_FALSE(expanded::FasterInHitWindow(*unit, 2, 135.0f));

    hits.active[2] = 0;
    EXPECT_FALSE(expanded::FasterInHitWindow(*unit, 3, 135.0f));
}
