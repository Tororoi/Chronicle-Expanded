#pragma once

#include "expanded/enemies/monsters.hpp"
#include "itemdata.hpp"

namespace expanded::frozen_tuna {

// The ice prison's size on each body it encases (ice_prisons.hpp), one table for the party members and the monster
// kinds (prison_scales.cpp): its scale, 1 for a body the table does not list.
float PrisonScale(Character member);
float PrisonScale(Monster kind);

} // namespace expanded::frozen_tuna
