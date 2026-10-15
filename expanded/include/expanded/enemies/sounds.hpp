#pragma once

#include <vector>

class CMonstorUnit;

namespace expanded {

// The dungeon sound sets that carry a kind's sounds: snd100 + the dungeon of each floor laying it out,
// Demon Shaft's five 20-floor bands snd106 to snd110.
std::vector<int> HomeSoundSets(int model_no);

// Hooks: BtLoadMonstor loaded a staged floor's kinds, which brings in the monster banks of their home sets;
// CSound::SE_Play picks the bank a monster port effect plays from (CC 0), or -1 to leave the port alone.
void MonsterSoundsFloorLoaded(const CMonstorUnit &unit);
int  MonsterSoundBank(int port, int program, int note);

} // namespace expanded
