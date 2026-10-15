#include "expanded/enemies/stronger.hpp"

#include "expanded/core/options.hpp"

namespace expanded {

namespace {

const bool kRegistered = AddToggle({.key = kStrongerEnemies,
                                    .label = "Stronger Enemies",
                                    .help = "\"Stronger Enemies\"\nMonsters are as strong\nas a region deeper."});

} // namespace

bool StrongerEnemies() {
    return OptionOn(kStrongerEnemies);
}

} // namespace expanded
