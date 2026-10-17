#include <gtest/gtest.h>

#include "expanded/items/items.hpp"
#include "itemdata.hpp"

TEST(Items, FruitOfEdenCanBeSoldAndThrownAway) {
    expanded::ItemsApply();

    EXPECT_EQ(GetItemData(ITEM_FRUIT_OF_EDEN)->kind_flags & 0x10, 0);
    EXPECT_NE(GetItemData(ITEM_GOURD)->kind_flags & 0x10, 0);
    EXPECT_NE(GetItemData(ITEM_FISHING_ROD)->kind_flags & 0x10, 0);
}
