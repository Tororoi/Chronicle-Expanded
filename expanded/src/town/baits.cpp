#include "expanded/town/baits.hpp"

#include <array>
#include <iterator>
#include <tuple>

#include "expanded/town/fish_kinds.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

struct ExtraBait {
    EsaInfo                             esa;      // its item, and the distance fish notice it from
    std::array<float, fish::kKindCount> affinity; // by fish kind
};

// Every kind's affinity 0 but kind's.
constexpr std::array<float, fish::kKindCount> OnlyLikedBy(int kind, float affinity) {
    std::array<float, fish::kKindCount> liked{};
    liked[kind] = affinity;
    return liked;
}

// The Fruit of Eden, which only the Baron Garayan goes for, and Cheese, which no fish does; noticed from 25, as most baits.
constexpr std::array kExtraBaits = {
    ExtraBait{{ITEM_FRUIT_OF_EDEN, 25.0f}, OnlyLikedBy(fish::kBaronGarayan, 1.0f)},
    ExtraBait{{ITEM_CHEESE, 25.0f}, {}},
};

static_assert(kFirstExtraBait == std::tuple_size_v<decltype(esa_info)>);

} // namespace

int ExtraBaitType(int item_no) {
    for (int i = 0; i < std::ssize(kExtraBaits); i++) {
        if (kExtraBaits[i].esa.item_no == item_no) {
            return kFirstExtraBait + i;
        }
    }
    return -1;
}

const EsaInfo &BaitInfo(int type) {
    return type >= kFirstExtraBait ? kExtraBaits[type - kFirstExtraBait].esa : esa_info[type];
}

float ExtraBaitAffinity(int kind, int type) {
    const int bait = type - kFirstExtraBait;
    if (bait < 0 || bait >= std::ssize(kExtraBaits) || kind < 0 || kind >= fish::kKindCount) {
        return 0.0f;
    }
    return kExtraBaits[bait].affinity[kind];
}

bool PickerTakesBait(int item) {
    return ExtraBaitType(item) >= 0;
}

} // namespace expanded
