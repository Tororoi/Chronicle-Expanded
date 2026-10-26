#pragma once

class CCharacter;

namespace expanded::cactus {

// What Desert Bloom raises: a character on the floor of its own, the weapon loaded again from its own pack (its mesh and
// textures its own, the weapon in hand untouched) and turned point-up (its blade's axis, the model's +z, straight up),
// or a model of Expanded's as it is authored.
struct BloomModel {
    const char *name;
    bool        weapon; // the Cactus, from its pack; else pack and cfg
    const char *pack;
    const char *cfg;
};

// Loads the model (once a floor); false when it cannot be.
bool PropLoad(const BloomModel &model);
// Whether it is up; puts it up (loaded) or takes it down.
bool PropUp();
bool PropSpawn(const BloomModel &model);
void PropDown();
// Where it stands: its root at root, turned to yaw, scale times its size, alpha solid (0 clear to 1).
void PropPlace(const float *root, float yaw, float scale, float alpha);
void PropDraw();
// The character (tests read its place), or null.
CCharacter *PropCharacter();
// Taken down with nothing loaded (a floor change), for tests too.
void PropReset();

} // namespace expanded::cactus
