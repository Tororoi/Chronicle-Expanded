#pragma once

#include "expanded/enemies/enemy_events.hpp"

class CMonstorUnit;

namespace expanded {

// A summoned ally (the Pocklekul's pockle): a real monster of a retail kind, its own script running it, on a
// CMonstorUnit of its own that the dungeon steps, draws and shadows beside the floor's, NowMonstorUnit pointing at it
// meanwhile (every script command reads NowMonstorUnit). It is that unit's monster kAllyMonster and takes none of the
// floor's model or monster slots, so the floor's count, map, lock-on and kill tally never see it, and the floor's enemy
// features (their stats and speed, bosses, minibosses, revivals, ailments, confusion) pass it by (AllyUnit,
// AllyStepping). Its model, script and shots load as a floor's kind does (SetupBaseModel), into memory of its own: its
// images join the monsters' texture block and its shots the floor's shot pack, and whatever its load added to them goes
// back as it goes; its home dungeons' monster sounds are bound beside the floor's (sounds.hpp). One stands at a time.
inline constexpr int kAllyMonster = 0;

// The monster its shots carry (CSHOT_EFFECT::user_id_2): none of the floor's.
inline constexpr int kAllyShooter = kFloorMonsters;

// Whether unit is the ally's, and whether the ally's is the unit stepping or drawing now (NowMonstorUnit).
bool AllyUnit(const CMonstorUnit &unit);
bool AllyStepping();

// The ally's unit while one stands, else null; the floor's unit it stands beside.
CMonstorUnit *AllyStanding();
CMonstorUnit *AllyFloor();

// Loads kind (a MonstorTable kind) into the ally's unit and stands it at position, beside the floor's unit
// (NowMonstorUnit); one standing goes first. False, nothing standing, when it cannot load or stand.
bool AllySummon(int kind, const float *position);

// Takes it down: its monster gone, and what its load added to the texture table, the texture animations and the shot
// pack taken back out wherever it is still its own (a floor's reload or an event may have emptied them first).
void AllyDismiss();

// Forgets it without touching those: the next floor's own may stand where its did.
void AllyForget();

// Hooks: the floor's monsters stepped (motionDrive, after CMonstorUnit::Step; pause the step's hold), drawn
// (DrawMonstor) and their shadows cast (Draw_MainUnitShadow): the ally's with them; the player's move after the
// floor's monsters blocked it, which the ally's blocks too; a shot of the ally's just fired (EnemyShotFired), which
// carries kAllyShooter.
void AllyStep(int pause);
void AllyDraw();
void AllyShadow();
void AllyPlayerMove(const float *position, float *velocity);
void AllyShotFired();

// Tests replace the load: it sets kind up in unit's model slot 0 and stands it at position as its monster kAllyMonster
// (SetupBaseModel's and SetupViewMonstor's parts), false on failure.
using AllyLoader = bool (*)(CMonstorUnit &unit, int kind, const float *position);
void SetAllyLoader(AllyLoader loader);

} // namespace expanded
