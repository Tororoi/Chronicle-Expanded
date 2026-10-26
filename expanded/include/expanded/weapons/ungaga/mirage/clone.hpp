#pragma once

class CCharacter;

namespace expanded::mirage {

// The Mirage's copy of its wielder, as the mod's CharacterClone makes it: a second instance of the active character, its
// dungeon model loaded again from its own pack as a floor character (EnterFloorCopy), so its bones, skins, motion and cloth
// are its own while it draws with the character's own textures, and the weapon in hand copied (its frames, sharing its
// meshes and textures) into the copy's hand. It stands where it is put, facing the way it is put, holding the guard loop
// (kMirageGuardLoop) with its cloth swaying, in the character's own colours under the room's light, as solid as it is told
// (0 clear to 1 solid); it casts no shadow and is drawn with the dungeon's characters. Paused, it is drawn standing still.

// Loads the copy of the active character (once a floor, ahead of a plant); false when it cannot be.
bool CloneLoad();
// Puts the copy up at at, facing yaw: the guard loop from its first frame, the cloth hung at rest from the posed bones, the
// weapon in hand copied; false when it cannot be loaded.
bool CloneSpawn(const float *at, float yaw);
// Takes it down.
void CloneDown();
bool CloneUp();
// A frame unpaused: the copy kept where it was put, its motion and cloth stepped; drawn at alpha from now on.
void CloneStep(float alpha);
// The draw, with the dungeon's characters: nothing while it is clear, or the party member it was made from is not out.
void CloneDraw();
// The copy's root in the world, into out; false with the copy down.
bool CloneRoot(float *out);
// The copy (tests read its pose), and the weapon it holds, or null.
CCharacter *CloneCharacter();
CCharacter *CloneWeapon();

// Taken down with nothing loaded (a floor change), for tests too.
void CloneReset();

} // namespace expanded::mirage
