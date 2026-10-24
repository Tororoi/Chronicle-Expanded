#include <gtest/gtest.h>

#include <memory>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/toan/baselard.hpp"
#include "expanded/weapons/toan/claymore.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "test_support.hpp"

namespace {

// Toan with weapon_no in hand, his sword's grip at (10, 0, 0), and a swing hit planted 18 units out from it.
struct Swing {
    expanded::test::TestFloor       floor;
    expanded::test::Wielding        wielding;
    std::unique_ptr<CCollisionData> hits = std::make_unique<CCollisionData>();
    CCollisionData                 *saved_hits = NowColData;
    CFrame                          grip;
    CFrame                         *saved_frame = MainWeapon.frame;
    CCharacter                     *saved_weapon = NowWeapon;
    float                           tip[4] = {10.0f, 0.0f, 18.0f, 1.0f};

    explicit Swing(int weapon_no, int owner = 0) : wielding(weapon_no) {
        NowColData = hits.get();
        grip.SetPosition(10.0f, 0.0f, 0.0f);
        MainWeapon.frame = &grip;
        NowWeapon = &MainWeapon;
        int hit = hits->Set(tip, 20, 1, 6.2f, 0.0f, 2, 2, 0, 0);
        hits->hit[hit].owner = owner;
    }

    ~Swing() {
        NowColData = saved_hits;
        MainWeapon.frame = saved_frame;
        NowWeapon = saved_weapon;
    }

    int Planted() {
        int n = 0;
        for (int i = 0; i < 96; i++) {
            n += hits->active[i] != 0;
        }
        return n;
    }
};

} // namespace

TEST(Claymore, EachSwingHitIsPlantedAgainAtTheStockHitPoint) {
    Swing swing(ITEM_WEAPON_CLAYMORE);
    expanded::GreatswordHits(swing.tip);
    ASSERT_EQ(swing.Planted(), 2);
    const COLLISION_HIT &stock = swing.hits->hit[1];
    EXPECT_FLOAT_EQ(stock.pos[0], 10.0f);
    EXPECT_FLOAT_EQ(stock.pos[2], 18.0f / expanded::kGreatswordScale);
    EXPECT_EQ(stock.damage, 20);
    EXPECT_FLOAT_EQ(stock.radius, 6.2f);
    EXPECT_EQ(stock.owner, 0);
}

TEST(Claymore, OnlyToansSwingsWithTheClaymore) {
    {
        Swing swing(ITEM_WEAPON_DAGGER);
        expanded::GreatswordHits(swing.tip);
        EXPECT_EQ(swing.Planted(), 1);
    }
    {
        Swing swing(ITEM_WEAPON_CLAYMORE, 1); // Xiao's
        expanded::GreatswordHits(swing.tip);
        EXPECT_EQ(swing.Planted(), 1);
    }
}

TEST(Claymore, ThrowsAsTheBaselardDoes) {
    COLLISION_HIT swing = {};
    swing.owner = 0;
    {
        expanded::test::Wielding claymore(ITEM_WEAPON_CLAYMORE);
        EXPECT_TRUE(expanded::HeavyHandHit(swing));
    }
    COLLISION_HIT pellet = {};
    pellet.owner = 1;
    pellet.attack_no = 0;
    expanded::test::Wielding steve(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_CLAYMORE);
    EXPECT_TRUE(expanded::HeavyHandHit(pellet));
}
