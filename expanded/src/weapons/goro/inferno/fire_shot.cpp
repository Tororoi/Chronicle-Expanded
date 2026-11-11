#include "expanded/weapons/goro/inferno/fire_shot.hpp"
#include "sound_effect.h"

#include <cmath>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "expanded/weapons/goro/inferno/smash_blast.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_shot.hpp"
#include "shot_effect.hpp"
#include "snd.hpp"
#include "weaponelement.hpp"

namespace expanded {

bool FireShotsFlying() {
    bool any = false;
    if (InfernoSphere()) {
        UnderworldShotsEach([&](const CSHOT &, int) { any = true; });
    }
    return any;
}

bool FireShotHidden(const CSHOT &shots, int slot) {
    return InfernoSphere() && UnderworldShotPierces(shots, slot);
}

void FireShotsDraw() {
    if (!InfernoSphere()) {
        return;
    }
    UnderworldShotsEach([](const CSHOT &shots, int slot) {
        const float *way = shots.vector[slot];
        // Its flight is along its z: turned down (about x) by the climb, then about y onto the way it flies.
        float yaw = std::atan2(way[0], way[2]), pitch = std::atan2(-way[1], std::hypot(way[0], way[2]));
        RisingFlameDrawAt(shots.pos[slot], pitch, yaw, kFireShotScale);
    });
}

void FireShotEnded(const CSHOT &shots, int slot, const float *point) {
    if (!FireShotHidden(shots, slot)) {
        return;
    }
    // The blast stands on the floor beneath it (hers), its hit where it ended.
    const float floor[3] = {point[0], CharaMain.pos[1], point[2]};
    SmashBlastAt(floor);
    SndSePlay(SE_EXPLOSION, -1, 0);
    if (NowMonstorUnit == nullptr) {
        return;
    }
    BlastOptions options;
    options.reach = kFireShotBlast / kBlastFalloff[0].reach;
    options.flat = true; // its full damage as far as it reaches
    options.kick = kFireShotKick;
    options.element = WEAPON_ELEMENT_FIRE;
    PlantBlast(*NowMonstorUnit, point, shots.damage[slot], options);
}

} // namespace expanded
