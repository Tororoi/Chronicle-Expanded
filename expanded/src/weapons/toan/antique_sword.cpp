#include "expanded/weapons/toan/antique_sword.hpp"

#include <algorithm>

#include "dungeonmap.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "snd.hpp"

namespace expanded {

namespace {

// A real chest's mark: its circle's rim and centre.
constexpr spRGBA kRim = {0xF0, 0xC8, 0x30, 0x80};
constexpr spRGBA kCentre = {0xFF, 0xF4, 0xA8, 0x80};

void Fill(int x, int y, int width, int height, spRGBA colour) {
    set2DSpriteC4(Vif1Packet, CRect_i_(x, y, width, height), &colour, &colour, &colour, &colour);
}

} // namespace

int AntiqueSwordPrice(int item_no, int sell, int price) {
    if (item_no != ITEM_WEAPON_ANTIQUE_SWORD || !WeaponAbilitiesOn()) {
        return price;
    }
    return sell != 0 ? kAntiqueSwordSellPrice : kAntiqueSwordBuyPrice;
}

int SphereSellPrice(int item_no, int sphere_weapon_no, int price) {
    bool antique = item_no == ITEM_ATTACH_SYNTHESIS_SPHERE && sphere_weapon_no == ITEM_WEAPON_ANTIQUE_SWORD;
    return antique && WeaponAbilitiesOn() ? kAntiqueSwordSellPrice : price;
}

bool TreasureSenseActive() {
    return WeaponActive(ITEM_WEAPON_ANTIQUE_SWORD);
}

bool RealChest(const CDungeonMap &map, int box) {
    // A mimic takes a chest slot, its event pointing at the slot.
    return std::ranges::none_of(map.events, [box](const DUNGEON_EVENT &event) {
        return event.kind == DNG_EVENT_MIMIC && event.index == box;
    });
}

void TreasureSenseMark(const CDungeonMap &map, int box, int x, int y) {
    if (!TreasureSenseActive() || !RealChest(map, box)) {
        return;
    }
    // The icon's circle is 4 across at (2, 2) in the 8x8 cell, its centre 2 across at (3, 3); a big chest's is a pixel
    // wider all round, 6 across at (1, 1).
    if (map.boxes[box].kind == TREASURE_BOX_LARGE) {
        Fill(x + 3, y + 1, 2, 1, kRim);
        Fill(x + 2, y + 2, 1, 1, kRim);
        Fill(x + 5, y + 2, 1, 1, kRim);
        Fill(x + 1, y + 3, 1, 2, kRim);
        Fill(x + 6, y + 3, 1, 2, kRim);
        Fill(x + 2, y + 5, 1, 1, kRim);
        Fill(x + 5, y + 5, 1, 1, kRim);
        Fill(x + 3, y + 6, 2, 1, kRim);
        Fill(x + 3, y + 2, 2, 1, kCentre);
        Fill(x + 2, y + 3, 4, 2, kCentre);
        Fill(x + 3, y + 5, 2, 1, kCentre);
    } else {
        Fill(x + 3, y + 2, 2, 1, kRim);
        Fill(x + 2, y + 3, 1, 2, kRim);
        Fill(x + 5, y + 3, 1, 2, kRim);
        Fill(x + 3, y + 5, 2, 1, kRim);
        Fill(x + 3, y + 3, 2, 2, kCentre);
    }
}

} // namespace expanded
