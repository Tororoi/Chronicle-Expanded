#pragma once

namespace dc::game {
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// The magic circles' strength: retail's (1), or amplified (the Crysknife and Magical Hammer's "Circle Amplifier":
// 2 while one of the two is owned, 3 while both are). An amplified circle multiplies what it does by it:
// - attack up lasts 30 s times it, and every monster's rage 5 s times it;
// - money up gives it times retail's gain, money down takes 20 % times it;
// - each WHP and stat roll is multiplied by it, elements' and antis' losses too;
// - the WHP loss takes WHP to 1;
// - ABS full and WHP cure reach every party member's equipped weapon at 2, every weapon the party carries at 3.
// ABS full never lowers a weapon's ABS, so one overtrained past its max (the Macho Sword) keeps it, whatever the
// strength.
int CircleMagnitude();

// The Secret Armlet's "Favoured Circles": while it is owned (in hand, a bag or storage; a sphere doesn't pass it on),
// every bad circle is dealt as a good one, at the circles' strength:
// - money down as money up, the maximum WHP loss as its gain and the WHP loss as the WHP cure, each with that circle's
//   message;
// - the stat loss as a gain of the same rolls: the stat and one element and anti of each group go up, to 99 at most
//   (one past 99 already stays as it is);
// - the monsters' rage as every monster slowed (gooed) for kCircleSlowFrames times the strength, with its own
//   message. A monster gooed already keeps its goo, which never wears off.
bool CirclesFavoured();

// Monster steps the favoured rage circle slows the monsters for at retail strength: 5 s.
inline constexpr int kCircleSlowFrames = 300;

// The system message a circle shows is this far past what Run_TrapCircle gives, the circle's kind.
inline constexpr int kCircleMessages = 0x12C;

// Hook: Run_TrapCircle, with the circle's kind (a default weapon's stamina already in): the kind it is dealt as, a
// favoured money down, maximum WHP loss or WHP loss's good counterpart.
int DealtCircle(int kind);

// Hook: Run_TrapCircle, with the kind dealt: runs an amplified or favoured circle, or any ABS full, on the active
// character's weapon, its sound too. Gives what Run_TrapCircle returns (its message less kCircleMessages), or -1 to
// leave the circle to retail.
int ExpandedCircle(int kind, WEAPON_HAVE &weapon);

// One frame: the favoured rage circle's slow counted down on a frame the monsters step (MonstersStepping), and ended
// on every monster still slowed by it.
void CircleSlowStep();

} // namespace expanded
