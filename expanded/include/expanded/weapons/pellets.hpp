#pragma once

#include "shot_effect.hpp"

namespace expanded {

// The player's pellets: a CSHOT's slots.
inline constexpr int kPelletSlots = sizeof(CSHOT::used) / sizeof(CSHOT::used[0]);

} // namespace expanded
