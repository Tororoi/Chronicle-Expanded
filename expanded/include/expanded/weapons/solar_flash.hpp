#pragma once

#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/dungeon/scene_light.hpp"

class CCharacter;
class CMonstorUnit;
class CSHOT;

namespace expanded {

// Solar Flash: hold guard with the sword drawn and the blade whitens as the room darkens; at full charge it is primed,
// guard or not, and the glow that grew on the weapon through the charge stays. The next swing carries the charge: as it comes forward the room flashes white
// and eases back, every monster within kSolarFlashRadius takes a light hit and is shoved away from the flash, and the
// floor is blinded (status/blind.hpp). The flash bills the weapon's wear. A charge left unused fades away. One flash
// at a time: no charge while a blinding runs. Super Steve's Solar Shot charges the slingshot the same way, and its next
// pellet carries the flash to wherever it ends, the monster it struck taking the pellet in place of the light hit.
// Times are game frames.

inline constexpr int   kSolarChargeFrames = 90;      // guard held this long primes the blade
inline constexpr int   kSolarShotChargeFrames = 120; // … and Xiao's slingshot
inline constexpr float kSolarPelletSize = 4.0f;      // the charged pellet's sprite, of a pellet's own
inline constexpr float kSolarStruckReach = 30.0f;    // a pellet that met a monster struck the nearest within this
inline constexpr int   kSolarPrimedFrames = 600;     // a primed charge unused this long fades …
inline constexpr int   kSolarFadeFrames = 30;        // … over this long, as does the glow of a charge dropped early
inline constexpr int   kSolarRampFrames = 30;        // the cyan build-up on Toan, the last frames of the charge
inline constexpr float kSolarFlashRadius = 300.0f;   // the hit's reach from the flash
inline constexpr float kSolarHitRadius = 25.0f;      // each monster's own hit sphere, centred on it
inline constexpr int   kSolarHitLife = 3;            // frames each hit sphere tests for
inline constexpr float kSolarKickSpeed = 2.0f;       // the shove away from the flash
inline constexpr float kSolarKickDecay = 0.3f;
inline constexpr float kSolarPrimedWhite = 45.0f; // the white Toan keeps while primed
inline constexpr float kSolarBladeWhite = 200.0f; // the blade's white at full charge
inline constexpr int   kSolarGuardGrace = 10;     // frames out of the guard a charge survives (a blocked hit drops it for a few)

// What differs between the weapons that carry the flash.
struct SolarProfile {
    int       weapon_no;
    int       charge_frames; // guard held this long primes it
    bool      shot;          // the charge rides the next pellet and flashes where it ends, not on the next swing
    float     damage_share;  // of the weapon's attack, as each monster's light hit (0 = none)
    float     flash_whp;     // what the flash bills the weapon, before Endurance
    float     prime_dim;     // how far the room darkens while charged (0..1)
    bool      glow;          // the weapon carries a glow while primed
    float     glow_size;     // its diameter at full charge
    float     glow_along;    // where on the weapon, from its first hit point (0) toward its second (1) …
    float     glow_axis;     // … or, when not 0, this far along the model's own length axis (the Sun Sword's hilt emblem)
    FlashLook look;
    GlowLook  glow_look = GlowLook::kGold;
    bool      judgement = false; // locked on, the judgement blade hangs over the target and the first swing drops it
    float     land_whp = 0.0f;   // what the blade's landing bills, in place of the flash's
    bool      primed_white = true; // the wielder keeps a slight white while primed
    int       drop_frames = 0;     // the judgement blade's fall, paced to land in this many frames (0 = under gravity) …
    bool      to_the_hilt = false; // … and into the ground to the hilt
    bool      bolts = false;       // the Sword of Zeus: the swing calls its lightning (sword_of_zeus.hpp)
};

extern const SolarProfile kSunSwordFlash;
// Big Bang: twice the Sun Sword's share, a cool white, and the judgement blade (its glow on the blade alone).
extern const SolarProfile kBigBangFlash;
// Super Steve carrying the Sun Sword's SynthSphere: Solar Shot.
extern const SolarProfile kSolarShotFlash;
// The Sword of Zeus: no hit of its own and an electric white eased away in two seconds; primed, the swing calls its
// lightning. Locked on, the judgement blade (a red ring for its glow) falls to the hilt on the first swing, paced to
// land as it comes forward, the bolt on the target; each later swing of the combo darkens the room as it winds up and
// brings another bolt down as it comes forward, until the combo ends or the lock goes. Not locked on, the swing is a
// volley.
extern const SolarProfile kZeusFlash;

// The charge's phase, for the tests and the other swords.
enum class SolarPhase {
    kIdle,
    kCharging,
    kPrimed,
    kWindup,  // the primed swing under way
    kDropping, // the judgement blade falling
    kChain,    // the Sword of Zeus's combo, a bolt on each swing
    kFlying,  // the charged pellet out
    kLanding, // the pellet ended: the room plunges to black before the flash
    kFading,
};

SolarPhase SolarFlashPhase();
// The glow's size, 0 (none) to 1 (primed).
float SolarGlowSize();

// The game's own charge-complete flash on the party (Toan's sword charge, Goro's): a charge is ready.
void ChargeCompleteFlash();

// One step of the charge (once a frame the monsters step unpaused).
void SolarFlashStep();
// The floor changed or its monsters were put back: everything back as it was.
void SolarFlashReset();

// The flash from point: the white, the light hits (all but the monster except), the bill (whp, or the profile's
// below 0) and the blinding.
void SolarFlashAt(const SolarProfile &profile, const float *point, int except = -1, float whp = -1.0f);
// The flash where a blast went off: the light hits on all the blast did not reach, and the blinding once its throw
// is over (kBlastThrowFrames), so the blinding does not stop the throw.
void SolarBlastFlash(const SolarProfile &profile, const float *point, float whp);

// The light hit on every monster in reach of point but except and those within spared of it (a blast's reach): a hit
// sphere on each, base damage share of attack.
int PlantSolarHits(CMonstorUnit &unit, const float *point, int base_damage, int except = -1, float spared = 0.0f);

// Hooks: Xiao fired a pellet into slot of shots, and a pellet ended at point (on a monster when struck): a primed
// Solar Shot rides the one and flashes where the other ends.
void SolarShotFired(CSHOT &shots, int slot);
void SolarShotEnded(const CSHOT &shots, int slot, const float *point, bool struck);

// Hooks: the white a charge puts on its wielder and the weapon, added to ambient; the glow's draw, with the dungeon's
// effects.
void SolarFlashLight(const CCharacter &character, float *ambient);
bool SolarFlashOwnsLight(const CCharacter &character);
void SolarGlowDraw();

} // namespace expanded
