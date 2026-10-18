#pragma once

class CMonstorUnit;

namespace expanded {

// The game's dungeon hooks, each calling the features that act there in turn.

// The dungeon's step, after the monsters' shot effects stepped: Expanded's shots and the Expanded monsters' own.
void DungeonShotsStep(CMonstorUnit &unit);

// A floor's monsters loaded (BtLoadMonstor, after its kinds were set up).
void DungeonFloorLoaded(CMonstorUnit &unit);

// The floor's monsters put back (CleanViewMonstor): confusion ends (its stars dropped when drop_stars) and
// Expanded's shots stop.
void DungeonMonstersCleared(bool drop_stars);

// A step the monsters take unpaused (before motionDrive): the clocks poison and confusion count by.
void DungeonMonstersTick();

// A kind's model pack as SetupBaseModel loads it: a boss's built pack or an Expanded kind's own, else as it is.
void MonsterPackFile(char *filename, int model_no);

// Around a monster's draw (DrawMonstor): its parts lit and moved before, put back after.
void MonsterDrawing(CMonstorUnit &unit, int monster);
void MonsterDrawn(CMonstorUnit &unit, int monster);

// A monster's life just ran out (CheckDmg): whether the kill counts down the floor and up the player's kills.
bool MonsterKilled(const CMonstorUnit &unit, int monster);

// A monster's step (CMonstorUnit::Step): true when a feature stepped it in place of its script.
bool MonsterStepTaken(CMonstorUnit &unit, int monster);

} // namespace expanded
