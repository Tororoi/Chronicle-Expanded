#include "expanded/weapons/ruby/fairys_ring.hpp"

#include <algorithm>
#include <format>

#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"

namespace expanded {

int FairyFortunePoints(int fish_kind, int points) {
    if (std::ranges::contains(kGarayans, fish_kind) || !WeaponOwned(ITEM_WEAPON_FAIRY_S_RING)) {
        return points;
    }
    LogEvent("fairy_fortune", std::format("\"kind\":{},\"points\":{},\"paid\":{}", fish_kind, points, points * kFairyFortune));
    return points * kFairyFortune;
}

} // namespace expanded
