#pragma once

namespace dc::game {
class CTexture;
} // namespace dc::game

namespace expanded {

// The dungeons' shared torch fire (kTorchFireImage: the flame's two sheets, its glow and the heat shimmer's mask), entered
// into the floor characters' block under names of its own (kTorchFirePrefix before each), so a floor of any dungeon shows
// the same orange flame: one without torches, and one whose torches burn another colour (each dungeon's map carries its
// own copy, under the same names). Entered once a floor, the first time it is drawn.
inline constexpr const char *kTorchFireImage = "dun/effect/fire.img";
inline constexpr const char *kTorchFirePrefix = "xp_";

// Whether it is in, entered now when it is not (false when it cannot be: not tried again until the next floor).
bool TorchFireReady();
// One of its pictures by the fire's own name ("alpha01", "d01e02", "d01e03", "lightling"), or null when it is not in.
CTexture *TorchFireTexture(const char *name);

} // namespace expanded
