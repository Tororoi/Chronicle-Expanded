#pragma once

namespace expanded {

// Toan's Bone Rapier and Xiao's Bone Slingshot.
// - "Skeleton Key": in hand, or as Super Steve's SynthSphere, a bone door (the gate of a Divine Beast Cave floor's
//   locked room, which asks for the Bone Key) opens without its key (dungeon/door_keys).
// - "Gravedigger": in hand, or Super Steve carrying a Cross Hinder, Bone Rapier or Bone Slingshot sphere, a monster
//   that would get back up stays down: by its own death script, or by Harder Enemy AI.
bool SkeletonKeyActive();
bool GravediggerActive();

} // namespace expanded
