#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded::babels_spear {

// What Curse of Babel raises, a character on the floor of its own, the weapon in hand untouched:
//   - kSpear, Ungaga's: Babel's Spear loaded again from its own pack (its mesh and textures its own), turned point-up (its
//     blade's axis, the model's +z, straight up).
//   - kStatue, Super Steve's: Super Steve copied from its pack (its model, motions and skins its own, drawn with the
//     textures the slingshot in Xiao's hand was loaded with), standing fork up (its grip's rotation turned a quarter in
//     pitch and a quarter in yaw, set into the root's children, the root left its scale alone), held on its weapon's taut
//     catch pose: kStatueKey, a key of the copy's own, holding frame kStatuePose.
enum class CopyForm {
    kSpear,
    kStatue
};

inline constexpr int   kStatueKey = 14;
inline constexpr float kStatuePose = 254.0f;

// Loads the form (once a floor) and puts it up; false when it cannot be loaded.
bool CopySpawn(CopyForm form);
void CopyDown();
bool CopyUp();
// Where it stands: its root at root, turned to yaw, scale times its size, tint added to its ambient (each of red, green
// and blue, 0-255), alpha solid (0 clear to 1); the statue's pose stepped.
void CopyPlace(const float *root, float yaw, float scale, float tint, float alpha);
// A point (x, y, z) in the frame of the statue's fork bone (eff30, the fork's centre, where Xiao's pellets leave from) in
// the world as the statue stands, into world (x, y, z); false with no statue up.
bool CopyForkPoint(const float *local, float *world);
void CopyDraw();
// The character up (tests read its place), or null.
CCharacter *CopyCharacter();

// Taken down with nothing loaded (a floor change), for tests too.
void CopyReset();

} // namespace expanded::babels_spear
