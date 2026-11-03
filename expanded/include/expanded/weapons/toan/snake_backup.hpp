#pragma once

// Snake Charmer's backup (serpent_sword.hpp), added to a kind's own script. The Pirate's Chariot's AI loop backs it off
// straight whenever the player gets too close: a routine (its too-close backup) that turns it to the player once and hops
// it straight back, three hops a pass, until it is some way off, with the two it calls (a leg straight back, and a wait
// for the hop's end facing the player). Most kinds have only a dodge to roll for, a hop at a time at random angles, and
// some none, so as a kind's script loads (each model slot, for the floor) a copy of it is made with the Chariot's three
// added past its end, its hop the kind's own backup motion, and its AI loop (label 100) made to call the backup at the
// end of each pass while the player is within a constant (a stub its loop's jump back to the top goes through). The
// constant is 0, so the copy runs as the script does, until Snake Charmer raises it for a monster's step.

#include <cstddef>
#include <span>
#include <vector>

namespace dc::game {
class CMonstorUnit;
}

namespace expanded {

using dc::game::CMonstorUnit;

// The backup added to a program: its routine's record and its trigger's constant cell (both in its code).
struct SnakeBackup {
    int record = 0;
    int trigger = 0;
};

// The program with the backup added (its hop motion), or false when its AI loop has no pass to call it from.
bool AddSnakeBackup(std::span<const unsigned char> program, int motion, std::vector<unsigned char> &out, SnakeBackup &backup);

// Hook (SetupBaseModel): a kind's script loaded into a model slot: the script its monsters run (the copy with the backup
// for a kind that backs off by it, else script).
char *SnakeBackupScript(CMonstorUnit &unit, int slot, int model_no, char *script, int size);

// The backup added to a loaded program (by its code), or null.
const SnakeBackup *SnakeBackupOf(const char *code);

} // namespace expanded
