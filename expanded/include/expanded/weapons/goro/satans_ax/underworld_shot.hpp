#pragma once

namespace dc::game {
class CSHOT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Super Steve carrying Satan's Ax's SynthSphere: Pull of the Underworld from Xiao's slingshot. A shot held drawn
// kUnderworldShotFrames (the game's charge-complete flash) flies kUnderworldShotSize times as large (the Flamingo's
// pink pellet, as all the sphere's are: PelletWeapon) at kUnderworldShotSpeed of its speed for kUnderworldShotLife
// times as long, and while it flies the pull (satans_ax.hpp) draws the monsters toward it,
// kUnderworldShotPull times as strongly as Goro's smash does, until their bodies reach it, and its sparks stream in to it. It goes on through the monsters it meets,
// hitting each once, until a wall or its life ends it. Times are game frames.
inline constexpr int   kUnderworldShotFrames = 60;
inline constexpr float kUnderworldShotSpeed = 0.25f;
inline constexpr float kUnderworldShotLife = 2.0f;
inline constexpr float kUnderworldShotPull = 0.5f;
inline constexpr float kUnderworldShotSize = 2.0f;

// Whether Xiao is in play with the sphere on Super Steve (the Inferno's sphere too: inferno.hpp).
bool UnderworldShotActive();

// Whether the Inferno's sphere is the one, and whether she holds the slingshot drawn with it (its pull is on then too, a
// quarter as strongly: kUnderworldChargeShare).
bool UnderworldShotInferno();
bool UnderworldShotDrawn();

// The charged pellets let go so far, counting each as it is let go.
int UnderworldShotsFired();

// Each charged pellet in flight: its shots, slot, where it is and the way it flies.
template <typename Each>
void UnderworldShotsEach(Each each);

// Where the charged pellet in flight is (the one let go last), or false with none flying.
bool UnderworldShotAt(float *at);

// Hooks: a dungeon step; a pellet let go into slot; its hit planted (CSHOT::step); a pellet ended.
void UnderworldShotStep();
void UnderworldShotFired(CSHOT &shots, int slot);
void UnderworldShotHit(const CSHOT &shots, int slot, COLLISION_HIT &hit);
void UnderworldShotEnded(const CSHOT &shots, int slot);

// Whether slot of shots is a charged pellet, which goes on through a monster it meets (PelletPierces).
bool UnderworldShotPierces(const CSHOT &shots, int slot);
// Whether a hit is a charged pellet's on a monster it has hit already (MonsterHitRefused).
bool UnderworldShotRefused(int monster, const COLLISION_HIT &hit);

// Nothing charged or flying (the floor changed; tests too).
void UnderworldShotReset();

// The pellets in flight, slot by slot (UnderworldShotsEach).
int          UnderworldShotSlots();
const CSHOT *UnderworldShotIn(int i, int &slot);

template <typename Each>
void UnderworldShotsEach(Each each) {
    for (int i = 0; i < UnderworldShotSlots(); i++) {
        int          slot = -1;
        const CSHOT *shots = UnderworldShotIn(i, slot);
        if (shots != nullptr) {
            each(*shots, slot);
        }
    }
}

} // namespace expanded
