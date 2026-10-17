#include "expanded/items/items.hpp"

#include "itemdata.hpp"

namespace expanded {

namespace {

// The kind flag that keeps an item off the shop's sell list and out of the trash.
constexpr s16 kKeep = 0x10;

// Items retail keeps that can be sold and thrown away.
constexpr int kTradable[] = {ITEM_FRUIT_OF_EDEN};

} // namespace

void ItemsApply() {
    static bool applied = false;

    if (!applied) {
        for (int item : kTradable) {
            if (ITEM_DATA *data = GetItemData(item)) {
                data->kind_flags &= ~kKeep;
            }
        }
        applied = true;
    }
}

} // namespace expanded
