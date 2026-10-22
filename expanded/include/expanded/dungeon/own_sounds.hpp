#pragma once

namespace expanded {

// Sounds of banks no dungeon set carries, played on a port of Expanded's own that the game leaves unused, so the
// rest of the game's sounds can be ducked under one: they drop to kDuckLevel as it goes off, hold kDuckHold frames,
// then come back up as it fades out over its frames. Times are game frames.

// A note of a program in the bank named bank inside the pack at path, sounding for frames.
struct OwnSound {
    const char *path;
    const char *bank;
    int         program;
    int         note;
    int         frames;
};

inline constexpr int   kOwnSoundPort = 9;
inline constexpr int   kOwnSoundBanks = 16; // the port's bank and the 15 its effects select with CC 0
inline constexpr float kDuckLevel = 0.4f;
inline constexpr int   kDuckHold = 15;

// Has a sound's bank read as a floor loads (once a session), so playing it reads no disc.
void RegisterOwnSound(const OwnSound &sound);
// Reads, decodes and binds a sound's bank now; false when it is not on the disc.
bool LoadOwnSound(const OwnSound &sound);
// Plays a loaded sound at full volume, centred, ducking the game's other sounds under it with duck; false when its
// bank is not loaded.
bool PlayOwnSound(const OwnSound &sound, bool duck);
// How far the game's other sounds are ducked now: 1 when they are not.
float OwnSoundDuck();

// Hooks: a floor loaded (the registered banks are read); a frame the monsters step (the duck comes back up); the
// floor's monsters put back (the duck let go).
void OwnSoundsFloorLoaded();
void OwnSoundsStep();
void OwnSoundsReset();

} // namespace expanded
