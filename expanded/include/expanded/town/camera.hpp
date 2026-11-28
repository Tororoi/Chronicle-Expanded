#pragma once

// The town's walking camera, with the Modern Camera option on: it collides with what the town draws, not with the
// coarse camera meshes the original game gives each part. Each frame, just before the view is built from it, the line
// from the player's head to the eye the camera has eased to is checked: clear, the eye only keeps a sphere big enough to
// hold the near plane out of what is near it; cut, the eye moves in at once to stop short of the cut, and eases back
// out as it clears. The eye is moved for the frame drawn only: the game's own eye is put back afterwards. Only blocking things stop it (walls, buildings, rocks, the ground): pass-through ones (posts, pillars and what
// they hold up just over them, signs, lamps, low fences and rims) it passes, each told apart as a connected piece of
// its mesh by its size. A mesh whose pass-through pieces the eye is in is drawn without its back faces then, so the
// eye sees out through it rather than its inside. Water, glows
// and the like that do not write depth are left out. The original game's wall rules (its turns away from walls,
// pull-ins and lifts) and the mouse's wall clamps stand aside while it runs.

namespace dc::game {
class CCamera;
class CEditGround;
} // namespace dc::game

namespace expanded {

// Whether the town camera's collision is Expanded's, walking and fishing alike: the option on. EdMoveChara skips its
// own wall rules then, and the mouse its clamps.
bool TownCameraOn();

// Hook: EditMainDraw, before the view is built from the camera shown: the walking or fishing camera's eye kept out of
// the town (fishing, the line to the eye runs from the float it watches).
void TownCameraSolve(dc::game::CCamera *shown, dc::game::CEditGround *ground, int mode);

// Hook: EditLoop, once the frame is drawn: the game's own eye put back where the solve moved it, so the game eases it
// on from there and the solve starts again from where the game aims, not from where it showed the eye.
void TownCameraRestore();

// Hook: EdMoveChara, where ground asking for a high camera lifts its height to `lowest`: the height to set this frame,
// eased up with the option on, at once as in the original game off.
float TownCameraHighRise(float height, float lowest);

// Leaves the town: its triangles let go.
void TownCameraReset();

} // namespace expanded
