#pragma once

class CMonstorUnit;

namespace expanded {

// A boss or boss part (MONSTER_KIND_NO_LOCK_ON) a staged roster put on the floor: it stays where the floor
// placed it, raises no event, and once beaten collapses and fades out like any monster.
bool BossOutOfPlace(int monster);

// The motion a boss collapses with once beaten, or -1 to fade as it stands.
int BossCollapseMotion(int model_no);

// Steps a beaten boss's death script runs for before its collapse takes over, and steps it fades over.
inline constexpr int kBossScriptFrames = 240;
inline constexpr int kBossFadeFrames = 96;

// Hooks: _SET_POSITION and _SET_MONSTOR_POS from a boss setting up or beaten (true keeps the monsters where
// they are), _RUN_SCRIPT and _STATUS_SET_EVENT (true drops the event), and Step before a monster's script
// runs (true steps the collapse instead).
bool BossKeepsPlace(const CMonstorUnit &unit, int monster);
bool BossEventHeld(int monster);
bool BossDeathStep(CMonstorUnit &unit, int monster);

} // namespace expanded
