#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded {

// The Inferno's flame over Goro as his pull begins: a monster's fireball (kRisingFlamePack, the same in every dungeon:
// entered as a floor character), playing its flight at kRisingFlameMotionSpeed (half its cfg's 0.25), turned head down
// so its tail points up, kRisingFlameScale times its size, its head kRisingFlameLift above his feet. It rises out of
// the floor beneath him from kRisingFlameDepth under over kRisingFlameRise frames (the floor hiding what is still under
// it), and sinks back the same way once the pull ends. As his smash begins it grows to kRisingFlameSmashScale over
// kRisingFlameGrow frames, playing faster as it grows, up to kRisingFlameSmashMotionSpeed (its cfg's own), and keeps
// that size and speed until it has sunk. Goro is lit by it: kRisingFlameGlow (red, green, blue) as it rises, up to
// kRisingFlameSmashGlow as it grows, all of it fading as it sinks. Worn by Xiao with the Inferno's SynthSphere on Super
// Steve, it is sized to her height and rises while she holds the slingshot drawn (RisingFlameFired). Times are game
// frames.
inline constexpr const char *kRisingFlamePack = "dun/effect/f_boll_3.chr";
inline constexpr const char *kRisingFlameCfg = "f_boll_3.cfg";
inline constexpr int         kRisingFlameKey = 0; // its cfg's flight
inline constexpr float       kRisingFlameMotionSpeed = 0.125f;
inline constexpr float       kRisingFlameSmashMotionSpeed = 0.25f;
inline constexpr float       kRisingFlameScale = 2.2f;
inline constexpr float       kRisingFlameSmashScale = 2.75f;
inline constexpr float       kRisingFlameLift = 0.0f;
inline constexpr float       kRisingFlameDepth = 40.0f;
inline constexpr int         kRisingFlameRise = 60;
inline constexpr int         kRisingFlameGrow = 30;
inline constexpr float       kRisingFlameHeight = 16.0f; // Goro's (CharaHeight): another wears it at their height
inline constexpr int         kRisingFlameFlare = 8;
inline constexpr int         kRisingFlameFade = 60;
inline constexpr float       kRisingFlameGlow[3] = {100.0f, 75.0f, 50.0f};
inline constexpr float       kRisingFlameSmashGlow[3] = {200.0f, 150.0f, 100.0f};

// One dungeon step: rising while up, sinking otherwise; growing while he smashes. Its fireball plays on while it is up,
// or while burning_elsewhere (drawn on Xiao's pellets: inferno/fire_shot.hpp).
void RisingFlameStep(bool up, bool smashing, bool burning_elsewhere = false);
// Xiao let a charged pellet go: hers flares to its full size over kRisingFlameFlare frames, then shrinks to nothing as it
// sinks over kRisingFlameFade (unless she draws again, when it rises on from where it is).
void RisingFlameFired();
// How far it has risen (0 under the floor, 1 over him), how far it has grown (0 to 1), its scale and its motion speed.
float RisingFlameRisen();
float RisingFlameGrown();
float RisingFlameScale();
float RisingFlameMotionSpeed();
// The draw, with the dungeon's effects; its fireball drawn anywhere else, turned (pitch about x, then yaw about y) and
// sized as asked.
void RisingFlameDraw();
void RisingFlameDrawAt(const float *at, float pitch, float yaw, float scale);
// The light it adds to Goro as he draws (CharacterLight).
void RisingFlameLight(const CCharacter &character, float *ambient);
void RisingFlameReset();

} // namespace expanded
