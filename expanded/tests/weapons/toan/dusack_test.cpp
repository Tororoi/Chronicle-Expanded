#include <gtest/gtest.h>

#include <memory>

#include "expanded/data/monster_scripts.hpp"
#include "expanded/weapons/toan/dusack.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::test::Wielding;

namespace {

// A floor whose monster 0 is of kind, with guard windows 0 and 1 its wake (to end) and its own guard motion's.
struct Floor {
    std::unique_ptr<CMonstorUnit> unit = std::make_unique<CMonstorUnit>();

    explicit Floor(int kind, float end = expanded::kMimicWakeGuardEnd) {
        unit->model[0].kind = static_cast<s16>(kind);
        unit->monster[0].base_model = 0;
        MONSTOR_GUARD_WINDOW &guard = unit->guard[0];
        guard.active[0] = guard.active[1] = 1;
        guard.motion_start[0] = expanded::kMimicWakeGuardStart;
        guard.motion_end[0] = end;
        guard.motion_start[1] = 173.0f;
        guard.motion_end[1] = 186.0f;
    }

    bool Opens(int window) const { return expanded::NoFoolsGoldOpens(*unit, 0, window); }
};

} // namespace

TEST(Dusack, MimicsWakeGuardLetsHitsThrough) {
    for (int weapon : {ITEM_WEAPON_DUSACK, ITEM_WEAPON_BRAVE_ARK}) {
        Wielding wielding(weapon);
        Floor    mimic(MONSTER_KIND_MIMIC_SMALL);
        EXPECT_TRUE(mimic.Opens(0));
        EXPECT_FALSE(mimic.Opens(1)); // its own guard still blocks
        Floor king(MONSTER_KIND_MIMIC_LARGE, expanded::kKingMimicWakeGuardEnd);
        EXPECT_TRUE(king.Opens(0));
        EXPECT_FALSE(king.Opens(1));
    }
}

TEST(Dusack, OthersKeepTheirGuard) {
    {
        Wielding wielding(ITEM_WEAPON_DAGGER);
        Floor    mimic(MONSTER_KIND_MIMIC_SMALL);
        EXPECT_FALSE(mimic.Opens(0));
    }

    Wielding wielding(ITEM_WEAPON_DUSACK);
    Floor    normal(MONSTER_KIND_NORMAL);
    EXPECT_FALSE(normal.Opens(0));
}

TEST(Dusack, SuperSteveSpheres) {
    for (int sphere : {ITEM_WEAPON_DUSACK, ITEM_WEAPON_BRAVE_ARK}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_TRUE(expanded::NoFoolsGoldActive());
    }

    Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SAX);
    EXPECT_FALSE(expanded::NoFoolsGoldActive());
}
