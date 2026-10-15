#pragma once

class CMonstorUnit;
struct COLLISION_HIT;

namespace expanded {

// A hit's throw: the first step's distance, and what each step after loses. It carries about speed^2 / (2 decay) units.
struct KickBack {
    float speed;
    float decay;
};

// About 17 units: a third of Big Bang's blast distance, at its fade.
inline constexpr KickBack kHeavyKickBack = {2.02f, 0.12f};

// Throws monster slot away from origin as a landed hit does, scaled by the monster's own kick-back scale.
void KickMonster(CMonstorUnit &unit, int slot, const float *origin, KickBack kick);

// A landed hit's weapon throw, over the hit's own.
void WeaponKickBack(CMonstorUnit &unit, int slot, const COLLISION_HIT &record);

} // namespace expanded
