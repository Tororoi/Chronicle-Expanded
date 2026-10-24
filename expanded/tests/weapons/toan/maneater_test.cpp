#include <gtest/gtest.h>

#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/toan/maneater.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::test::TestFloor;

namespace {

// A party whose Toan has weapon equipped (its WHP whp of 100), and the curse state put back after.
struct Party {
    TestFloor    floor;
    CUserStatus &status = floor.status();

    explicit Party(int weapon, float whp = 100.0f) {
        status.cur_chara = CHARA_TOAN;
        Equip(CHARA_TOAN, weapon, 0, whp);
        status.hp[CHARA_TOAN] = status.max_hp[CHARA_TOAN] = 70;
    }

    ~Party() {
        for (int chara = 0; chara < 6; chara++) {
            status.equipped_weapon_slot[chara] = -1;
        }
        expanded::CurseStep();
        expanded::BloodPriceStep();
    }

    void Equip(int chara, int weapon, int sphere = 0, float whp = 100.0f) {
        status.equipped_weapon_slot[chara] = 0;
        WEAPON_HAVE &record = status.chara_weapons[chara][0];
        record = {};
        record.item_no = static_cast<s16>(weapon);
        record.durability = 100;
        record.durability_f = whp;
        if (sphere != 0) {
            record.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
            record.attach[0].sphere_weapon_no = static_cast<s16>(sphere);
        }
    }

    WEAPON_HAVE &Sword(int chara = CHARA_TOAN) { return status.chara_weapons[chara][0]; }

    // A second of game frames.
    void Second() {
        for (int frame = 0; frame < expanded::kBloodPriceFrames; frame++) {
            expanded::BloodPriceStep();
        }
    }
};

} // namespace

TEST(Maneater, CursesTheWielderAndLiftingItCostsNothing) {
    Party party(ITEM_WEAPON_MANEATER);
    expanded::CurseStep();
    EXPECT_TRUE(party.status.ailments[CHARA_TOAN] & AILMENT_CURSE);
    EXPECT_TRUE(expanded::BloodPriceCursing(CHARA_TOAN));

    party.status.ailments[CHARA_TOAN] &= ~AILMENT_CURSE;
    expanded::CurseStep();
    expanded::CurseStep();
    EXPECT_EQ(party.status.ailments[CHARA_TOAN], 0);
    EXPECT_EQ(party.status.hp[CHARA_TOAN], 70);
    // Not immune, as the Evilcise's curse is.
    party.status.ailments[CHARA_TOAN] |= AILMENT_CURSE;
    EXPECT_TRUE(expanded::CurseAllowsAilment(AILMENT_POISON));
}

TEST(Maneater, AtLowWhpItFeedsOneHpASecond) {
    Party party(ITEM_WEAPON_MANEATER, 10.0f);
    party.Second();
    EXPECT_EQ(party.status.hp[CHARA_TOAN], 69);
    EXPECT_FLOAT_EQ(party.Sword().durability_f, 11.0f);
    // Out of the warning: no more.
    party.Second();
    EXPECT_EQ(party.status.hp[CHARA_TOAN], 69);
    EXPECT_FLOAT_EQ(party.Sword().durability_f, 11.0f);
}

TEST(Maneater, NeverFatalNeverABrokenSword) {
    {
        Party party(ITEM_WEAPON_MANEATER, 5.0f);
        party.status.hp[CHARA_TOAN] = 1;
        party.Second();
        EXPECT_EQ(party.status.hp[CHARA_TOAN], 1);
        EXPECT_FLOAT_EQ(party.Sword().durability_f, 5.0f);
    }
    {
        Party party(ITEM_WEAPON_MANEATER, 0.0f);
        party.Second();
        EXPECT_EQ(party.status.hp[CHARA_TOAN], 70);
    }
}

TEST(Maneater, SuperStevesSphereFeedsOnXiao) {
    Party party(ITEM_WEAPON_DAGGER, 5.0f);
    party.Equip(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_MANEATER, 5.0f);
    party.status.hp[CHARA_XIAO] = party.status.max_hp[CHARA_XIAO] = 40;
    party.Second();
    EXPECT_EQ(party.status.hp[CHARA_XIAO], 39);
    EXPECT_FLOAT_EQ(party.Sword(CHARA_XIAO).durability_f, 6.0f);
    EXPECT_EQ(party.status.hp[CHARA_TOAN], 70); // the Dagger feeds on no one
    expanded::CurseStep();
    EXPECT_TRUE(party.status.ailments[CHARA_XIAO] & AILMENT_CURSE);
}
