#pragma once

class CMonstorUnit;
struct RS_STACKDATA;

namespace expanded {

// Toan's Bone Rapier and Xiao's Bone Slingshot.
// - "Skeleton Key": in hand, or as Super Steve's SynthSphere, a bone door (Divine Beast Cave's back-floor door, which
//   asks for the Bone Key) opens without its key.
// - "Gravedigger": in hand, or Super Steve carrying a Cross Hinder, Bone Rapier or Bone Slingshot sphere, an undead
//   that would get back up stays down.
bool SkeletonKeyActive();
bool GravediggerActive();

// Hook: _ITEM_USE_WINDOW, its list of the items it takes (-1 at the end): with the Skeleton Key, a window that takes
// the Bone Key alone is answered with it into result, no window shown; true then.
bool SkeletonKeyAnswers(const int *items, RS_STACKDATA *result);

} // namespace expanded
