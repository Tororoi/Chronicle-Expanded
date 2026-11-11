#include "expanded/enemies/in_play.hpp"

#include <algorithm>
#include <array>

#include "collision.hpp"
#include "frame.hpp"

namespace expanded {

namespace {

std::array<CCPoly, kMonsterPolys> picked; // one body's, before they go in

} // namespace

int PickUpBodyPolys(CFrame &body, CCPoly *list, int count, const CBoxVu0 &box) {
    int room = std::max(0, kMonsterPolys - count);
    if (room == 0) {
        return 0;
    }
    int found = std::min(body.PickUpNearPoly(picked.data(), box), kMonsterPolys);
    int added = std::min(found, room);
    std::copy_n(picked.begin(), added, list + count);
    return added;
}

} // namespace expanded
