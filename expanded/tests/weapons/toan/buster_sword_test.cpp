#include <gtest/gtest.h>

#include <cstring>

#include "expanded/weapons/toan/buster_sword.hpp"
#include "itemdata.hpp"

namespace {

// An attachment entry as the board holds one: the item's own data.
ATTACH_LIST Entry(int item_no) {
    ATTACH_LIST entry = {};
    std::memcpy(&entry, GetAttachData(item_no), sizeof(entry));
    return entry;
}

WEAPON_HAVE Weapon(int item_no) {
    WEAPON_HAVE weapon = {};
    weapon.item_no = static_cast<s16>(item_no);
    return weapon;
}

} // namespace

TEST(BusterSword, AnAntiAttachmentIsWorthOneMoreOnIt) {
    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    for (int item = ITEM_ATTACH_DINOSLAYER; item <= ITEM_ATTACH_MAGE_SLAYER; item++) {
        int         anti = item - ITEM_ATTACH_DINOSLAYER;
        ATTACH_LIST entry = Entry(item);
        int         own = entry.vs_monster[anti];
        expanded::BusterBoostPlaced(&buster, entry);
        EXPECT_EQ(entry.vs_monster[anti], own + 1) << item;
        expanded::BusterBoostPlaced(&buster, entry); // placed again: still one more
        EXPECT_EQ(entry.vs_monster[anti], own + 1) << item;
        expanded::BusterBoostPlaced(nullptr, entry); // off the sword
        EXPECT_EQ(entry.vs_monster[anti], own) << item;
    }
}

TEST(BusterSword, OtherWeaponsAndAttachmentsKeepTheirValues) {
    WEAPON_HAVE dagger = Weapon(ITEM_WEAPON_DAGGER);
    ATTACH_LIST anti = Entry(ITEM_ATTACH_DINOSLAYER);
    int         own = anti.vs_monster[0];
    expanded::BusterBoostPlaced(&dagger, anti);
    EXPECT_EQ(anti.vs_monster[0], own);

    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    ATTACH_LIST fire = Entry(ITEM_ATTACH_FIRE);
    ATTACH_LIST before = fire;
    expanded::BusterBoostPlaced(&buster, fire);
    EXPECT_EQ(std::memcmp(&fire, &before, sizeof(fire)), 0);
}

TEST(BusterSword, ASwapBoostsThePlacedAndRestoresTheTaken) {
    WEAPON_HAVE buster = Weapon(ITEM_WEAPON_BUSTER_SWORD);
    ATTACH_LIST placed = Entry(ITEM_ATTACH_SKY_HUNTER);
    ATTACH_LIST taken = Entry(ITEM_ATTACH_MAGE_SLAYER);
    expanded::BusterBoostPlaced(&buster, taken);
    expanded::BusterBoostSwapped(buster, placed, taken);
    EXPECT_EQ(placed.vs_monster[ITEM_ATTACH_SKY_HUNTER - ITEM_ATTACH_DINOSLAYER],
              GetAttachData(ITEM_ATTACH_SKY_HUNTER)->vs_monster[ITEM_ATTACH_SKY_HUNTER - ITEM_ATTACH_DINOSLAYER] + 1);
    EXPECT_EQ(taken.vs_monster[9], GetAttachData(ITEM_ATTACH_MAGE_SLAYER)->vs_monster[9]);
}
