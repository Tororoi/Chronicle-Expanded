#include "expanded/town/sea.hpp"

#include <array>

#include "expanded/data/queens_canal.hpp"

namespace expanded {

namespace {

// The water's pass: EditMainDraw's DrawWater(0x15), after the fish.
constexpr int kWaterPass = 0x15;

struct Sea {
    int              map_no;
    std::string_view part;
};

constexpr std::array kSeas = {
    Sea{kEastHarborMapNo, "s0903"},
};

} // namespace

int TownPartPass(int map_no, std::string_view name, int pass) {
    // The name as the map's info has it, under its folder: gedit/s09/mds/s0903.
    name = name.substr(name.rfind('/') + 1);
    for (const Sea &sea : kSeas) {
        if (sea.map_no == map_no && sea.part == name) {
            return kWaterPass;
        }
    }
    return pass;
}

} // namespace expanded
