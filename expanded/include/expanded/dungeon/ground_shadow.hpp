#pragma once

namespace expanded {

// A round shadow on the floor for a prop the game casts none for: the shadow disc (data/prop_models.hpp), a character of
// its own on the floor, cast in the units' shadow pass as a character casts its shadow (its frame kGroundShadowLift over
// the floor, the point handed to the draw 12.8 below it, the light straight down), its scale the radius. One at a time;
// cast outside events only, as the monsters' are.
inline constexpr float kGroundShadowLift = 0.3f;

// Cast centred at (x, z) on the floor at height ground, radius across, from the next shadow pass, for owner (the prop, by
// any pointer of its own); no longer cast, if owner cast it.
void GroundShadowShow(float x, float ground, float z, float radius, const void *owner);
void GroundShadowHide(const void *owner);
bool GroundShadowShown();

// Hook: the units' shadow pass (Draw_MainUnitShadow).
void GroundShadowDraw();

// No longer cast, nothing loaded (the floor changed), for tests too.
void GroundShadowReset();

} // namespace expanded
