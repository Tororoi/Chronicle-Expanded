#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded::divine_beast_title {

// The monsters the Spirit Beast goes for, on the floor's unit (NowMonstorUnit).

// A monster the cat can go for: in a slot of the floor, living.
bool Targetable(int monster);

// A mimic not yet out of its chest and in play.
bool Dormant(int monster);

// Whether the cat's hit cannot land on it now: a mimic in its chest, or a monster in its untouchable frames.
bool Untouchable(int monster);

// The monster's body (its root character).
const CCharacter &Body(int monster);

// The monster Xiao has locked on to, or would lock on to, if targetable; else the targetable one nearest her within
// kSpiritBeastSight; or -1.
int LockOnCandidate();
int PickTarget();

// Where the cat walks and leaps to: the centre of the monster's largest hurt sphere the cat's hit damages (else its largest
// in use), the furthest forward of equals; with none, or a monster not yet in play (a mimic in its chest), its feet.
void AimPoint(int monster, float *out);

// Whether the monster gets the higher arc: a flyer, hoverer or tall body, a miniboss, or one drawn large.
bool TallLeap(int monster);

} // namespace expanded::divine_beast_title
