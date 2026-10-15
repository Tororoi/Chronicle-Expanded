#pragma once

#include "dataalloc_fwd.hpp"

class CMonstorUnit;
class CSHOT_EFFECT;
struct BT_SHOT_EFFECT;

namespace expanded {

// The shot pack: retail's five CSHOT_EFFECTs (indices 0-4), then Expanded's, which never move and
// grow without a cap. Expanded's take their own memory and texture block, and stay loaded until the
// floor ends.
inline constexpr int kRetailShotEffects = 5;

// Texture block Expanded's shot effects are entered into.
inline constexpr int kShotEffectBlock = 0x34;

// Free texture table entries kept back for what the dungeon enters after a load (menus, item models).
inline constexpr int kTextureReserve = 48;

// Pack indices in use: retail's five, then Expanded's loaded ones.
int ShotEffectCount();

// The effect at a pack index; an empty effect past the loaded ones.
CSHOT_EFFECT *ShotEffectAt(int index);

// Pack index of a loaded description, or -1. Never loads: what fires a shot calls this.
int ShotEffectIndex(const BT_SHOT_EFFECT *effect);

// Loads a description into Expanded's part of the pack with room for shots at once, from path
// (dun/effect/<model_name>.chr when null); returns its pack index (an existing one when loaded), or -1.
int EnterShotEffect(BT_SHOT_EFFECT *effect, int shots, const char *path = nullptr);

// EnterShotEffect for BtEntryEffectTbl[config].
int EnterTableShotEffect(int config, int shots);

// A monster kind's shot effect (which: 0 or 1), with SetupBaseModel's shot count; its pack index, or -1.
int EnterMonsterShotEffect(int model_no, int which);

// Loads a monster kind's shot effects into a unit's model slot, which every later spawn of it copies;
// for kinds set up mid-floor (the randomizer, Pocklekul's pockle, the Gemrons). Null leaves one as it is.
void EnterModelShotEffects(CMonstorUnit &unit, int model_slot, BT_SHOT_EFFECT *first, BT_SHOT_EFFECT *second, int shots);

// The form of a weapon a registered effect is fired in.
enum class WeaponShotForm {
    Weapon, // the weapon itself, carried by anyone in the party
    Sphere, // a synthesis sphere made from it, attached to Super Steve
};

// Declares a shot effect an Expanded weapon feature fires; it is loaded whenever the weapon (or its
// sphere on Super Steve) is with the party on a floor.
void RegisterWeaponShotEffect(int weapon_no, WeaponShotForm form, BT_SHOT_EFFECT *effect, int shots, const char *path = nullptr);

// Loads the registered effects of every weapon in the party's hands and bags and of Super Steve's
// synthesis spheres; once the floor is loaded, after a weapon is gained or built up or a sphere moves.
void ShotEffectsWeaponsChanged();

// Hooks beside retail's loops over its five: the dungeon starting and a floor ending unload Expanded's
// effects, retail's resets stop their shots, a loaded floor preloads, and step and draw run them.
void ShotEffectsClear();
void ShotEffectsReset();
void ShotEffectsFloorLoaded();
void ShotEffectsStep();
void ShotEffectsDraw();

// Tests replace the disc load: it sets up effect from its file in arena, returning false on failure.
using ShotEffectLoader = bool (*)(CSHOT_EFFECT &effect, CDataAlloc2<1> &arena, BT_SHOT_EFFECT *description, int shots,
                                  const char *path);
void SetShotEffectLoader(ShotEffectLoader loader);

} // namespace expanded
