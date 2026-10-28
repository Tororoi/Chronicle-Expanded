#include "expanded/town/fishing.hpp"

#include "expanded/weapons/toan/mardan.hpp"
#include "expanded/weapons/ungaga/scorpion.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"

namespace expanded {

int FishKind(int kind, int rare_interval) {
    return MardanFishKind(kind, rare_interval);
}

float FishSize(float size, const FishInfo &info) {
    return AriseFishSize(size, info);
}

float BaitNoticeRadius(int item_no, float radius) {
    return VenomLureRadius(item_no, BaitFlamboyanceRadius(radius));
}

} // namespace expanded
