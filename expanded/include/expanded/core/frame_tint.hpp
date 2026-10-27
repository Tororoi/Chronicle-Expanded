#pragma once

namespace dc::game {
class CFrame;
} // namespace dc::game

namespace expanded {

// Frames drawn with something added to the ambient light for their own draw, as the Expanded monsters light a part
// of themselves (the Bomb Gemron's bomb reddening as its fuse burns). Every unit of a monster species draws the same
// frames in turn, so a unit sets its tint before its own draw. Tints are forgotten as a floor's kinds are set up.

// Adds to a frame's ambient (in the ambient's 0-255 units); 0, 0, 0 stops.
void SetFrameTint(const CFrame *frame, float red, float green, float blue);

// Hooks: CFrame::DrawVu1 around its visual's draw; Begin gives back whether it tinted (saved holds the ambient).
bool FrameTintBegin(const CFrame *frame, float *ambient, float *saved);
void FrameTintEnd(float *ambient, const float *saved);

} // namespace expanded
