#include <gtest/gtest.h>

#include <memory>

#include "dungeonmap.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/drain_seeker.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// A real chest in slot 0, a mimic's in slot 1.
std::unique_ptr<CDungeonMap> Chests() {
    auto map = std::make_unique<CDungeonMap>();
    for (int box : {0, 1}) {
        map->boxes[box] = {.used = 1, .closed = 1, .kind = TREASURE_BOX_SMALL};
        map->events[box].kind = box == 0 ? DNG_EVENT_TREASURE_BOX : DNG_EVENT_MIMIC;
        map->events[box].index = box;
    }
    return map;
}

} // namespace

TEST(DrainSeeker, BloodScentMarksMimicsAsMonsters) {
    auto                     map = Chests();
    expanded::test::Wielding seeker(ITEM_WEAPON_DRAIN_SEEKER);
    EXPECT_TRUE(expanded::BloodScentActive());
    EXPECT_FALSE(expanded::BloodScentMimic(*map, 0));
    EXPECT_TRUE(expanded::BloodScentMimic(*map, 1));
}

TEST(DrainSeeker, BloodScentNeedsTheDrainSeeker) {
    auto                     map = Chests();
    expanded::test::Wielding dagger(ITEM_WEAPON_DAGGER);
    EXPECT_FALSE(expanded::BloodScentActive());
    EXPECT_FALSE(expanded::BloodScentMimic(*map, 1));
}

TEST(DrainSeeker, SuperSteveCarriesBloodScent) {
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DRAIN_SEEKER);
    EXPECT_TRUE(expanded::BloodScentActive());
}

TEST(DrainSeeker, OffWithTheWeaponAbilities) {
    auto                        map = Chests();
    expanded::test::Wielding    seeker(ITEM_WEAPON_DRAIN_SEEKER);
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_FALSE(expanded::BloodScentActive());
    EXPECT_FALSE(expanded::BloodScentMimic(*map, 1));
}
