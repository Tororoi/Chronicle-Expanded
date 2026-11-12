#include "expanded/core/item_models.hpp"

#include "data/data_read.hpp"
#include "expanded/data/gold_bar.hpp"
#include "itemdata.hpp"

namespace expanded {

const char *ExpandedItemModel(int item_no) {
    if (item_no == ITEM_GOLD_BULLION && DataFileSize(kGoldBarModel) > 0 && DataFileSize(kGoldBarPictures) > 0) {
        return kGoldBar;
    }
    return nullptr;
}

} // namespace expanded
