#pragma once

namespace dc::game {
class CFrame;
} // namespace dc::game

namespace expanded {

// The frame named name in root's tree (root included), or null; null for a null root.
CFrame *FrameNamed(CFrame *root, const char *name);

// Where frame's origin is in the world (x, y, z, 1).
void FrameOrigin(CFrame &frame, float *world);

} // namespace expanded
