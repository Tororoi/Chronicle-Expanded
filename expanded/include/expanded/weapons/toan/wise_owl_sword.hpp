#pragma once

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// Wise Owl Sword.
// - "Wise Owl Always Knows": while a Wise Owl Sword is owned, a line names the key (Shiny Stone, Red Berry or Pointy
//   Chestnut) a monster within kWiseOwlRange carries, as it comes in range, and again for another one.
// - "Wise Owl's Key": in hand, or as Super Steve's SynthSphere, a Wise Owl Forest floor's locked room (its
//   gate asks for the Moustache Key) opens without its key (dungeon/door_keys).
inline constexpr float kWiseOwlRange = 500.0f;

bool WiseOwlsKeyActive();

// The monster within range carrying one of Wise Owl Forest's keys, the nearest, or -1.
int WiseOwlKeyHolder(const CMonstorUnit &unit);

// Hook: each step the monsters take.
void WiseOwlStep(const CMonstorUnit &unit);

} // namespace expanded
