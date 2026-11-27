#pragma once

// The town's walking camera, with the Modern Camera option on: it collides with what the town draws, not with the
// coarse camera meshes the original game gives each part. Each frame, just before the view is built from it, the eye
// the camera has eased to is swept as a sphere (big enough to hold the near plane) out from the player's head, and
// stopped short of the first thing it would touch; the camera's own easing then takes it back out once the way is
// clear. Only large things stop it (walls, buildings, rocks, the ground): the small ones (posts and pillars, signs,
// lamps, low fences and rims) it passes, each told apart as a connected piece of its mesh by its size. Water, glows
// and the like that do not write depth are left out. The original game's wall rules (its turns away from walls,
// pull-ins and lifts) and the mouse's wall clamps stand aside while it runs.

namespace dc::game {
class CCamera;
class CEditGround;
} // namespace dc::game

namespace expanded {

// Whether the walking camera's collision is Expanded's: the option on and not fishing (whose camera keeps the
// original game's). EdMoveChara skips its own wall rules then, and the mouse its clamps.
bool TownCameraOn(bool fishing);

// Hook: EditMainDraw, before the view is built from the camera shown: the walking camera's eye kept out of the town.
void TownCameraSolve(dc::game::CCamera *shown, dc::game::CEditGround *ground, int mode);

// Leaves the town: its triangles let go.
void TownCameraReset();

} // namespace expanded
