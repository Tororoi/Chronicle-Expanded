#include "expanded/core/motion_keys.hpp"

#include "gameutil.hpp"

namespace expanded {

bool MotionTrackWaits(int key, int track_type) {
    return key < 0 && track_type != MOTION_KEY_VERTEX;
}

} // namespace expanded
