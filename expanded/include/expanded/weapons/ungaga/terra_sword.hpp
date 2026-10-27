#pragma once

#include <numbers>

#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/kick_back.hpp"

namespace dc::game {
class CCharacter;
class CMonstorUnit;
struct BT_SHOT_EFFECT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Terra Sword "Big Rock": with the Terra Sword in Ungaga's hand, or Super Steve carrying its SynthSphere, holding the
// guard pose kRockChargeFrames (the clock runs from the first frame in the pose while R1 stays down, and starts again
// once it is let go) turns the weapon green (kRockGreen added to its light, growing evenly with the charge) and, as it
// is full, plays the game's charge-complete flash: the drop is primed, and waits, R1 held or not, until the wielder is
// locked on to a living monster. Then something drops on that monster: its bottom kDropHeight over the floor under it,
// falling from rest at kDropGravity a frame (about two and a half seconds) and following it across the floor. It grows
// from nothing to its size over the first kDropGrowDrop of the fall, its round shadow (ground_shadow.hpp) over the
// first kShadowGrowDrop, and the monster darkens under it, its own light easing down with the fall to its share at the
// floor. The green holds until the fall ends and fades over kRockGreenFadeFrames from there. No charge starts while a
// drop is up.
//   - Ungaga's: Master Utan's boulder (gedit/s96's iwa.chr, kRockRadius about its centre), turned to his facing, coming
//     to rest kRockSink into the floor. Where it lands, every living monster whose body comes within kRockBlastRadius
//     takes kRockBlastShare times the weapon's attack and is thrown away from it at kRockKick (twice Big Bang's throw;
//     one right under it from kRockKickLead past it on the far side from Ungaga, so toward him); the game's shockwave
//     (syougekiha) bursts on the floor there at kShockScale with the crash of Master Utan's boulder (kRockCrashSound),
//     the controller rumbles as at a knockdown blow, and the drop costs kRockWhp. The monster darkens to
//     kRockTargetDim.
//   - Super Steve's: a nut (gedit/s04's e114kinomi.chr) kNutScale times its size, which comes down on the monster's
//     head (its kind's height over its root, the nut's radius over that) and bonks it that frame (kNutBonkSound):
//     kNutBonkShare times the attack, the monster shoved from Xiao as a swing shoves (kNutKick) and confused for as
//     long as the nut stays (status/confuse.hpp). The nut hops off at kNutHopUp, drifting kNutHopSide square to Xiao's
//     line to the monster (to a side picked at the drop) and falling at kNutHopGravity, to the floor where it comes
//     down. The monster darkens to kNutTargetDim.
// A drop whose monster dies or leaves as it falls comes down where it is, onto the floor: the boulder lands as it would
// on a monster, the nut simply rests. Down, either rests kDropRestFrames sunk into the floor as a solid column
// (solid_column.hpp: its radius, up to its top), then fades over kDropFadeFrames and is gone, its shadow gone as it
// starts to fade; under the boulder, each living monster whose root is within its radius takes kRockPinShare times the
// attack every kRockPinFrames. The nut's monster comes to its senses as the nut goes.
// The drops' hits pass any guard, wear no weapon HP and carry none of the weapon's abilities (no element, ailment,
// critical, steal or drain); one that its monster did not take (invincible that frame) is planted again the next frame,
// kDropHitTries times in all. It all goes when the wielder changes or puts the weapon away, and with the floor. Times
// are game frames, speeds per frame.
inline constexpr int   kRockChargeFrames = 300;
inline constexpr float kRockGreen[3] = {80.0f, 150.0f, 20.0f};
inline constexpr int   kRockGreenFadeFrames = 30;
inline constexpr float kDropHeight = 500.0f;
// 180 a second, squared: from kDropHeight it lands as fast as a drop of 300 does at 300 a second, squared (about 424
// a second).
inline constexpr float kDropGravity = 300.0f * 300.0f / kDropHeight / 3600.0f;
inline constexpr float kDropGrowDrop = 150.0f;
inline constexpr float kShadowGrowDrop = 300.0f;
inline constexpr float kDropLeast = 0.01f; // the size it starts at, of its own: nothing would be a singular matrix
inline constexpr int   kDropRestFrames = 1200;
inline constexpr int   kDropFadeFrames = 30;
// A planted hit's least radius (it sits on the monster's largest body sphere).
inline constexpr float kDropHitRadius = 6.0f;
inline constexpr int   kDropHitTries = 4;

// The boulder.
inline constexpr const char *kRockFile = "gedit/s96/chara/iwa.chr";
inline constexpr const char *kRockCfg = "info.cfg";
inline constexpr float       kRockRadius = 20.4f;
inline constexpr float       kRockSink = 4.0f;
inline constexpr float       kRockTargetDim = 0.55f;
inline constexpr float       kRockBlastRadius = 26.0f; // the boulder's 21, and 5
inline constexpr float       kRockBlastShare = 4.0f;
inline constexpr KickBack    kRockKick = {std::numbers::sqrt2_v<float> * kBlastKick.speed, kBlastKick.decay};
inline constexpr float       kRockKickLead = 1.0f;
inline constexpr float       kRockKickUnder = 2.0f; // a monster whose root is this near the spot is under it
inline constexpr float       kRockWhp = 10.0f;
inline constexpr float       kRockPinShare = 1.0f;
inline constexpr int         kRockPinFrames = 60;

// The boulder's crash: the sound of the boulder Master Utan throws as it lands in his first scene (SE 1747: program
// 113, note 26 of his arena set's bank), the game's other sounds ducked under it.
inline constexpr OwnSound kRockCrashSound = {.path = "sound/set/snd201.snd", .bank = "d02b_g.hd", .program = 113, .note = 26, .frames = 100};

// The shockwave: syougekiha's one clip (frames 2 to 31) at its own rate, grown from its 3 to the boulder's 21.
inline constexpr const char *kShockFile = "dun/mainchara/wep_eff/syougekiha.chr";
inline constexpr int         kShockShots = 1;
inline constexpr float       kShockRate = 0.5f;
inline constexpr float       kShockScale = 21.0f / 3.0f;

// The nut: 1.9 about its centre at its own size.
inline constexpr const char *kNutFile = "gedit/s04/chara/e114kinomi.chr";
inline constexpr const char *kNutCfg = "e114kinomi.cfg";
inline constexpr float       kNutScale = 2.0f;
inline constexpr float       kNutRadius = 1.9f * kNutScale;
inline constexpr float       kNutSink = 0.5f;
inline constexpr float       kNutTargetDim = 0.8f;
inline constexpr float       kNutBonkShare = 0.5f;
inline constexpr KickBack    kNutKick = {1.2f, 0.2f};
inline constexpr float       kNutHopUp = 80.0f / 60.0f;
inline constexpr float       kNutHopSide = 45.0f / 60.0f;
inline constexpr float       kNutHopGravity = 400.0f / 3600.0f;
inline constexpr int         kNutHopAimFrames = 15; // where the hop comes down, worked out again as the nut falls

// The nut's bonk: the knock it makes landing on Toan's head in Brownboo Village's scene (SE 350: program 46, note 50 of
// the village's set's bank).
inline constexpr OwnSound kNutBonkSound = {.path = "sound/set/snd14.snd", .bank = "s04a_g.hd", .program = 46, .note = 50, .frames = 12};

enum class BigRockPhase {
    kIdle,
    kCharging,
    kPrimed,
};

BigRockPhase BigRockState();
// The weapon's green, 0 to 1.
float BigRockGreen();
// Whether a drop is up (falling, hopping off or resting), and falling; where it is (x, its centre's height, z) and the
// floor under it; its size (of its own) and how solid it is (0 to 1); its frames at rest (-1 before it rests).
bool  BigRockUp();
bool  BigRockFalling();
void  BigRockAt(float *at);
float BigRockGround();
float BigRockScale();
float BigRockAlpha();
int   BigRockRestAge();
// Whether a hit is one of the drops' (no guard stops it, it wears no weapon HP).
bool BigRockHit(const COLLISION_HIT &hit);
// The shockwave's description, as the weapon's shot effects load it (shot_effects.hpp).
BT_SHOT_EFFECT *BigRockShockwave();

// Hooks: a step (once a frame the monsters step unpaused); the draw, after the monsters; a monster took a hit
// (CheckDmg); the green on the weapon in hand, added to ambient.
void BigRockStep();
void BigRockDraw();
void BigRockHitTaken(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);
void BigRockLight(const CCharacter &character, float *ambient);

// Everything dropped (the floor changed), for tests too.
void BigRockReset();

} // namespace expanded
