#include "expanded/weapons/goro/last_judgement.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/blade_scale.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

int                 grown_frames = 0; // of kJudgementGrowFrames
float               scale = 1.0f;
int                 full_frames = 0; // at its full size
bool                spent = false;   // its full size run out: no growing until the guard is let go
bool                shown = false;   // the hammer in his hand drawn at a size not its own
const CSHOT_EFFECT *impact = nullptr;
int                 impact_slot = -1;
float               impact_scale = 1.0f; // the hammer's as it landed

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(JudgementHammerReset), true);

} // namespace

bool JudgementHammerWielded() {
    return Playing(CHARA_GORO) && NowWeapon == &MainWeapon && WeaponWielded(ITEM_WEAPON_LAST_JUDGEMENT);
}

float JudgementHammerScale() {
    return JudgementHammerWielded() ? scale : 1.0f;
}

int JudgementHammerAttack(int attack) {
    float times = 1.0f + (kJudgementGrownAttack - 1.0f) * (JudgementHammerScale() - 1.0f) / (kJudgementGrownScale - 1.0f);
    return static_cast<int>(std::lround(static_cast<float>(attack) * times));
}

float JudgementHammerHitRadius(float radius) {
    return radius * JudgementHammerScale();
}

void JudgementHammerStep() {
    if (!JudgementHammerWielded()) {
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
    if (grown_frames >= kJudgementGrowFrames && full_frames < kJudgementFullFrames) {
        if (++full_frames >= kJudgementFullFrames) {
            spent = true;
        }
    } else if (guarding && !spent) {
        grown_frames = std::min(kJudgementGrowFrames, grown_frames + 1);
        full_frames = 0;
    } else {
        grown_frames = std::max(0, grown_frames - 1);
        if (grown_frames == 0) {
            full_frames = 0;
        }
    }
    scale = 1.0f + (kJudgementGrownScale - 1.0f) * static_cast<float>(grown_frames) / kJudgementGrowFrames;
    ScaleSwordInHand(scale);
    shown = scale != 1.0f;
}

void JudgementHammerImpact(CSHOT_EFFECT &effect, int slot) {
    if (!JudgementHammerWielded() || slot < 0 || slot >= static_cast<int>(std::size(effect.chara))) {
        return;
    }
    effect.chara[slot].SetScale(scale, scale, scale);
    impact = &effect;
    impact_slot = slot;
    impact_scale = scale;
}

void JudgementHammerHitSet(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit) {
    if (&effect == impact && slot == impact_slot) {
        hit.radius *= impact_scale;
        if (effect.active[slot] == 0) {
            impact = nullptr;
            impact_slot = -1;
        }
    }
}

void JudgementHammerReset() {
    grown_frames = 0;
    scale = 1.0f;
    full_frames = 0;
    spent = false;
    impact = nullptr;
    impact_slot = -1;
}

} // namespace expanded
