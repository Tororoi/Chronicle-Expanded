#pragma once

namespace dc::game {
class CFrame;
class CBoxVu0;
class CCPoly;
} // namespace dc::game

namespace expanded {

// How many monsters take part at once. The game holds every one past the four nearest the player frozen where it
// stands (CMonstorUnit::CheckViewLevel), out of reach of every hit, pull and burn until a nearer one falls; Expanded
// lets every monster close enough to be seen take part (a boss floor's already do). Groups come at the player all at
// once.
inline constexpr int kMonstersInPlay = 16;

// A monster's step lists the polygons it moves against: the floor's near it, then the bodies of the others taking part
// near it, kMonsterPolys at most (the buffer it is given: CMonstorUnit::Step). With every monster in view taking part
// a tight group could carry the list past it, so another's body goes in only as far as there is room. Adds to list
// (count already in it) the polygons of body within box that fit, and returns how many it added.
inline constexpr int kMonsterPolys = 400;

int PickUpBodyPolys(CFrame &body, CCPoly *list, int count, const CBoxVu0 &box);

} // namespace expanded
