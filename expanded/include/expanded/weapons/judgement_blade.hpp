#pragma once

namespace dc::game {
class CCharacter;
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The judgement blade: a copy of the sword in hand, kJudgementScale times its size, hanging point-down over a monster
// and fading in, then let fall under gravity to the ground at the monster's feet. Big Bang hangs it over the target it
// is locked on to while its Solar Flash is primed and drops it on the first swing; the landing is the blast. Times
// are game frames.

inline constexpr float kJudgementScale = 2.0f;
inline constexpr float kJudgementMargin = 2.0f;    // the tip over the monster's height
inline constexpr float kJudgementLength = 10.1f;   // the sword's reach from its grip, at its own size
inline constexpr float kJudgementGravity = 500.0f; // units a second squared
inline constexpr int   kJudgementFadeFrames = 15;

// Hangs the copy over monster (fading in; a new monster takes it over at once).
void JudgementHover(CMonstorUnit &unit, int monster);
// Hangs the copy with its grip height over point, shown that much (0..1), with no monster under it.
void JudgementHoverAt(const float *point, float height, float shown);
// Lets the hanging copy fade away.
void JudgementRelease();
// Lets the hanging copy fall: false when none hangs. Paced, it comes down in frames at an even speed rather than under
// gravity; to the hilt, until its grip is at the ground (the blade in it) rather than its tip.
bool JudgementDrop(int frames = 0, bool to_the_hilt = false);
bool JudgementHanging();
bool JudgementFalling();
// The landing, once: true with where it struck (the monster's feet).
bool JudgementLanded(float *point);
// Takes it down at once (the floor changed, the weapon was put away).
void JudgementReset();
// Where its glow sits (its middle), or false when none shows; and how much of it shows (0..1).
bool  JudgementGlowPoint(float *point);
float JudgementShown();
// Whether character is the copy (its draw takes the charged blade's white, hilt and all).
bool JudgementIsCopy(const CCharacter &character);

// One step of the fade and the fall (once a frame the monsters step unpaused), and its draw.
void JudgementStep();
void JudgementDraw();

} // namespace expanded
