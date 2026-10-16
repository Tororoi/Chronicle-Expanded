#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/normalize.hpp"
#include "expanded/enemies/stronger.hpp"
#include "monstorunit.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"
#include "userstatus.hpp"

namespace {

using expanded::EnemyStat;
using expanded::Monster;
using expanded::No;
using expanded::NormalizedStat;

} // namespace

TEST(StrongerEnemies, IsAnExpandedToggleOffByDefault) {
    EXPECT_FALSE(expanded::StrongerEnemies());
    bool listed = false;
    for (const expanded::Toggle &toggle : expanded::Toggles()) {
        listed = listed || std::string(toggle.key) == expanded::kStrongerEnemies;
    }
    EXPECT_TRUE(listed);
}

TEST(StrongerEnemies, ARegionsOwnMonstersTakeTheNextRegionsStrengthButKeepTheirAbs) {
    expanded::EnemyStats werewolf = expanded::NormalizedStats(Monster::WEREWOLF, 1, true);
    EXPECT_EQ(werewolf.hp, 493);
    EXPECT_EQ(werewolf.abs, 12);
    // Past Demon Shaft's last band the trend carries on.
    EXPECT_EQ(NormalizedStat(Monster::WHITE_FANG_ENHANCED, EnemyStat::Hp, 10, true), 17313);
}

TEST(StrongerEnemies, AMimicIsAtLeastTheNextOne) {
    EXPECT_EQ(NormalizedStat(Monster::MIMIC_DBC, EnemyStat::Hp, 0, true), 150);
    EXPECT_EQ(NormalizedStat(Monster::MIMIC_DBC, EnemyStat::Defense, 0, true), 2);
    EXPECT_EQ(NormalizedStat(Monster::MIMIC_DS_ENHANCED_THRICE, EnemyStat::Hp, 10, true), 12000);
    EXPECT_EQ(NormalizedStat(Monster::KING_MIMIC_DS_ENHANCED_THRICE, EnemyStat::Hardness, 10, true), 60);
}

TEST(StrongerEnemies, TheToggleScalesTheFloorsOwnMonsters) {
    std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("dc_stronger_enemies_" + std::to_string(dc::test::ProcessId()));
    std::filesystem::create_directories(root);
    PathsSetSaveRoot(root);
    Config config = ConfigGet();
    ASSERT_TRUE(expanded::SetOption(config.expanded_options, expanded::kStrongerEnemies, true));
    ASSERT_TRUE(ConfigChange(config));
    ASSERT_TRUE(expanded::StrongerEnemies());

    auto         unit = std::make_unique<CMonstorUnit>();
    auto         status = std::make_unique<CUserStatus>();
    CUserStatus *saved_status = UserStatus;
    UserStatus = status.get();
    UserStatus->cur_floor = 4;
    selectMapNo = 1;
    unit->model[0] = MonstorTable[No(Monster::WEREWOLF)];
    expanded::EnemyModelSetUp(*unit, 0, No(Monster::WEREWOLF));
    EXPECT_EQ(expanded::SpeciesStats(0).hp, 493);
    EXPECT_EQ(expanded::SpeciesStats(0).abs, 12);

    UserStatus = saved_status;
    std::filesystem::remove_all(root);
}
