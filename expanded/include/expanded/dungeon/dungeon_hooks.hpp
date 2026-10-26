#pragma once

#include "shot_utils.hpp"

class CMonstorUnit;
class CSHOT;
class CSHOT_EFFECT;
struct COLLISION_HIT;

namespace expanded {

// The game's dungeon hooks, each calling the features that act there in turn.

// The dungeon's step, after the monsters' shot effects stepped: Expanded's shots and the Expanded monsters' own.
void DungeonShotsStep(CMonstorUnit &unit);

// A floor's monsters loaded (BtLoadMonstor, after its kinds were set up).
void DungeonFloorLoaded(CMonstorUnit &unit);

// The floor's monsters put back (CleanViewMonstor): confusion ends (its stars dropped when drop_stars), Expanded's
// shots stop and a flash's charge and blinding end.
void DungeonMonstersCleared(bool drop_stars);

// A step the monsters take unpaused (before motionDrive): the clocks poison and confusion count by, the weapons'
// curses, the Solar Flash's charge and blinding, and the Wise Owl Sword's watch for key holders.
void DungeonMonstersTick();

// A step after the player's pellets have stepped (motionDrive), as the mod's pellet catcher runs: the Divine Beast cat.
void PlayerShotsStepped();

// A kind's model pack as SetupBaseModel loads it: a boss's built pack or an Expanded kind's own, else as it is.
void MonsterPackFile(char *filename, int model_no);

// Around a monster's draw (DrawMonstor): its parts lit and moved before, put back after.
void MonsterDrawing(CMonstorUnit &unit, int monster);
void MonsterDrawn(CMonstorUnit &unit, int monster);

// A monster's life just ran out (CheckDmg): the DeSanga's heal, and whether the kill counts down the floor and up the
// player's kills.
bool MonsterKilled(const CMonstorUnit &unit, int monster);

// The item a killed monster drops, before the game's rare-item roll (CMonstorUnit::Step): drop_item, its set drop or -1,
// as the weapons' effects change it.
int MonsterDropItem(const CMonstorUnit &unit, int monster, int drop_item);

// The ABS a kill gives the active character's weapon (Step's grant, before the back floor's doubling): abs, the
// monster's own, as the weapons' effects change it.
int MonsterKillAbs(const CMonstorUnit &unit, int monster, int abs);

// A weapon hit's damage on a monster (CheckDmg, after the weapon's anti-monster bonus), as the weapons' effects
// change it.
float MonsterWeaponDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage);

// A character's defense against a monster's hit (BtCheckDamageProc) and in the dungeon menu's status, defense their
// own, as the weapons' effects change it.
int PlayerDefense(int chara, int defense);

// The active character's lock-on range scale (SetNearLockOnTarget, setTargetCursor), as the weapons' effects change it.
float PlayerLockOnRange(float rate);

// A monster's step (CMonstorUnit::Step): true when a feature stepped it in place of its script.
bool MonsterStepTaken(CMonstorUnit &unit, int monster);

// Xiao fired a pellet into slot of shots (BattleActionPlay_Jinn), and a pellet ended at point, on a monster when struck
// (CSHOT::step): the weapons that ride or answer her shots.
void PelletFired(CSHOT &shots, int slot);
void PelletEnded(CSHOT &shots, int slot, const float *point, bool struck);
// A pellet's hit just planted (CSHOT::step).
void PelletHitSet(const CSHOT &shots, int slot, COLLISION_HIT &hit);
// A shot effect's hit just planted (CSHOT_EFFECT::Step).
void ShotEffectHitSet(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit);
// A pellet about to be stepped (CSHOT::step).
void PelletStepping(CSHOT &shots, int slot);
// Whether a pellet ending lands its own hit: not one whose hit is a bolt or a bomb's blast.
bool PelletHurts(const CSHOT &shots, int slot);

// Whether the lock-on target's name plate is left off (setTargetCursor): something hangs over the target.
bool TargetNameHidden();

// Whether a monster refuses a hit it would take (CheckDmg): one the hit's source has landed on it already.
bool MonsterHitRefused(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);

// Whether a hit of Xiao's makes the monster flinch (CheckDmg; retail's never do): a blow of one of her weapons' own.
bool PlayerHitStaggers(const COLLISION_HIT &hit);

// A monster took the hit that reached it, past its guard and invincibility (CheckDmg).
void MonsterHitTaken(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);

// Where a monster's script finds the player (_GET_POSITION, _GET_DISTANCE): the Angel Gear's shield ring, else
// confusion's target, else the player.
void ScriptPlayerPosition(float *position);

// How far from the player a monster's body stops (CMonstorUnit::MoveCheck2), radius the game's.
float MonsterBlockRadius(float radius);

// A shot effect's collision test (CSHOT_EFFECT::Step), as checkCollision's: the Angel Gear's returned shots meet walls
// alone.
SHOT_COLLISION_RESULT ShotEffectCollision(CSHOT_EFFECT &effect, int slot, float *hit_position, float *position);

// The point a shot meets as the player (checkCollision), point hers: the Angel Gear's pouch while its shield is solid.
void ShotPlayerPoint(float *point);

// The action gauge's refill this step (motionDrive), gain the game's: held as the Angel Gear's shield's health.
float ActionGaugeGain(float gain);

// The Heal ability's tick came, before it heals the active character: the Angel Gear's party heal.
void HealTicked();

// The player's hits about to be checked (BtCheckDamageProc): the swings on the Angel Gear's shield spent first.
void PlayerHitsChecking();

// The WHP a shot of Xiao's costs, wear an ordinary one's (BattleActionPlay_Jinn): the Steel Slingshot's, a charged
// shot's.
float XiaoShotWear(float wear);

// The percentage of the hit's damage a monster's sphere takes from owner (CheckDmg).
int MonsterSpherePercent(const CMonstorUnit &unit, int monster, int sphere, int owner, const COLLISION_HIT &hit, int percent);

// Whether a monster's guard window blocks the hit that reached it (CheckDmg).
bool MonsterGuardBlocks(const CMonstorUnit &unit, int monster, int window, const COLLISION_HIT &hit);

} // namespace expanded
