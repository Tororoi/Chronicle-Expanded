#pragma once

#include "expanded/dungeon/atla_drops.hpp"

namespace expanded {

// The atla an Atla Gemron drops in a dungeon (clamped to the seven): 5 % the Atlamillia Sword (from Sun & Moon Temple
// on; before it a weapon's sphere), 15 % its SynthSphere, 10 % the dungeon's featured weapon's (Demon Shaft has none),
// else the SynthSphere of a weapon from the tiers the dungeon draws from whose owner has joined the party. Rolled on
// ModRandom.
AtlaDrop RollAtlaGemronDrop(int dungeon);

} // namespace expanded
