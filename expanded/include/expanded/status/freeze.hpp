#pragma once

struct COLLISION_HIT;

namespace expanded {

// A hit on a frozen player ends the freeze, other ailments kept, and loses its own Freeze.
void BreakFreeze(COLLISION_HIT *hit);

} // namespace expanded
