#include <gtest/gtest.h>

#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/toan/evilcise.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

using expanded::test::TestFloor;

namespace {

// A party whose Toan has weapon equipped, and the curse state put back after.
struct Party {
    TestFloor    floor;
    CUserStatus &status = floor.status();

    explicit Party(int weapon) {
        status.cur_chara = CHARA_TOAN;
        Equip(CHARA_TOAN, weapon);
        status.hp[CHARA_TOAN] = 70;
    }

    ~Party() {
        // Unequipped, nobody is cursed.
        for (int chara = 0; chara < 6; chara++) {
            status.equipped_weapon_slot[chara] = -1;
        }
        expanded::CurseStep();
    }

    void Equip(int chara, int weapon, int sphere = 0) {
        status.equipped_weapon_slot[chara] = 0;
        WEAPON_HAVE &record = status.chara_weapons[chara][0];
        record = {};
        record.item_no = static_cast<s16>(weapon);
        if (sphere != 0) {
            record.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
            record.attach[0].sphere_weapon_no = static_cast<s16>(sphere);
        }
    }

    s32 &Ailments(int chara = CHARA_TOAN) { return status.ailments[chara]; }
};

} // namespace

TEST(Evilcise, CursesTheWielder) {
    Party party(ITEM_WEAPON_EVILCISE);
    expanded::CurseStep();
    EXPECT_TRUE(party.Ailments() & AILMENT_CURSE);
    EXPECT_TRUE(expanded::JealousSoulCursing(CHARA_TOAN));

    // Unequipped: freed.
    party.Equip(CHARA_TOAN, ITEM_WEAPON_DAGGER);
    expanded::CurseStep();
    EXPECT_FALSE(party.Ailments() & AILMENT_CURSE);
    EXPECT_FALSE(expanded::JealousSoulCursing(CHARA_TOAN));
}

TEST(Evilcise, LiftingItPoisonsAndLeavesOneHp) {
    Party party(ITEM_WEAPON_EVILCISE);
    expanded::CurseStep();

    party.Ailments() &= ~AILMENT_CURSE;
    expanded::CurseStep();
    EXPECT_TRUE(party.Ailments() & AILMENT_POISON);
    EXPECT_EQ(party.status.hp[CHARA_TOAN], 1);

    // The rest of the floor it stays lifted, and the poison stays.
    expanded::CurseStep();
    EXPECT_FALSE(party.Ailments() & AILMENT_CURSE);
    EXPECT_TRUE(party.Ailments() & AILMENT_POISON);
    EXPECT_TRUE(expanded::CurseAllowsAilment(AILMENT_GOO));
}

TEST(Evilcise, CursedAgainOnEveryFloor) {
    Party party(ITEM_WEAPON_EVILCISE);
    expanded::CurseStep();
    party.Ailments() &= ~AILMENT_CURSE;
    expanded::CurseStep();

    party.floor.SetUpModel(0, expanded::Monster::CAVE_BAT);
    expanded::CurseStep();
    EXPECT_TRUE(party.Ailments() & AILMENT_CURSE);
}

TEST(Evilcise, ImmuneToOtherHarm) {
    Party party(ITEM_WEAPON_EVILCISE);
    expanded::CurseStep();
    EXPECT_FALSE(expanded::CurseAllowsAilment(AILMENT_POISON));
    EXPECT_FALSE(expanded::CurseAllowsAilment(AILMENT_FREEZE));
    EXPECT_FALSE(expanded::CurseAllowsAilment(AILMENT_GOO));
    EXPECT_TRUE(expanded::CurseAllowsAilment(AILMENT_STAMINA));

    party.Ailments() |= AILMENT_GOO | AILMENT_STAMINA;
    expanded::CurseStep();
    EXPECT_EQ(party.Ailments(), AILMENT_CURSE | AILMENT_STAMINA);
}

TEST(Evilcise, KnockOutIsNotALift) {
    Party party(ITEM_WEAPON_EVILCISE);
    expanded::CurseStep();

    party.Ailments() = AILMENT_KNOCKED_OUT;
    party.status.hp[CHARA_TOAN] = 0;
    expanded::CurseStep();

    // Revived: cursed again, no price.
    party.Ailments() = 0;
    party.status.hp[CHARA_TOAN] = 35;
    expanded::CurseStep();
    EXPECT_EQ(party.Ailments(), AILMENT_CURSE);
    EXPECT_EQ(party.status.hp[CHARA_TOAN], 35);
}

TEST(Evilcise, SuperSteveSphereCursesXiao) {
    Party party(ITEM_WEAPON_DAGGER);
    party.status.hp[CHARA_XIAO] = 60;
    party.Equip(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_EVILCISE);
    expanded::CurseStep();
    EXPECT_TRUE(party.Ailments(CHARA_XIAO) & AILMENT_CURSE);
    EXPECT_FALSE(party.Ailments(CHARA_TOAN) & AILMENT_CURSE);

    party.Equip(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DAGGER);
    expanded::CurseStep();
    EXPECT_FALSE(party.Ailments(CHARA_XIAO) & AILMENT_CURSE);
}
