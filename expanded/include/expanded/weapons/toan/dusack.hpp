#pragma once

class CMonstorUnit;

namespace expanded {

// Dusack "No Fool's Gold" (also the Brave Ark's), as the mod has it: a mimic or king mimic can be hit while it wakes. A
// chest mimic's wake is a guard (data/monster_scripts.hpp: its window from kMimicWakeGuardStart to kMimicWakeGuardEnd, a
// king's to kKingMimicWakeGuardEnd), and with the sword in hand that window lets every hit through; the mimic's own guard
// still blocks. Super Steve carrying either sphere has it. The Dusack's chest tier and Swift Strikes live with the Sax and
// the Shamshir.
bool NoFoolsGoldActive();

// Hook (MonsterGuardBlocks): whether monster_no's guard window lets hits through.
bool NoFoolsGoldOpens(const CMonstorUnit &unit, int monster_no, int window);

} // namespace expanded
