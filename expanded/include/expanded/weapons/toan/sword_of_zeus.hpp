#pragma once

namespace dc::game {
class CCameraFollow;
class CCharacter;
} // namespace dc::game

namespace expanded {

struct SolarProfile;

// Sword of Zeus "Lightning". A bolt is the Divine Beast Cave cutscene's lightning (gedit/s99/chara/lightning.chr)
// coming down where it strikes, with a thunderclap: a Big Bang blast there over four fifths its reach (kBoltBlast,
// kBoltReachShare), through any guard, as thunder whatever the weapon's selected element (its own thunder strength,
// so a monster immune to thunder takes nothing); the monster it strikes takes it where it stands, the rest are
// thrown. Billed kBoltWhp. A volley is a bolt on each of the kBoltCount nearest monsters within kBoltReach, billed
// as one. The sword's Solar Flash calls the bolts (see solar_flash.hpp); its charge attack is always the lunge, and
// held kChargeHighFrames past the lunge's threshold the lunge jumps kChargeHigh times higher and its landing is a
// bolt, the sword hanging over where it will land and falling to the hilt as he comes down. Super Steve with its
// sphere calls them from the slingshot (zeus_shot.hpp). Times are game frames.

inline constexpr float kBoltScale = 10.0f;
inline constexpr float kBoltBlast = 1.0f;      // of a Big Bang blast's falloff steps …
inline constexpr float kBoltReachShare = 0.8f; // … and of its reaches: 4×/3×/2×/1× the attack within 8/20/32/40
inline constexpr float kBoltWhp = 10.0f;
inline constexpr float kBoltReach = 300.0f;
inline constexpr int   kBoltCount = 6;
inline constexpr int   kChargeHighFrames = 180;
inline constexpr float kChargeHigh = 1.5f;
inline constexpr float kChargeAhead = 40.0f;      // the bolt this far ahead of Toan, not locked on
inline constexpr float kChargeHangHeight = 45.0f; // the hanging sword's grip over where it will land
inline constexpr int   kChargeFallFrames = 19;    // its fall, to the hilt as the lunge comes down

// Its Solar Flash: no hit of its own and an electric white eased away in two seconds; primed, the swing calls its
// lightning. Locked on, the judgement blade (a red ring for its glow) falls to the hilt on the first swing, paced to
// land as it comes forward, the bolt on the target; each later swing of the combo darkens the room as it winds up and
// brings another bolt down as it comes forward, until the combo ends or the lock goes. Not locked on, the swing is a
// volley.
extern const SolarProfile kZeusFlash;

// A bolt's thunder: the storm's first sample's second clap (a lone bolt, or a combo's first), that or its second
// sample's first clap at random (a combo's later bolts), the first sample whole (a volley, all its bolts under one), or
// none.
enum class Thunder {
    kFirst,
    kCombo,
    kVolley,
    kNone
};

// A bolt at point (still: the monster it strikes, which takes it where it stands, or -1), billed unless not, with
// its thunder; leaving the spared monsters alone (one bit each). False when the lightning is not loaded on this floor.
bool BoltAt(const float *point, int still = -1, bool bill = true, Thunder thunder = Thunder::kFirst, int spared = 0);
// A bolt on a live monster, at its feet.
bool BoltOn(int monster, bool bill = true, Thunder thunder = Thunder::kFirst);
// A volley from origin: how many bolts came down.
int BoltVolley(const float *origin);

bool ZeusActive();

// Hooks: whether a held charge may become the whirlwind (never with the Sword of Zeus); the gravity a jump falls
// under (the high lunge's, the same airtime from a higher arc); one step of the charge attack (once a frame the
// monsters step unpaused); the floor's monsters put back; the white the charge puts on the blade.
bool  WhirlwindAllowed();
float LungeGravity(float gravity);
void  ZeusChargeStep();
void  ZeusChargeReset();
void  ZeusChargeLight(const CCharacter &character, float *ambient);
// Hook: the dungeon camera's follow point was set. Through the high lunge the eye is held at its height, so Toan rises
// in view rather than the camera with him; it has the player's own height back as he lands.
void ZeusCameraHold(CCameraFollow &camera);

} // namespace expanded
