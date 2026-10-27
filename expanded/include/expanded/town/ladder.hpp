#pragma once

#include <vector>

namespace dc::game {
struct ED_EVENT_PARAM;
} // namespace dc::game

namespace expanded {

// One frame of an ally's ladder sequence: where the player stands and faces, and the motion it starts.
struct TownLadderFrame {
    float x, y, z;
    float yaw;
    int   motion = -1; // the motion set this frame, or -1 to keep the one playing
    int   flags = 0;   // its playback flags: 2 holds the last frame
    float speed = -1;  // its speed, or -1 for the KEY's
    int   sound = -1;  // sound played this frame, or -1
};

// An ally's ladder sequence.
struct TownLadder {
    std::vector<TownLadderFrame> frames;
    bool                         camera = false; // the camera follows at the ladder's angle, as Toan's climb has it
    float                        camera_yaw = 0; // that angle
    float                        release = 0;    // the camera's angle at the end, from the player's facing
};

// The ladder sequence of party member ally, standing at x, y, z facing yaw, at the ladder end param; no
// frames for a party member who has none (Toan, or a ladder that can't be read).
TownLadder TownLadderSequence(int ally, const float position[3], float yaw, const ED_EVENT_PARAM &param);

// The ladder press at param: false lets Toan climb; true for an ally, who plays its sequence or is refused.
bool TownAllyTakesLadder(const ED_EVENT_PARAM *param);

// Each step of EdMoveChara: plays a frame of the ladder sequence; true while one holds the player.
bool TownAllyLadderStep();

// Stops the ladder sequence.
void TownAllyLadderCancel();

} // namespace expanded
