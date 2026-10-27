#include "expanded/dungeon/dungeon_hooks.hpp"

#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/debug/stage.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/ground_shadow.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/dungeon/scene_light.hpp"
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
#include "expanded/status/freeze.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/big_bang_shot.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/curse.hpp"
#include "expanded/weapons/heal.hpp"
#include "expanded/weapons/heavens_cloud_shot.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/kick_back.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/toan/agas_sword.hpp"
#include "expanded/weapons/toan/atlamillia_sword.hpp"
#include "expanded/weapons/toan/big_bang.hpp"
#include "expanded/weapons/toan/brave_ark.hpp"
#include "expanded/weapons/toan/claymore.hpp"
#include "expanded/weapons/toan/cross_hinder.hpp"
#include "expanded/weapons/toan/dark_cloud.hpp"
#include "expanded/weapons/toan/dusack.hpp"
#include "expanded/weapons/toan/gladius.hpp"
#include "expanded/weapons/toan/heavens_cloud.hpp"
#include "expanded/weapons/toan/macho_sword.hpp"
#include "expanded/weapons/toan/maneater.hpp"
#include "expanded/weapons/toan/seventh_heaven.hpp"
#include "expanded/weapons/toan/shamshir.hpp"
#include "expanded/weapons/toan/sun_sword.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "expanded/weapons/toan/tsukikage.hpp"
#include "expanded/weapons/toan/wise_owl_sword.hpp"
#include "expanded/weapons/ungaga/cactus.hpp"
#include "expanded/weapons/ungaga/charge_shot.hpp"
#include "expanded/weapons/ungaga/de_sanga.hpp"
#include "expanded/weapons/ungaga/halberd.hpp"
#include "expanded/weapons/ungaga/hercules_wrath.hpp"
#include "expanded/weapons/ungaga/javelin.hpp"
#include "expanded/weapons/ungaga/mirage.hpp"
#include "expanded/weapons/ungaga/partisan.hpp"
#include "expanded/weapons/ungaga/scorpion.hpp"
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
#include "expanded/weapons/xiao/steel_slingshot.hpp"
#include "expanded/weapons/zeus_shot.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"

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

void DungeonFloorUnloaded() {
    ShotEffectsClear();
    FloorCharactersClear();
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
    EventLogFloorLoaded();
}

void DungeonStatusReady() {
    StageStatusReady();
}

void DungeonFrameBegins() {
    StageFrame();
}

void DungeonShadowsDrawn(const float *player) {
    AtlaDropsDrawOverShadows(player);
}

void DungeonMonstersDrawn() {
    DesertBloomDraw();
}

void DungeonNpcsDrawn() {
    MirageCloneDraw();
}

void DungeonEffectsDrawn() {
    ShotEffectsDraw();
    ConfuseStarsDraw();
    SolarGlowDraw();
    BigBangShotDraw();
    ChargingBullDraw();
    SpiritBeastDraw();
    GuardianReflectorDraw();
}

void DungeonRasterDrawn() {
    MirageHazeDraw();
}

void DungeonShotsDrawn() {
    BombGemronEffectsDraw();
}

bool PlayerHitPassesBy(const COLLISION_HIT &hit) {
    return PlayerHitSkipped(hit);
}

void PlayerHitReached(COLLISION_HIT &hit) {
    BreakFreeze(&hit);
    DivineGuardHit(hit);
    HerosCourageHit(hit);
}

void PlayerHitReacted(bool blocked) {
    DivineGuardBlocked(blocked);
}

void DungeonMonstersCleared(bool drop_stars) {
    ConfuseFloorReset(drop_stars);
    ShotEffectsReset();
    SolarFlashReset();
    BlindReset();
    JudgementReset();
    SceneLightReset();
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
    BlindTick();
    SceneLightStep();
    JudgementStep();
    SolarFlashStep();
    BigBangShotStep();
    ZeusChargeStep();
    ZeusShotStep();
    TyphoonShotStep();
    ChargingBullStep();
    DragonShotStep();
    TornadoChargeStep();
    GuardianGraceStep();
    DesertBloomStep();
    AirStrikeStep();
    if (NowMonstorUnit != nullptr) {
        WiseOwlStep(*NowMonstorUnit);
    }
    StageStep();
    EventLogStep();
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
    EventLogKilled(unit, monster);
    BossRecordKill(unit, monster);
    VampireKill(unit, monster);
    return BossCastCountsKill(monster);
}

int MonsterDropItem(const CMonstorUnit &unit, int monster, int drop_item) {
    return SolarHarvestDrop(unit, monster, drop_item);
}

int MonsterKillAbs(const CMonstorUnit &unit, int monster, int abs) {
    return MarineHunterAbs(unit, monster, SanctifierAbs(unit, monster, JacketHunterAbs(unit, monster, abs)));
}

float MonsterWeaponDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    return SanctifierDamage(unit, monster, hit, damage);
}

int MonsterDefense(const CMonstorUnit &unit, int monster, int defense) {
    return MarineHunterDefense(unit, monster, defense);
}

int MonsterHardness(const CMonstorUnit &unit, int monster, int hardness) {
    return MarineHunterHardness(unit, monster, hardness);
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
    MoonlitFocusShot(shots.vector[slot]);
    SolarShotFired(shots, slot);
    BigBangShotFired(shots, slot);
    ZeusShotFired(shots, slot);
    TyphoonShotFired(shots, slot);
    StealShotFired(shots, slot);
    RicochetShotFired(slot);
    DoubleImpactFired(shots, slot);
    ChargingBullFired(shots, slot);
    DragonShotFired(shots, slot);
    TornadoShotFired(shots, slot);
    SpiritBeastFired(shots, slot);
    AirStrikeFired(shots, slot);
}

void PelletEnded(CSHOT &shots, int slot, const float *point, bool struck) {
    SolarShotEnded(shots, slot, point, struck);
    BigBangShotEnded(shots, slot, point);
    ZeusShotEnded(shots, slot, point, struck);
    TyphoonShotEnded(shots, slot, point, struck);
    RicochetShotEnded(shots, slot, point, struck);
    ChargingBullEnded(shots, slot);
    AirStrikeEnded(shots, slot, point);
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

void MonsterPoisoned(const MONSTOR &target) {
    VenomPoisoned(target);
}

void MonsterDamaged(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    EventLogDamaged(unit, monster, amount);
    CactusMonsterDamaged(unit, monster, hit, amount);
}

bool HitWearsWeapon(const COLLISION_HIT &hit) {
    return !DesertBloomPrick(hit) && !UngagaBlastHit(hit);
}

void MonsterMoveChecked(CMonstorUnit &unit, int monster) {
    SolidColumnMonsterMove(unit, monster);
}

void PlayerMoveChecked(const float *position, float *velocity) {
    SolidColumnPlayerMove(position, velocity);
}

void DungeonShadowsDrawing() {
    GroundShadowDraw();
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
    SHOT_COLLISION_RESULT result = angel_gear::ReflectCollision(effect, slot, hit_position, position);
    return SolidColumnShot(effect, slot, result, hit_position, position);
}

void ShotPlayerPoint(float *point) {
    GuardianReflectorPlayerPoint(point);
}

float ActionGaugeGain(float gain) {
    return GuardianReflectorGaugeGain(gain);
}

int HealCounter(int counter) {
    return GuardianGraceCounter(counter, kHealPeriod);
}

bool HealsWithoutTheAbility() {
    return GuardianGraceHeals();
}

void PlayerMotionStepping(CCharacter &chara, int chara_no) {
    SwiftStrikesMotion(chara, chara_no);
    QuickComboMotion(chara, chara_no);
    LockOnSpeedMotion(chara, chara_no);
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

bool LandedHitWearsWeapon(int attacker, const COLLISION_HIT &hit) {
    return !ChargeShotHit(attacker, &hit) && HitWearsWeapon(hit);
}

void MonsterKnockedBack(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit) {
    WeaponKickBack(unit, monster, hit);
}

void MonsterItemStolen(const CMonstorUnit &unit, int monster, int item) {
    StealShotTaken(unit, monster, item);
}

bool KillAbsTaken(WEAPON_HAVE &weapon, int abs, int max_abs) {
    return OvertrainingAbs(weapon, abs, max_abs);
}

bool AilmentAllowed(int ailment) {
    return CurseAllowsAilment(ailment) && HerosCourageAllowsAilment(ailment);
}

bool MonsterCanGuard(int monster) {
    return BlindGuardBlocks(monster);
}

bool MonsterGuardBlocks(const CMonstorUnit &unit, int monster, int window, const COLLISION_HIT &hit) {
    return MonsterCanGuard(monster) && !BlastHit(hit) && !ChargingBullHit(hit) && !SpiritBeastHit(hit) && !GuardCrushActive() &&
           !NoFoolsGoldOpens(unit, monster, window);
}

} // namespace expanded
