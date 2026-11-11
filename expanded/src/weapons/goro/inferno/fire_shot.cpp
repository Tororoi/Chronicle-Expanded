#include "expanded/weapons/goro/inferno/fire_shot.hpp"
#include "sound_effect.h"

#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "expanded/weapons/goro/inferno/smash_blast.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_shot.hpp"
#include "itemdata.hpp"
#include "motionmodel.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "weaponelement.hpp"

namespace expanded {

namespace {

// Whether her charged pellets are a fireball: either sphere's.
bool Burning() {
    return InfernoSphere() || (UnderworldShotActive() && SatansAxFlame());
}

} // namespace

bool FireShotsFlying() {
    bool any = false;
    if (Burning()) {
        UnderworldShotsEach([&](const CSHOT &, int) { any = true; });
    }
    return any;
}

bool FireShotHidden(const CSHOT &shots, int slot) {
    return Burning() && UnderworldShotPierces(shots, slot);
}

void FireShotHit(const CSHOT &shots, int slot, COLLISION_HIT &hit) {
    if (InfernoSphere() && FireShotHidden(shots, slot)) {
        hit.flags = GetWeaponElementAttr(WEAPON_ELEMENT_FIRE);
    }
}

void FireShotsDraw() {
    if (!Burning()) {
        return;
    }
    const float scale = InfernoSphere() ? kFireShotScale : kSatansShotScale;
    UnderworldShotsEach([scale](const CSHOT &shots, int slot) {
        const float *way = shots.vector[slot];
        // Its flight is along its z: turned down (about x) by the climb, then about y onto the way it flies.
        float yaw = std::atan2(way[0], way[2]), pitch = std::atan2(-way[1], std::hypot(way[0], way[2]));
        RisingFlameDrawAt(shots.pos[slot], pitch, yaw, scale);
    });
}

void FireShotEnded(const CSHOT &shots, int slot, const float *point) {
    if (!FireShotHidden(shots, slot)) {
        return;
    }
    // The blast stands on the floor beneath it (hers), its hit where it ended.
    const bool  inferno = InfernoSphere();
    const float floor[3] = {point[0], CharaMain.pos[1], point[2]};
    SmashBlastAt(floor, inferno ? BlastLook::kOrange : BlastLook::kPink);
    SndSePlay(SE_EXPLOSION, -1, 0);
    if (NowMonstorUnit == nullptr) {
        return;
    }
    BlastOptions options;
    options.reach = (inferno ? kFireShotBlast : kSatansShotBlast) / kBlastFalloff[0].reach;
    options.flat = true; // its full damage as far as it reaches
    options.kick = kFireShotKick;
    options.element = inferno ? WEAPON_ELEMENT_FIRE : NowWeaponHave->best_elem; // Satan's Ax's: the one selected
    PlantBlast(*NowMonstorUnit, point, shots.damage[slot], options);
}

} // namespace expanded
