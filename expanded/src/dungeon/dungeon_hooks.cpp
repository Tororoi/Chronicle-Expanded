#include "expanded/dungeon/dungeon_hooks.hpp"

#include "dun/gameloop.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/atla_gemron.hpp"
#include "expanded/enemies/black_knight.hpp"
#include "expanded/enemies/bomb_gemron.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/boss_record.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/crystal_gemron.hpp"
#include "expanded/enemies/ice_queen.hpp"
#include "expanded/enemies/revive.hpp"
#include "expanded/enemies/sounds.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/enemies/steve.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/big_bang_shot.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/heavens_cloud_shot.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/toan/agas_sword.hpp"
#include "expanded/weapons/toan/atlamillia_sword.hpp"
#include "expanded/weapons/toan/brave_ark.hpp"
#include "expanded/weapons/toan/claymore.hpp"
#include "expanded/weapons/toan/cross_hinder.hpp"
#include "expanded/weapons/toan/dark_cloud.hpp"
#include "expanded/weapons/toan/dusack.hpp"
#include "expanded/weapons/toan/gladius.hpp"
#include "expanded/weapons/toan/heavens_cloud.hpp"
#include "expanded/weapons/toan/maneater.hpp"
#include "expanded/weapons/toan/sun_sword.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "expanded/weapons/toan/wise_owl_sword.hpp"
#include "expanded/weapons/ungaga/mirage.hpp"
#include "expanded/weapons/xiao/angel_gear.hpp"
#include "expanded/weapons/xiao/angel_gear/shield_ring.hpp"
#include "expanded/weapons/xiao/angel_gear/shot_reflect.hpp"
#include "expanded/weapons/xiao/angel_shooter.hpp"
#include "expanded/weapons/xiao/bandit_slingshot.hpp"
#include "expanded/weapons/xiao/divine_beast_title.hpp"
#include "expanded/weapons/xiao/double_impact.hpp"
#include "expanded/weapons/xiao/dragons_y.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"
#include "expanded/weapons/xiao/hardshooter.hpp"
#include "expanded/weapons/xiao/matador.hpp"
#include "expanded/weapons/xiao/shot_charge.hpp"
#include "expanded/weapons/xiao/steel_slingshot.hpp"
#include "expanded/weapons/zeus_shot.hpp"
#include "monstorunit.hpp"

namespace expanded {

void DungeonShotsStep(CMonstorUnit &unit) {
    ShotEffectsStep();
    BossCastStep(unit);
    IceQueenStep(unit);
    CrystalGemronStep(unit);
    AtlaGemronStep(unit);
    BlackKnightStep();
    GuardianReflectorStep();
    MirageStep();
}

void DungeonFloorLoaded(CMonstorUnit &unit) {
    ConfuseFloorLoaded();
    ShotEffectsFloorLoaded();
    NavigationFloorLoaded();
    MonsterSoundsFloorLoaded(unit);
    OwnSoundsFloorLoaded();
    SteveLinesFloorLoaded(unit);
    AtlaDropsFloorLoaded();
    AtlamilliaFloorLoaded();
}

void DungeonMonstersCleared(bool drop_stars) {
    ConfuseFloorReset(drop_stars);
    ShotEffectsReset();
    SolarFlashReset();
    BigBangShotReset();
    ZeusChargeReset();
    ZeusShotReset();
    OwnSoundsReset();
}

void DungeonMonstersTick() {
    MonsterStepClockAdvance();
    OwnSoundsStep();
    ConfuseTick();
    CurseStep();
    AtlamilliaStep();
    BloodPriceStep();
    GreatswordStep();
    TyphoonStep();
    HerosCourageStep();
    SolarFlashStep();
    BigBangShotStep();
    ZeusChargeStep();
    ZeusShotStep();
    TyphoonShotStep();
    ChargingBullStep();
    DragonShotStep();
    GuardianGraceStep();
    if (NowMonstorUnit != nullptr) {
        WiseOwlStep(*NowMonstorUnit);
    }
}

void PlayerShotsStepped() {
    SpiritBeastStep();
}

void MonsterPackFile(char *filename, int model_no) {
    BossPackFile(filename, model_no);
    SpeciesPackFile(filename, model_no);
}

void MonsterDrawing(CMonstorUnit &unit, int monster) {
    BombGemronDrawing(unit, monster);
    CrystalGemronDrawing(unit, monster);
    AtlaGemronDrawing(unit, monster);
}

void MonsterDrawn(CMonstorUnit &unit, int monster) {
    BombGemronDrawn(unit, monster);
    AtlaGemronDrawn(unit, monster);
}

bool MonsterKilled(const CMonstorUnit &unit, int monster) {
    BossRecordKill(unit, monster);
    return BossCastCountsKill(monster);
}

int MonsterDropItem(const CMonstorUnit &unit, int monster, int drop_item) {
    return SolarHarvestDrop(unit, monster, drop_item);
}

int MonsterKillAbs(const CMonstorUnit &unit, int monster, int abs) {
    return SanctifierAbs(unit, monster, JacketHunterAbs(unit, monster, abs));
}

float MonsterWeaponDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    return SanctifierDamage(unit, monster, hit, damage);
}

int PlayerDefense(int chara, int defense) {
    return DefensiveLegacyDefense(chara, defense);
}

float PlayerLockOnRange(float rate) {
    return LockOnDistanceRate(HolyReachRate(MirageLineReachRate(rate)));
}

bool MonsterStepTaken(CMonstorUnit &unit, int monster) {
    return BossDeathStep(unit, monster) || BombGemronStep(unit, monster) || ReviveStep(unit, monster) ||
           BlindStep(unit, monster);
}

void PelletFired(CSHOT &shots, int slot) {
    SolarShotFired(shots, slot);
    BigBangShotFired(shots, slot);
    ZeusShotFired(shots, slot);
    TyphoonShotFired(shots, slot);
    StealShotFired(shots, slot);
    RicochetShotFired(slot);
    DoubleImpactFired(shots, slot);
    ChargingBullFired(shots, slot);
    DragonShotFired(shots, slot);
    SpiritBeastFired(shots, slot);
}

void PelletEnded(CSHOT &shots, int slot, const float *point, bool struck) {
    SolarShotEnded(shots, slot, point, struck);
    BigBangShotEnded(shots, slot, point);
    ZeusShotEnded(shots, slot, point, struck);
    TyphoonShotEnded(shots, slot, point, struck);
    RicochetShotEnded(shots, slot, point, struck);
    ChargingBullEnded(shots, slot);
}

void PelletHitSet(const CSHOT &shots, int slot, COLLISION_HIT &hit) {
    ChargingBullHit(shots, slot, hit);
}

void ShotEffectHitSet(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit) {
    DragonShotHit(effect, slot, hit);
}

void PelletStepping(CSHOT &shots, int slot) {
    RicochetStep(shots, slot);
}

bool PelletHurts(const CSHOT &shots, int slot) {
    return !ZeusShotCallsBolt(shots, slot) && !BigBangShotBlasts(shots, slot);
}

bool TargetNameHidden() {
    return JudgementHanging() || JudgementFalling() || BigBangShotHanging();
}

bool MonsterHitRefused(const CMonstorUnit &, int monster, const COLLISION_HIT &hit) {
    return DragonShotRefused(monster, hit);
}

void MonsterHitTaken(const CMonstorUnit &, int monster, const COLLISION_HIT &hit) {
    SpiritBeastHitTaken(hit);
    MirageHitTaken(monster);
}

void ScriptPlayerPosition(float *position) {
    if (NowMonstorUnit != nullptr && (angel_gear::RingTarget(NowMonstorUnit->current_monster, position) ||
                                      MirageTarget(NowMonstorUnit->current_monster, position))) {
        return;
    }
    ConfuseScriptPlayerPosition(position);
}

float MonsterBlockRadius(float radius) {
    return angel_gear::RingBlockRadius(radius);
}

SHOT_COLLISION_RESULT ShotEffectCollision(CSHOT_EFFECT &effect, int slot, float *hit_position, float *position) {
    return angel_gear::ReflectCollision(effect, slot, hit_position, position);
}

void ShotPlayerPoint(float *point) {
    GuardianReflectorPlayerPoint(point);
}

float ActionGaugeGain(float gain) {
    return GuardianReflectorGaugeGain(gain);
}

void HealTicked() {
    AngelGearHealTick();
}

void PlayerHitsChecking() {
    GuardianReflectorSpend();
}

float XiaoShotWear(float wear) {
    return ChargedShotWear(EnduranceUpShotWear(wear));
}

int MonsterSpherePercent(const CMonstorUnit &unit, int monster, int sphere, int owner, const COLLISION_HIT &hit, int percent) {
    return SpiritBeastSpherePercent(unit.effect[monster], sphere, owner, hit, percent);
}

bool PlayerHitStaggers(const COLLISION_HIT &hit) {
    return SpiritBeastHit(hit);
}

bool MonsterGuardBlocks(const CMonstorUnit &unit, int monster, int window, const COLLISION_HIT &hit) {
    return BlindGuardBlocks(monster) && !BlastHit(hit) && !ChargingBullHit(hit) && !SpiritBeastHit(hit) && !GuardCrushActive() &&
           !NoFoolsGoldOpens(unit, monster, window);
}

} // namespace expanded
