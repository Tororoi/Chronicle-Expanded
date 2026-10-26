#pragma once

class CCharacter;
class CSHOT;
struct BT_SHOT_EFFECT;

namespace expanded {

// Hercules' Wrath "Air Strike", the guard charge's second level. With Hercules' Wrath in Ungaga's hand, or Super Steve
// carrying its SynthSphere, the guard held kMirageGuardFrames plants the Mirage's decoy (mirage.hpp, the first level;
// it stands kHerculesMirageFrames). Held kAirStrikeChargeFrames more, the room darkens toward kAirStrikeDim and the
// weapon turns gold (kAirStrikeGold added to its light, on a ramp that barely moves at first and is full at the end), the
// copy's weapon with it; letting the guard go before then drops it all. Charged, the game's charge-complete flash plays
// and the strike is primed for as long as a decoy stands: a new decoy keeps it, and as the last one dissolves the dark and
// the gold go with it, and the strike with them. One charge a hold of the guard.
//   - Ungaga's: his next combo swing reaching kAirStrikeSwingFrame (the end of his first swing's hit window) calls it on
//     the decoy. A sparkle (gedit/s78's cutscene e508_ex, kSparkleFile) plays its first clip at the copy's spear tip
//     (its dcol0), following it, then its second at kSparkleSecondRate where the tip was, at the decoy's height. Along
//     the second the room plunges from the dim to black (frames kSparkleDarkFrom to kSparkleBlastFrame), then the blast
//     lands there: Big Bang's falloff at kAirStrikeReach times its reach (4, 3, 2 and 1 times the attack within 20, 50,
//     80 and 100), through any guard, felt as nothing and wearing no weapon HP as it lands; the strike bills
//     kAirStrikeWhp once. The decoy is dispelled and the Sword of Zeus's white flash eases back over two seconds. From
//     kSparkleFadeFrom to kSparkleFadeTo the sparkle and the gold fade out together, and it ends.
//   - Super Steve's: the first pellet Xiao fires while primed carries it, the slingshot gold. Where the pellet ends (on a
//     monster, at a wall, at the end of its range, or wherever it is after kAirStrikeMissFrames) the sparkle's first clip
//     plays; its second and the blast are on the floor under that spot (the highest at or below the pellet's height plus
//     kAirStrikeFloorAbove; the decoy's height when there is none). Once the pellet is away, the strike comes even if the
//     decoy goes in its flight. The strike bills Super Steve.
// It all goes when the weapon leaves the hand (the room's light back at once), the active character changes or the
// floor does. Times are game frames.
inline constexpr int   kAirStrikeChargeFrames = 300;
inline constexpr float kAirStrikeDim = 0.35f;
inline constexpr float kAirStrikeGold[3] = {150.0f, 130.0f, 50.0f};
inline constexpr float kAirStrikeGoldCurve = 4.0f;    // the gold's ramp: (e^(4f) - 1) / (e^4 - 1) of the charge f
inline constexpr float kAirStrikeSwingFrame = 677.0f; // Ungaga's first combo swing: its hit window ends here …
inline constexpr float kAirStrikeSwingEnd = 686.0f;   // … and the second swing starts here
inline constexpr float kAirStrikeReach = 2.0f;
inline constexpr float kAirStrikeWhp = 20.0f;
inline constexpr int   kAirStrikeMissFrames = 180;
inline constexpr float kAirStrikeFloorAbove = 10.0f;

// The sparkle: its first clip (KEY 0, frames 2 to 16) at its own 0.3 a frame, its second (KEY 1, frames 21 to 80) at
// kSparkleSecondRate. Its third (81 to 96) never plays.
inline constexpr const char *kSparkleFile = "gedit/s78/chara/e508_ex.chr";
inline constexpr int         kSparkleShots = 1;
inline constexpr float       kSparkleFirstStart = 2.0f, kSparkleFirstEnd = 16.0f;
inline constexpr float       kSparkleSecondStart = 21.0f, kSparkleSecondEnd = 80.0f, kSparkleSecondRate = 0.2f;
inline constexpr float       kSparkleDarkFrom = 21.0f, kSparkleBlastFrame = 36.0f;
inline constexpr float       kSparkleFadeFrom = 60.0f, kSparkleFadeTo = 68.0f;

bool AirStrikeActive();

enum class AirStrikePhase {
    kIdle,
    kCharging,
    kPrimed,
    kFlying,   // Xiao's primed pellet out
    kStriking, // the sparkle playing
};

AirStrikePhase AirStrikeState();
// The weapon's gold, 0 to 1.
float AirStrikeGold();
// The sparkle's description, as the weapon's shot effects load it (shot_effects.hpp).
BT_SHOT_EFFECT *AirStrikeSparkle();

// Hooks: a step (once a frame the monsters step unpaused); Xiao fired a pellet into slot of shots, and a pellet ended at
// point; the gold on the weapon in hand and the decoy's, added to ambient.
void AirStrikeStep();
void AirStrikeFired(const CSHOT &shots, int slot);
void AirStrikeEnded(const CSHOT &shots, int slot, const float *point);
void AirStrikeLight(const CCharacter &character, float *ambient);

// Everything dropped (the floor changed), for tests too.
void AirStrikeReset();

} // namespace expanded
