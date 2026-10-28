#pragma once

// Queens' canal tide (what dcdata builds for it: expanded/data/queens_canal.hpp). The canal's water stands at the level
// of the time of day, low in the morning, the game's own afternoon and night, high at dusk: its surface, its rippling
// water body and the fishing water. A change of level shows under the black of the time's change (or of a load). At
// low tide the canal floor is reached by the ladder, whose message point says the tide is too high otherwise; a player
// down there when the tide rises is taken to East Harbor's dock under that black. The low canal has mist where the
// waterfalls meet it, ripples under a player standing in it and round the ladder's rails, and a player wading it drawn
// before the water so the water covers what is under it.

namespace dc::game {
class CEffectGroup;
struct ED_EVENT_POINT;
} // namespace dc::game

namespace expanded {

// The canal's level at the game's clock time (0 to 12).
float CanalTideLevel(float time);

// The level shown in Queens now: the clock's, once a change has had its black.
float CanalTideShown();

// Each frame Queens is drawn (EditMainDraw): the level shown, the water set to it, the ripples placed, and a player
// caught down in the canal by the rising tide taken to the dock.
void QueensCanalStep();

// Hook: CheckEventPoint: false keeps a point out: the ladder's top but at low tide (its foot always, so a player down
// there can climb out), its message point at low tide.
bool CanalEventPointOn(const dc::game::ED_EVENT_POINT &point);

// Hook: EditMainDraw, before the water: a player in the low canal drawn first, so the water hides what is under it and
// the player's own drawing afterwards leaves it hidden.
void DrawWadingPlayer();

// Hook: EditMainDraw's effects, beside Matataki's waterfall spray: the low canal's waterfalls' mist.
void CanalMist(dc::game::CEffectGroup *group);

// Hook: EffectWaterSpray, before the spray enters: its velocity leant as the mist it is part of leans (none but the
// canal's).
void SprayLean(float *velocity);

} // namespace expanded
