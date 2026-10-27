#pragma once

class CCharacter;
struct COLLISION_HIT;

namespace expanded {

struct SolarProfile;

// Big Bang "Detonate": the whirlwind is a blast at Toan's feet, the falloff blast (weapons/blast.hpp) with the game's
// explosion in place of the tornado, the spin's own hits taken away so nothing is struck twice; it bills kDetonateWhp.
// The blade whitens across the charge that becomes it. Its guard charge is Solar Flash with the judgement blade
// (kBigBangFlash). While it is in hand, or Super Steve carries its SynthSphere, the game's explosions cannot hurt the
// wielder.
inline constexpr float kDetonateWhp = 20.0f;
inline constexpr float kWhirlExplosion = 1.0f;   // the whirlwind's explosion (explosion.chr), of its own size …
inline constexpr float kLandingExplosion = 1.5f; // … and the judgement blade's landing's
inline constexpr float kWhirlRing = 70.0f;       // their shock rings' reach
inline constexpr float kLandingRing = 90.0f;

bool DetonateActive();

// Its Solar Flash: twice the Sun Sword's share, a cool white, and the judgement blade (its glow on the blade alone).
extern const SolarProfile kBigBangFlash;

// Big Bang's explosion at point, scale times its size: explosion.chr playing once, the look alone, loaded with the
// floor while Big Bang is with the party (the game's bomb explosion until it is).
void DetonationAt(const float *point, float scale, float ring);

// Hooks: ToanKey_Play started the whirlwind (its tornado just set) and asks whether the spin's sword hits; the player's
// hit test found a hit (BtCheckDamageProc): whether it passes Toan by; the light the charge puts on the blade.
void WhirlwindStarted();
bool WhirlSwordHits();
bool PlayerHitSkipped(const COLLISION_HIT &hit);
void DetonateChargeLight(const CCharacter &character, float *ambient);

} // namespace expanded
