#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "btactstatus.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/xiao/moves_while_firing.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

using expanded::test::OptionScope;
using expanded::test::TestFloor;
using expanded::test::Wielding;

// chara out with weapon_no carrying sphere's SynthSphere (none for 0), at action (BtActStatus.action_no, the weapon action
// on when not 0): whether her move is left to her.
bool Moves(int chara, int weapon_no, int sphere, int action) {
    TestFloor     floor;
    Wielding      wielding(weapon_no, sphere);
    BT_ACT_STATUS saved = BtActStatus;
    floor.status().cur_chara = chara;
    BtActStatus.action_on = action != 0 ? 1 : 0;
    BtActStatus.action_no = action;
    bool moves = expanded::FiringLeavesMove();
    BtActStatus = saved;
    return moves;
}

// Osmond's weapons, as the weapon table owns them.
std::vector<int> OsmondWeapons() {
    std::vector<int> weapons;
    for (int weapon_no = ITEM_WEAPON_START; weapon_no < ITEM_WEAPON_END; weapon_no++) {
        const WEAPON_DATA *data = GetWeaponData(weapon_no);
        if (data != nullptr && data->owner == CHARA_OSMOND) {
            weapons.push_back(weapon_no);
        }
    }
    return weapons;
}

constexpr int kShots[] = {expanded::kXiaoDraw, expanded::kXiaoHold, expanded::kXiaoShot};

} // namespace

// The gun line, every one of them.
TEST(MovesWhileFiring, OsmondsWeaponsAreTheGunLine) {
    std::vector<int> weapons = OsmondWeapons();
    for (int weapon_no : {ITEM_WEAPON_MACHINE_GUN, ITEM_WEAPON_JACKAL, ITEM_WEAPON_LAUNCHER, ITEM_WEAPON_LAUNCHERV2, ITEM_WEAPON_BLESSING_GUN,
                          ITEM_WEAPON_SKUNK, ITEM_WEAPON_G_CRUSHER, ITEM_WEAPON_HEXA_BLASTER, ITEM_WEAPON_STAR_BREAKER, ITEM_WEAPON_SUPERNOVA,
                          ITEM_WEAPON_SNAIL, ITEM_WEAPON_SWALLOW}) {
        EXPECT_TRUE(expanded::OsmondWeapon(weapon_no)) << weapon_no;
        EXPECT_NE(std::find(weapons.begin(), weapons.end(), weapon_no), weapons.end()) << weapon_no;
    }
    EXPECT_FALSE(expanded::OsmondWeapon(ITEM_WEAPON_DIVINEBEASTTITLE));
    EXPECT_FALSE(expanded::OsmondWeapon(ITEM_WEAPON_CACTUS));
    EXPECT_FALSE(expanded::OsmondWeapon(0));
}

// Super Steve with any Osmond weapon's sphere: drawing, holding and letting go a shot leave her stick's move to her.
TEST(MovesWhileFiring, EveryOsmondSphereMoves) {
    for (int sphere : OsmondWeapons()) {
        for (int action : kShots) {
            EXPECT_TRUE(Moves(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, sphere, action)) << sphere << " " << action;
        }
    }
}

// Not shooting, she walks as ever (the game's own step, untouched).
TEST(MovesWhileFiring, OnlyWhileShooting) {
    EXPECT_FALSE(Moves(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_STAR_BREAKER, 0));
}

// Without a sphere, with an empty one or another character's weapon's, or on another slingshot, she stands still to
// shoot, as in the game.
TEST(MovesWhileFiring, OtherwiseSheStandsToShoot) {
    for (int action : kShots) {
        EXPECT_FALSE(Moves(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, 0, action)) << action;
        EXPECT_FALSE(Moves(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_DIVINEBEASTTITLE, action)) << action;
        EXPECT_FALSE(Moves(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CACTUS, action)) << action;
        EXPECT_FALSE(Moves(CHARA_XIAO, ITEM_WEAPON_DIVINEBEASTTITLE, ITEM_WEAPON_STAR_BREAKER, action)) << action;
    }
}

// Osmond himself is the game's (he moves while firing already), and no one else is moved by it.
TEST(MovesWhileFiring, OnlyXiao) {
    EXPECT_FALSE(Moves(CHARA_OSMOND, ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_JACKAL, expanded::kXiaoDraw));
}

TEST(MovesWhileFiring, NotWithTheWeaponAbilitiesOff) {
    OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    for (int sphere : OsmondWeapons()) {
        EXPECT_FALSE(Moves(CHARA_XIAO, ITEM_WEAPON_SUPER_STEVE, sphere, expanded::kXiaoDraw)) << sphere;
    }
}

// Each of Osmond's weapons fires as the game sets his gun type from it (Get_Main_EffectPtr).
TEST(MovesWhileFiring, GunTypesAreTheGames) {
    TestFloor     floor;
    BT_ACT_STATUS saved = BtActStatus;
    CUserStatus  &status = floor.status();
    status.equipped_weapon_slot[CHARA_OSMOND] = 0;
    for (int weapon_no : OsmondWeapons()) {
        status.chara_weapons[CHARA_OSMOND][0].item_no = static_cast<s16>(weapon_no);
        BtActStatus.gun_type = -1;
        Get_Main_EffectPtr(CHARA_OSMOND, 0);
        EXPECT_EQ(expanded::OsmondGunType(weapon_no), BtActStatus.gun_type) << weapon_no;
    }
    BtActStatus = saved;
    EXPECT_EQ(expanded::OsmondGunType(ITEM_WEAPON_SWALLOW), expanded::kMachineGun);
    EXPECT_EQ(expanded::OsmondGunType(ITEM_WEAPON_SNAIL), expanded::kLauncher);
    EXPECT_EQ(expanded::OsmondGunType(ITEM_WEAPON_SKUNK), expanded::kFlamethrower);
    EXPECT_EQ(expanded::OsmondGunType(ITEM_WEAPON_CACTUS), -1);
}
