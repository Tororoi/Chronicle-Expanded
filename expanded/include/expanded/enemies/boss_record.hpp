#pragma once

#include "expanded/enemies/monsters.hpp"

namespace dc::game {
class CMonstorUnit;
} // namespace dc::game

namespace expanded {

// The bosses beaten in a save (expanded/docs/bosses.md): those whose life ran out (expanded.json's
// bosses.defeated), and those a save's progress shows beaten, its next dungeon visited. A boss is the monster
// whose fall ends its fight: Dran, Master Utan, the Ice Queen, King's Curse's coffin, Minotaur Joe and the
// Black Knight.
bool BossDefeated(Monster boss);

// Hook: CheckDmg ran a monster's life out.
void BossRecordKill(const CMonstorUnit &unit, int monster);

} // namespace expanded
