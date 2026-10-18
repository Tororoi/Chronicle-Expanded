#pragma once

namespace expanded {

// Whether a motion track waits for its first key (MotionProc found none at or before the frame): it leaves its frame
// as it is until then, where retail reads the track's header as a key. Vertex tracks play as retail's.
bool MotionTrackWaits(int key, int track_type);

} // namespace expanded
