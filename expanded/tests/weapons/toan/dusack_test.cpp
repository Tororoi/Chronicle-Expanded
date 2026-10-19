#include <gtest/gtest.h>

#include <memory>

#include "expanded/weapons/toan/dusack.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "test_support.hpp"

using expanded::test::Wielding;

namespace {

// A floor whose monster 0 is of kind.
struct Floor {
    std::unique_ptr<CMonstorUnit> unit = std::make_unique<CMonstorUnit>();

    explicit Floor(int kind) {
        unit->model[0].kind = static_cast<s16>(kind);
        unit->monster[0].base_model = 0;
    }

    ~Floor() { expanded::NoFoolsGoldInvincible(0, 0); }

    bool HitThrough() const { return expanded::NoFoolsGold(*unit, 0); }
};

} // namespace

TEST(Dusack, MimicsTakeHitsWhileTheyWake) {
    for (int kind : {MONSTER_KIND_MIMIC_SMALL, MONSTER_KIND_MIMIC_LARGE}) {
        for (int weapon : {ITEM_WEAPON_DUSACK, ITEM_WEAPON_BRAVE_ARK}) {
            Wielding wielding(weapon);
            Floor    floor(kind);
            expanded::NoFoolsGoldInvincible(0, 100);
            EXPECT_TRUE(floor.HitThrough());

            // A hit's short invincibility holds.
            expanded::NoFoolsGoldInvincible(0, 8);
            EXPECT_FALSE(floor.HitThrough());
        }
    }
}

TEST(Dusack, OthersKeepTheirInvincibility) {
    {
        Wielding wielding(ITEM_WEAPON_DAGGER);
        Floor    mimic(MONSTER_KIND_MIMIC_SMALL);
        expanded::NoFoolsGoldInvincible(0, 100);
        EXPECT_FALSE(mimic.HitThrough());
    }

    Wielding wielding(ITEM_WEAPON_DUSACK);
    Floor    normal(MONSTER_KIND_NORMAL);
    expanded::NoFoolsGoldInvincible(0, 100);
    EXPECT_FALSE(normal.HitThrough());
}

TEST(Dusack, SuperSteveSpheres) {
    for (int sphere : {ITEM_WEAPON_DUSACK, ITEM_WEAPON_BRAVE_ARK}) {
        Wielding wielding(ITEM_WEAPON_SUPER_STEVE, sphere);
        EXPECT_TRUE(expanded::NoFoolsGoldActive());
    }

    Wielding wielding(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SAX);
    EXPECT_FALSE(expanded::NoFoolsGoldActive());
}
