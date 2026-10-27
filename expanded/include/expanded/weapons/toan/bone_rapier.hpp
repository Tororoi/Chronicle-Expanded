#pragma once

namespace dc::game {
class CMonstorUnit;
struct RS_STACKDATA;
} // namespace dc::game

namespace expanded {

// Toan's Bone Rapier and Xiao's Bone Slingshot.
// - "Skeleton Key": in hand, or as Super Steve's SynthSphere, a bone door (the gate of a Divine Beast Cave floor's
//   locked room, which asks for the Bone Key) opens without its key.
// - "Gravedigger": in hand, or Super Steve carrying a Cross Hinder, Bone Rapier or Bone Slingshot sphere, a monster
//   that would get back up stays down: by its own death script, or by Harder Enemy AI.
bool SkeletonKeyActive();
bool GravediggerActive();

// Hook: _ITEM_USE_WINDOW, its list of the items it takes (-1 at the end): with the Skeleton Key, a window that takes
// the Bone Key alone is answered with it into result, no window shown; true then.
bool SkeletonKeyAnswers(const int *items, RS_STACKDATA *result);

} // namespace expanded
