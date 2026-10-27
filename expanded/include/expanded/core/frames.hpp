#pragma once

namespace dc::game {
class CFrame;
} // namespace dc::game

namespace expanded {

// The frame named name in root's tree (root included), or null; null for a null root.
CFrame *FrameNamed(CFrame *root, const char *name);

// Where frame's origin is in the world (x, y, z, 1).
void FrameOrigin(CFrame &frame, float *world);

// A weapon raised point-up: its root's rotation (the grip) turned a quarter about x, so the blade's axis, the model's +z,
// stands straight up. The draw keeps a rigid model's root rotation under the character's turn.
void TurnPointUp(CFrame &root);

// A slingshot posed: its grip's rotation (its root's, without its scale) turned pitch quarter-turns about x, then yaw
// quarter-turns about y, set into the root's children (their rotations and places, which are in the root's space). The
// draw sets the root's own rotation from the character's, so the pose has to live below it.
void PoseBelowRoot(CFrame &root, int pitch, int yaw);

} // namespace expanded
