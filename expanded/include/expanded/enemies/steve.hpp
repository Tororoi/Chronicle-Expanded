#pragma once

#include <string>

class ClsMes;
class CMonstorUnit;

namespace expanded {

// Steve's lock-on lines about a monster: messages 4000 + its name number × 10, ten of them.
inline constexpr int kSteveLines = 4000;

// A dungeon sound set's Steve file in a language: steve01 to steve06, Demon Shaft's bands steve07_1 to _5.
std::string SteveFile(int sound_set, int language);

// The text of message id in a .mes bank, or nullptr where it has none or an empty placeholder.
const short *SteveMessage(const short *bank, int id);

// Hooks: BtLoadMonstor loaded a staged floor's kinds, which brings in their home dungeons' Steve files; and
// ClsMes::GetTextLineDataTop asks for a line, given from those files when Steve's own file lacks it.
void   SteveLinesFloorLoaded(const CMonstorUnit &unit);
short *SteveLine(const ClsMes &mes, int id);

} // namespace expanded
