#pragma once

namespace dc::game {
class CCharacter;
} // namespace dc::game

namespace expanded::angel_gear {

// The Guardian Reflector's giant slingshot: the weapon in Xiao's hand loaded again from
// its own character pack (commenu/weapon/c04wNN.chr, Super Steve's own model for its sphere) as a floor character, so its
// mesh, motion and keys are its own and the slingshot in her hand is never touched. It stands kPropAhead units out from her
// along its orbit bearing and kPropUp above her feet, kPropScale times the size, facing along the bearing, upright as the
// mod tuned it (kPropPreset: the grip's rotation turned three quarter-turns in pitch and one in yaw, set into the root's
// children once loaded, since the draw sets the root's own rotation from the character's). It plays the weapon's own draw,
// hold and shoot keys; its pouch travels kPropPull weapon units (the authored draw is about 4.2), the draw re-pathed to run
// from the taut catch pose with a cubic ease-out, and it idles on that catch pose (key kPropKeyIdle). Its orbit eases toward
// the wanted bearing every frame. Distances are units, in the world unless named the weapon's.
inline constexpr float kPropScale = 2.0f;
inline constexpr float kPropUp = 7.5f;
inline constexpr float kPropAhead = 12.0f;
inline constexpr float kPropPull = 2.5f;
inline constexpr int   kPropPreset = 7;
inline constexpr int   kPropKeyDraw = 11, kPropKeyHold = 12, kPropKeyShoot = 13, kPropKeyIdle = 14;
inline constexpr float kPropIdleFrame = 254.0f; // the end of the snap: the pouch forward and taut
inline constexpr float kPropShootRate = 0.3f;   // the release's rate (the authored 0.7 read too snappy at this size)
inline constexpr float kPropOrbitEase = 0.18f, kPropOrbitRate = 0.20f;

// Loads the copy of the weapon in Xiao's hand (once a floor, ahead of her guard); false when it cannot be.
bool PropLoad();
// Whether the copy is up.
bool PropUp();
// Puts the copy up, at the wanted bearing; false when it cannot be loaded.
bool PropSpawn();
// Takes it down.
void PropDown();
// Whether the weapon in her hand is not the one the copy was made from (it must come down).
bool PropStale();
// Plays one of the weapon's keys on the copy, from its first frame.
void PropKey(int key);
int  PropKeyNow();
// The bearing it should face, relative to her facing (radians; 0 straight ahead), and the one it faces now.
void  PropOrbitTarget(float bearing);
float PropOrbit();
// A frame: the orbit eased toward its target and the copy placed about her (before its points are read); then, at alpha (0
// clear to 1 solid), its motion stepped.
void PropPlace();
void PropAnimate(float alpha);
void PropDraw();
// Its grip (the root), its pouch and its muzzle (eff30, where her own pellets leave from), in the world as it stands; with
// the copy down, the grip's place.
void PropRoot(float *out);
void PropPouch(float *out);
// The pouch's place at the catch pose (the point a shot meets while the shield is solid), whatever its motion.
void PropCatchPoint(float *out);
void PropMuzzle(float *out);
// The copy itself (tests read its keys and pose), or null.
CCharacter *PropCharacter();

// Taken down with nothing loaded (a floor change), for tests too.
void PropReset();

} // namespace expanded::angel_gear
