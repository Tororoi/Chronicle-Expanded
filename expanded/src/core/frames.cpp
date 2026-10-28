#include "expanded/core/frames.hpp"

#include <algorithm>
#include <cmath>

#include "frame.hpp"

namespace expanded {

CFrame *FrameNamed(CFrame *root, const char *name) {
    return root != nullptr ? root->SearchFrame(const_cast<char *>(name)) : nullptr;
}

void FrameOrigin(CFrame &frame, float *world) {
    float origin[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    frame.GetWorldPosition(world, origin);
}

void TurnPointUp(CFrame &root) {
    float row1[3], row2[3];
    std::copy(root.local[1], root.local[1] + 3, row1);
    std::copy(root.local[2], root.local[2] + 3, row2);
    for (int c = 0; c < 3; c++) {
        root.local[1][c] = -row2[c];
        root.local[2][c] = row1[c];
    }
    root.world_valid = false;
}

void PoseBelowRoot(CFrame &root, int pitch, int yaw) {
    float g[3][3];
    float length = std::sqrt(root.local[0][0] * root.local[0][0] + root.local[0][1] * root.local[0][1] + root.local[0][2] * root.local[0][2]);
    for (int r = 0; r < 3; r++) {
        float p[3] = {root.local[r][0], root.local[r][1], root.local[r][2]};
        for (float &v : p) {
            v = length > 1e-6f ? v / length : v;
        }
        for (int q = pitch; q > 0; q--) {
            float p1 = p[2], p2 = -p[1];
            p[1] = p1, p[2] = p2;
        }
        for (int t = yaw; t > 0; t--) {
            float p0 = p[2], p2 = -p[0];
            p[0] = p0, p[2] = p2;
        }
        std::copy(p, p + 3, g[r]);
    }
    for (CFrame *child = root.child; child != nullptr; child = child->brother) {
        for (int row = 0; row < 4; row++) {
            float a[3] = {child->local[row][0], child->local[row][1], child->local[row][2]};
            for (int c = 0; c < 3; c++) {
                child->local[row][c] = a[0] * g[0][c] + a[1] * g[1][c] + a[2] * g[2][c];
            }
        }
        child->world_valid = false;
    }
}

} // namespace expanded
