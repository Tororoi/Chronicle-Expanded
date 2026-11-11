#pragma once

namespace dc::game {
class CSHOT_EFFECT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Last Judgement's growing hammer: while Goro holds the guard with Last Judgement in hand the hammer grows, evenly, to
// kJudgementGrownScale times its size over kJudgementGrowFrames; let go before it is full and it shrinks back at the
// same rate (over as long as it grew). Full, it keeps that size kJudgementFullFrames, then shrinks back over
// kJudgementGrowFrames, and grows again only from a fresh hold of the guard. Its size carries his swings' hits out
// with the hammer's head and widens them, its charge attack's ground impact and that impact's reach with it, and his
// attack by kJudgementGrownAttack at its full size (evenly between). Times are game frames.
inline constexpr float kJudgementGrownScale = 1.5f;
inline constexpr int   kJudgementGrowFrames = 180;
inline constexpr int   kJudgementFullFrames = 900;
inline constexpr float kJudgementGrownAttack = 1.5f;

// Whether Goro is in play with Last Judgement in hand.
bool JudgementHammerWielded();

// The hammer's size now (1 its own), and what it makes of his attack and a swing's hit radius.
float JudgementHammerScale();
int   JudgementHammerAttack(int attack);
float JudgementHammerHitRadius(float radius);

// Hooks: a step (with the monsters'); his charge attack's impact set in slot of effect (GoroKey_Play); a shot effect
// planting its hit (ShotEffectHitSet), the impact's reaching its size times as far.
void JudgementHammerStep();
void JudgementHammerImpact(CSHOT_EFFECT &effect, int slot);
void JudgementHammerHitSet(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit);

void JudgementHammerReset();

} // namespace expanded
