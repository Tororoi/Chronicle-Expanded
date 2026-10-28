#pragma once

#include "expanded/enemies/monsters.hpp"
#include "itemdata.hpp"

namespace expanded::frozen_tuna {

// The ice prison's size on each body it encases (ice_prisons.hpp), one table for the party members and the monster
// kinds (prison_scales.cpp): its scale across (x and z) and up (y), about its base; 1 and 1 for a body the table does
// not list.
struct PrisonSize {
    float horizontal = 1.0f;
    float vertical = 1.0f;
};

PrisonSize PrisonScale(Character member);
PrisonSize PrisonScale(Monster kind);

} // namespace expanded::frozen_tuna
