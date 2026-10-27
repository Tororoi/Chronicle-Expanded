#pragma once

class CMonstorUnit;

namespace expanded {

// A flash's blinding: every ordinary monster on the floor stops, holds its guard (or stands idle, for a kind with none)
// and lowers it on its own clip's clock just before the blinding ends, then takes up its script from the top. A hit
// staggers it with its damage clip and sends it back to the guard. Its guard blocks nothing while blinded. Bosses and
// the monsters still in their chests are left alone. Times are game frames.

inline constexpr int kBlindFrames = 300;       // how long a flash holds the floor
inline constexpr int kBlindStaggerFrames = 20; // a hit's stagger before the guard comes back up
inline constexpr int kBlindIdleBeat = 24;      // the pause between a lowered guard and the script taking over

// The motions a kind holds the blinding with: its guard loop (or its idle), the clip that lowers the guard (-1 for
// none) and its stagger (-1 for none); false for a kind the blinding leaves alone.
struct BlindClips {
    int hold;
    int lower;
    int stagger;
};

bool BlindClipsOf(int model_no, BlindClips *clips);

// Blinds the floor for frames from now; a blinding already running is renewed, and whoever was lowering their
// guard raises it again.
void Blind(CMonstorUnit &unit, int frames = kBlindFrames);
// Ends the blinding at once: every monster's script back from the top.
void BlindEnd();
bool Blinding();
int  BlindFramesLeft();
bool Blinded(int monster);

// Counts the blinding down on a frame the monsters step (MonstersStepping): an event or a hold pauses it.
void BlindTick();

// Hooks: a monster's step in place of its script while blinded; whether its guard windows block a hit.
bool BlindStep(CMonstorUnit &unit, int monster);
bool BlindGuardBlocks(int monster);

} // namespace expanded
