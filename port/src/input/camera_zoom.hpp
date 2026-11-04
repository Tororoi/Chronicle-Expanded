#pragma once

namespace dc::game {
class CCameraFollow;
class CEditGround;
class CDungeonMap;
} // namespace dc::game

namespace dc::input {

/**
 * Opens a town zoom scope for one walk update; collision correction still owns the eye.
 */
void TownZoomBegin(game::CCameraFollow *camera, game::CEditGround *ground, int map);

/**
 * Applies pending mouse-wheel zoom to the town camera inside the open scope.
 */
void TownZoomRead(game::CCameraFollow *camera);

/**
 * Closes the town zoom scope and restores the retail near and far camera distances.
 */
void TownZoomEnd();

/**
 * Applies collision-validated mouse zoom to the dungeon gameplay camera, never to scripted cameras. The requested
 * distance is scale times the wheel's (a player drawn larger).
 */
void DungeonZoomApply(game::CCameraFollow *camera, game::CDungeonMap *map, bool lock_on, float previous_distance, float scale = 1.0f);

/**
 * Returns the dungeon eye-lift distance, capped by the requested zoom (scale times the wheel's).
 */
float DungeonZoomNearDistance(game::CCameraFollow *camera, float normal, bool lock_on, float scale = 1.0f);

} // namespace dc::input
