#pragma once

class CMonstorUnit;

namespace expanded {

// Dusack "No Fool's Gold" (also the Brave Ark's): a mimic or king mimic can be hit while it wakes, through the long
// invincibility its script gives it as the box opens. The short invincibility after a hit still holds, and so does its
// guard. Super Steve carrying either sphere has it. The Dusack's chest tier and Swift Strikes live with the Sax and the
// Shamshir.
//
// Invincibility of at least kWakeInvincibleSteps is a wake (retail mimics: 100 on waking, 8 after a hit).
inline constexpr int kWakeInvincibleSteps = 30;

bool NoFoolsGoldActive();

// Hook: _STATUS_SET_MUTEKI: monster_no's script gave it steps of invincibility.
void NoFoolsGoldInvincible(int monster_no, int steps);

// Hook: CMonstorUnit::CheckDmg's invincibility test: whether monster_no takes hits through its invincibility.
bool NoFoolsGold(const CMonstorUnit &unit, int monster_no);

} // namespace expanded
