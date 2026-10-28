#pragma once

namespace expanded {

// Sounds of banks no dungeon set carries, played on a port of Expanded's own that the game leaves unused, so the
// rest of the game's sounds can be ducked under one: they drop to kDuckLevel as it goes off, hold kDuckHold frames,
// then come back up as it fades out over its frames. Times are game frames.

// A stretch of a note's sound: [from, to) seconds into it, faded out over its last fade seconds.
struct OwnCut {
    int   note;
    float from;
    float to;
    float fade;
};

// A note of a program in the bank named bank inside the pack at path, sounding for frames at volume (of 127); with
// variants, one of that many notes from it picked at random each time (as the game picks among the storm's thunders).
// With cuts, it is one of those stretches of the program's notes, at one of variants pitches a semitone apart about it.
struct OwnSound {
    const char   *path;
    const char   *bank;
    int           program;
    int           note;
    int           frames;
    int           variants = 1;
    const OwnCut *cuts = nullptr;
    int           cut_count = 0;
    int           volume = 0x7F;
};

inline constexpr int   kOwnSoundPort = 9;
inline constexpr int   kOwnSoundBanks = 16; // the port's bank and the 15 its effects select with CC 0
inline constexpr float kDuckLevel = 0.4f;
inline constexpr int   kDuckHold = 15;

// Has a sound's bank read as a floor loads (once a session), so playing it reads no disc.
void RegisterOwnSound(const OwnSound &sound);
// Reads, decodes and binds a sound's bank now; false when it is not on the disc.
bool LoadOwnSound(const OwnSound &sound);
// Plays a loaded sound at its volume, centred, ducking the game's other sounds under it with duck (of a sound with
// cuts, the cut at that index, or one at random); false when its bank is not loaded.
bool PlayOwnSound(const OwnSound &sound, bool duck, int cut = -1);
// Lets go a loaded sound's note (one of a single note, no cuts), as CSound::SE_Stop lets go an effect: for a held one.
void StopOwnSound(const OwnSound &sound);
// How far the game's other sounds are ducked now: 1 when they are not.
float OwnSoundDuck();

// Hooks: a floor loaded (the registered banks are read); a frame the monsters step (the duck comes back up); the
// floor's monsters put back (the duck let go).
void OwnSoundsFloorLoaded();
void OwnSoundsStep();
void OwnSoundsReset();

} // namespace expanded
