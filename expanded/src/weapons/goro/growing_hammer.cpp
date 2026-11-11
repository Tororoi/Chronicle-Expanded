#include "expanded/weapons/goro/growing_hammer.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/blade_scale.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

int                 grown_frames = 0; // of kGrowingHammerGrowFrames
float               scale = 1.0f;
int                 full_frames = 0; // at its full size
bool                spent = false;   // its full size run out: no growing until the guard is let go
bool                shown = false;   // the hammer in his hand drawn at a size not its own
const CSHOT_EFFECT *impact = nullptr;
int                 impact_slot = -1;
float               impact_scale = 1.0f; // the hammer's as it landed

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(GrowingHammerReset), true);

} // namespace

float GrowingHammerFull() {
    if (!Playing(CHARA_GORO) || NowWeapon != &MainWeapon) {
        return 1.0f;
    }
    for (const GrowingHammerTier &tier : kGrowingHammers) {
        if (WeaponWielded(tier.weapon_no)) {
            return tier.full;
        }
    }
    return 1.0f;
}

bool GrowingHammerWielded() {
    return GrowingHammerFull() > 1.0f;
}

float GrowingHammerScale() {
    return GrowingHammerWielded() ? scale : 1.0f;
}

int GrowingHammerAttack(int attack) {
    float times = GrowingHammerScale(); // its size, its strength
    return static_cast<int>(std::lround(static_cast<float>(attack) * times));
}

float GrowingHammerHitRadius(float radius) {
    return radius * GrowingHammerScale();
}

void GrowingHammerStep() {
    if (!GrowingHammerWielded()) {
        // Out of his hand, it is its own size again.
        grown_frames = 0;
        scale = 1.0f;
        full_frames = 0;
        spent = false;
        if (shown) {
            ScaleSwordInHand(1.0f);
            shown = false;
        }
        return;
    }
    bool guarding = Guarding();
    if (!guarding) {
        spent = false;
    }
    if (grown_frames >= kGrowingHammerGrowFrames && full_frames < kGrowingHammerFullFrames) {
        if (++full_frames >= kGrowingHammerFullFrames) {
            spent = true;
        }
    } else if (guarding && !spent) {
        grown_frames = std::min(kGrowingHammerGrowFrames, grown_frames + 1);
        full_frames = 0;
        if (grown_frames == kGrowingHammerGrowFrames) {
            ChargeCompleteFlash(); // full
        }
    } else {
        grown_frames = std::max(0, grown_frames - 1);
        if (grown_frames == 0) {
            full_frames = 0;
        }
    }
    scale = 1.0f + (GrowingHammerFull() - 1.0f) * static_cast<float>(grown_frames) / kGrowingHammerGrowFrames;
    ScaleSwordInHand(scale);
    shown = scale != 1.0f;
}

void GrowingHammerImpact(CSHOT_EFFECT &effect, int slot) {
    if (!GrowingHammerWielded() || slot < 0 || slot >= static_cast<int>(std::size(effect.chara))) {
        return;
    }
    if (!InfernoWielded()) { // the Inferno's own impact is hidden behind its blast, which takes the size instead
        effect.chara[slot].SetScale(scale, scale, scale);
    }
    impact = &effect;
    impact_slot = slot;
    impact_scale = scale;
}

void GrowingHammerHitSet(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit) {
    if (&effect == impact && slot == impact_slot) {
        hit.radius *= impact_scale;
        if (effect.active[slot] == 0) {
            impact = nullptr;
            impact_slot = -1;
        }
    }
}

void GrowingHammerReset() {
    grown_frames = 0;
    scale = 1.0f;
    full_frames = 0;
    spent = false;
    impact = nullptr;
    impact_slot = -1;
}

} // namespace expanded
