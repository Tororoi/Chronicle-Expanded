#include "expanded/core/frames.hpp"

#include "frame.hpp"

namespace expanded {

CFrame *FrameNamed(CFrame *root, const char *name) {
    return root != nullptr ? root->SearchFrame(const_cast<char *>(name)) : nullptr;
}

void FrameOrigin(CFrame &frame, float *world) {
    float origin[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    frame.GetWorldPosition(world, origin);
}

} // namespace expanded
