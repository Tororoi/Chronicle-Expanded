#include "expanded/town/fishing.hpp"

#include "expanded/weapons/ungaga/scorpion.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"

namespace expanded {

float BaitNoticeRadius(int item_no, float radius) {
    return VenomLureRadius(item_no, BaitFlamboyanceRadius(radius));
}

} // namespace expanded
