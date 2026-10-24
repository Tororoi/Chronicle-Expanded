#pragma once

#include "types.h"

class CDungeonMap;

namespace expanded {

// Atlas the Expanded features place on a floor, each carrying a drop of its own in place of a georama part: a weapon,
// or a weapon's SynthSphere at a sixth of its base values with all its abilities. Toan picks one up as any atla; the
// drop is handed over and named, and nothing is written to the floor's atla records. An atla left on the floor is
// lost with it.

struct AtlaDrop {
    int  weapon = 0;
    bool sphere = false; // the weapon's SynthSphere, else the weapon itself
    // A SynthSphere with these values (and the weapon's level), not a sixth of its weapon's base ones.
    bool own_values = false;
    s16  status[4] = {};
    s8   elem[5] = {};
    s8   vs_monster[10] = {};
    s16  flags = 0;
    s8   level = 0;
    // Told once the drop has been picked up and handed over, with tag.
    void (*taken)(int tag) = nullptr;
    int tag = 0;
};

// An atla's way into place: from a pose (its turn q0, w first, its scale and where its frame stands) to a real
// atla's (q1, scale 1, to), eased, rising 6 at the middle.
struct AtlaPath {
    float q0[4], q1[4];
    float scale;
    float from[3], to[3];
};

// Places an atla carrying drop at pos (its floor point: drawn 8 over it): a new one while the floor has room, else a
// collected one's place. Its index, or -1 for none (no atla model on the floor, or every place still holds one).
int PlaceAtlaDrop(const float *pos, const AtlaDrop &drop);

// With every place holding an atla: the one farthest from the player moved to pos, keeping its own content. Its index,
// or -1.
int TakeOverAtla(const float *pos);

// Moves an atla along path by progress: below 0 out of the draw, then on its way, 1 and over a real atla again. It
// cannot be picked up until then.
void MoveAtla(int atla, const AtlaPath &path, float progress);

// Draws an atla after the shadow pass, as the monsters are drawn: a shadow volume around it (the body of the monster
// it came out of) then darkens the ground under it but not the atla.
void DrawAtlaOverShadows(int atla, bool on);

// Hands a drop over at once, named.
void GiveAtlaDrop(const AtlaDrop &drop);

// Hooks: CDungeonMap::DrawAtraBoll for each atla it draws (true when the atla's draw is taken over), and the dungeon
// draw after the shadow pass (the atlas drawn over the shadows; pos the player's); the atla pickup
// as it takes the atla (true when it carries a drop, handed over then) and as it names it (true when named); and the
// floor's monsters set up (a new floor).
bool AtlaDrawn(CDungeonMap &map, int atla);
void AtlaDropsDrawOverShadows(const float *pos);
bool AtlaDropTaken(int atla);
bool AtlaDropNamed();
void AtlaDropsFloorLoaded();

} // namespace expanded
