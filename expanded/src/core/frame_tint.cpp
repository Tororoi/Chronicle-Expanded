#include "expanded/core/frame_tint.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include "expanded/enemies/enemy_events.hpp"

namespace expanded {

namespace {

struct Tint {
    const CFrame        *frame;
    std::array<float, 3> add;
};

std::vector<Tint> tints;

void ClearFrameTints() {
    tints.clear();
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(ClearFrameTints), true);

} // namespace

void SetFrameTint(const CFrame *frame, float red, float green, float blue) {
    auto found = std::find_if(tints.begin(), tints.end(), [&](const Tint &tint) { return tint.frame == frame; });
    if (red == 0.0f && green == 0.0f && blue == 0.0f) {
        if (found != tints.end()) {
            tints.erase(found);
        }
    } else if (found != tints.end()) {
        found->add = {red, green, blue};
    } else {
        tints.push_back({
            frame, {red, green, blue}
        });
    }
}

bool FrameTintBegin(const CFrame *frame, float *ambient, float *saved) {
    if (tints.empty()) {
        return false;
    }
    auto found = std::find_if(tints.begin(), tints.end(), [&](const Tint &tint) { return tint.frame == frame; });
    if (found == tints.end()) {
        return false;
    }
    std::copy_n(ambient, 4, saved);
    for (int i = 0; i < 3; i++) {
        ambient[i] += found->add[static_cast<std::size_t>(i)];
    }
    return true;
}

void FrameTintEnd(float *ambient, const float *saved) {
    std::copy_n(saved, 4, ambient);
}

} // namespace expanded
