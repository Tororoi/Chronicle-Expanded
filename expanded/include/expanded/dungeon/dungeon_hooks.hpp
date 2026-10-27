#pragma once

#include <optional>
#include <string>

#include "shot_utils.hpp"

namespace dc::game {
enum class MonsterDamageResult : int;
class CCharacter;
class CMonstorUnit;
class CSHOT;
class CSHOT_EFFECT;
struct COLLISION_HIT;
struct MONSTOR;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// The game's dungeon hooks, each calling the features that act there in turn. A game point that features share goes
// through a hook here; a feature that alone replaces or adjusts one piece of the game may be called from there directly.
//
// A summoned ally (enemies/ally_unit.hpp) is stepped, drawn and hit by the same CMonstorUnit code on a unit of its own,
// so its monster comes through the monster hooks too: they pass it by, but for what the ally does itself.
//
// A floor's state is reset one of two ways: in an enemy floor listener (AddEnemyFloorListener, enemy_events.hpp), as the
// next floor's kinds start being set up, for anything kept per floor; and, for what must also stop as the floor is left
// (effects drawn, sounds playing, lights), in DungeonMonstersCleared too.

// The dungeon's step after the monsters' shot effects stepped (DunMoveChara), unit the floor's.
void DungeonShotsStep(CMonstorUnit &unit);

// The models a floor loaded for Expanded released (a dungeon visit begins, or the next floor's monsters are about to
// load), and a floor's monsters loaded (BtLoadMonstor, after its kinds were set up).
void DungeonFloorUnloaded();
void DungeonFloorLoaded(CMonstorUnit &unit);

// The party's status taken up as a dungeon visit begins (GameInit, after UserStatus->Init).
void DungeonStatusReady();

// The top of a dungeon frame (GameLoop).
void DungeonFrameBegins();

// The dungeon's draw (MainDraw), where features draw over the game's: after the units' shadows (player, the player's
// position), the monsters, the event's characters, the shot effects (before the main effect), the map's raster and the
// player's shots.
void DungeonShadowsDrawn(const float *player);
void DungeonMonstersDrawn();
void DungeonNpcsDrawn();
void DungeonEffectsDrawn();
void DungeonRasterDrawn();
void DungeonShotsDrawn();

// A monster's hit reaching the player (BtCheckDamageProc): whether it passes him by, the hit before its damage is
// worked out, and the reaction it gave (blocked: his guard took it).
bool PlayerHitPassesBy(const COLLISION_HIT &hit);
void PlayerHitReached(COLLISION_HIT &hit);
void PlayerHitReacted(bool blocked);

// The floor's monsters put back (after CleanViewMonstor), as the floor fades out to change or an event takes its memory;
// drop_stars when confusion's stars go with them.
void DungeonMonstersCleared(bool drop_stars);

// Each dungeon step (MoveChara), whether or not the monsters step: what counts in their steps asks MonstersStepping.
void DungeonMonstersTick();

// The floor's monsters stepped (motionDrive, after CMonstorUnit::Step), pause the step's hold.
void DungeonMonstersStepped(int pause);

// A step after the player's pellets have stepped (motionDrive).
void PlayerShotsStepped();

// A kind's model pack as SetupBaseModel loads it: a boss's built pack or an Expanded kind's own, else as it is.
void MonsterPackFile(std::string &filename, int model_no);

// Around a monster's draw (DrawMonstor): its parts lit and moved before, put back after.
void MonsterDrawing(CMonstorUnit &unit, int monster);
void MonsterDrawn(CMonstorUnit &unit, int monster);

// A monster's life just ran out (CheckDmg): whether the kill counts down the floor and up the player's kills.
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

// A monster's defense taken off a hit's damage and the hardness wearing the weapon that hits it (CheckDmg), its own as
// the weapons' effects change them.
int MonsterDefense(const CMonstorUnit &unit, int monster, int defense);
int MonsterHardness(const CMonstorUnit &unit, int monster, int hardness);

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
// Ruby let an orb go into slot of effect (BattleActionShotRuby), its damage and her weapon's abilities on it: scale is
// the throw's charge on her damage (1 a quick shot, more a charged one), repeat for a full charge's second orb, drawn
// shimmering over the first along its path.
void RubyOrbThrown(CSHOT_EFFECT &effect, int slot, float scale, bool repeat);
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

// Whether a monster refuses a hit it would take (CheckDmg): one the hit's source has landed on it already, or one meant
// for another.
bool MonsterHitRefused(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);

// Whether a hit of Xiao's makes the monster flinch (CheckDmg; retail's never do): a blow of one of her weapons' own.
bool PlayerHitStaggers(const COLLISION_HIT &hit);

// A monster took the hit that reached it, past its guard and invincibility (CheckDmg).
void MonsterHitTaken(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);

// A hit just took amount off a monster's life (CheckDmg; its life is past it, under 0 when it took more than it had).
void MonsterDamaged(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);

// A monster's attack sphere just set in the collision table (CheckDmg), hit its record or -1.
void MonsterSwingSet(CMonstorUnit &unit, int monster, int hit);

// Another monster's hit on the current monster's sphere where no hit of the player's landed (CheckDmg): CheckDmg's
// result, or -1 for none.
std::optional<MonsterDamageResult> MonsterFriendlyHit(CMonstorUnit &unit, int sphere);

// Whether a hit of a sword-wielder's on a monster wears the weapon (CheckDmg, landed or guarded): not the hits of the
// effects that wear nothing.
bool HitWearsWeapon(const COLLISION_HIT &hit);

// A monster's step after the other monsters blocked it (CMonstorUnit::Step, after MoveChecMonster), and the player's after
// the monsters blocked it (his move, after CMonstorUnit::MoveCheck), velocity his step: for what else stands in the way.
void MonsterMoveChecked(CMonstorUnit &unit, int monster);
void PlayerMoveChecked(const float *position, float *velocity);

// The units' shadow pass (Draw_MainUnitShadow), before it darkens the screen: for shadows the game casts none for.
void DungeonShadowsDrawing();

// Whether an ailment may take hold of the party (BtSetStatusErr): false when a weapon's effect keeps it off.
bool AilmentAllowed(int ailment);

// Poison took hold of a monster that had none (Poison, from CheckDmg's rolls).
void MonsterPoisoned(const MONSTOR &target);

// Where a monster's script finds the player (_GET_POSITION, _GET_DISTANCE): the first of the features that draw it
// elsewhere, else the player.
void ScriptPlayerPosition(float *position);

// How far from the player a monster's body stops (CMonstorUnit::MoveCheck2), radius the game's.
float MonsterBlockRadius(float radius);

// A shot effect's collision test (CSHOT_EFFECT::Step), checkCollision's result as the features change it.
SHOT_COLLISION_RESULT ShotEffectCollision(CSHOT_EFFECT &effect, int slot, float *hit_position, float *position);

// The point a shot meets as the player (checkCollision), point hers, as the features move it.
void ShotPlayerPoint(float *point);

// The action gauge's refill this step (motionDrive), gain the game's, as the features hold it.
float ActionGaugeGain(float gain);

// The Heal ability's counter, stepped (its tick comes at kHealPeriod), as the weapons' effects hold it; whether the tick
// heals with a weapon whose flags lack the ability; and the tick come, before it heals the active character.
int  HealCounter(int counter);
bool HealsWithoutTheAbility();
void HealTicked();

// The player's motion about to step (motionDrive), chara_no the member: the weapons that change its speed.
void PlayerMotionStepping(CCharacter &chara, int chara_no);

// The player's hits about to be checked (BtCheckDamageProc).
void PlayerHitsChecking();

// The WHP a shot of Xiao's costs, wear an ordinary one's (BattleActionPlay_Jinn), as the weapons' effects change it.
float XiaoShotWear(float wear);

// The percentage of the hit's damage a monster's sphere takes from owner (CheckDmg).
int MonsterSpherePercent(const CMonstorUnit &unit, int monster, int sphere, int owner, const COLLISION_HIT &hit, int percent);

// A landed weapon hit on a monster (CheckDmg): whether it wears the weapon (attacker its owner), and its knockback set,
// for the weapons that throw it further.
bool LandedHitWearsWeapon(int attacker, const COLLISION_HIT &hit);
void MonsterKnockedBack(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit);

// A monster's item stolen by the player's hit (CheckDmg's steal).
void MonsterItemStolen(const CMonstorUnit &unit, int monster, int item);

// A kill's ABS reaching the weapon (CMonstorUnit::Step, after the zone without weapon changes): true when a weapon's
// effect took it in place of the game's grant up to max_abs.
bool KillAbsTaken(WEAPON_HAVE &weapon, int abs, int max_abs);

// Whether a monster's guard windows can block at all: not while it is blinded. Any hit on a monster, another monster's
// too, is held to it.
bool MonsterCanGuard(int monster);

// Whether a monster's guard window blocks the hit that reached it (CheckDmg): MonsterCanGuard, unless the hit is one of
// the weapons that pass a guard.
bool MonsterGuardBlocks(const CMonstorUnit &unit, int monster, int window, const COLLISION_HIT &hit);

} // namespace expanded
