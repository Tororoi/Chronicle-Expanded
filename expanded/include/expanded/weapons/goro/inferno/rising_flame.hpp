#pragma once

namespace expanded {

// The Inferno's flame over Goro as his pull begins: a monster's fireball (kRisingFlamePack, the same in every dungeon:
// entered as a floor character), playing its flight, turned head down so its tail points up, kRisingFlameScale times its
// size, its head kRisingFlameLift above his feet. It rises out of the floor beneath him from kRisingFlameDepth under
// over kRisingFlameRise frames (the floor hiding what is still under it), and sinks back the same way once the pull
// ends. Times are game frames.
inline constexpr const char *kRisingFlamePack = "dun/effect/f_boll_3.chr";
inline constexpr const char *kRisingFlameCfg = "f_boll_3.cfg";
inline constexpr int         kRisingFlameKey = 0; // its cfg's flight
inline constexpr float       kRisingFlameScale = 2.75f;
inline constexpr float       kRisingFlameLift = 0.0f;
inline constexpr float       kRisingFlameDepth = 40.0f;
inline constexpr int         kRisingFlameRise = 60;

// One dungeon step: rising while up, sinking otherwise.
void RisingFlameStep(bool up);
// How far it has risen (0 under the floor, 1 over him).
float RisingFlameRisen();
// The draw, with the dungeon's effects.
void RisingFlameDraw();
void RisingFlameReset();

} // namespace expanded
