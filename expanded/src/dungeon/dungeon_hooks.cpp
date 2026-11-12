#include "expanded/dungeon/dungeon_hooks.hpp"

#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/debug/stage.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/ground_shadow.hpp"
#include "expanded/dungeon/magic_circles.hpp"
#include "expanded/dungeon/navigation.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/ally_unit.hpp"
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
#include "expanded/weapons/goro/battle_ax.hpp"
#include "expanded/weapons/goro/big_hammer.hpp"
#include "expanded/weapons/goro/frozen_tuna.hpp"
#include "expanded/weapons/goro/inferno.hpp"
#include "expanded/weapons/goro/inferno/fire_shot.hpp"
#include "expanded/weapons/goro/inferno/smash_blast.hpp"
#include "expanded/weapons/goro/midas_curse.hpp"
#include "expanded/weapons/goro/midas_touch.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_shot.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_sparks.hpp"
#include "expanded/weapons/goro/satans_ax/underworld_tint.hpp"
#include "expanded/weapons/goro/tall_hammer.hpp"
#include "expanded/weapons/goro/turtle_shell.hpp"
#include "expanded/weapons/heal.hpp"
#include "expanded/weapons/heavens_cloud_shot.hpp"
#include "expanded/weapons/judgement_blade.hpp"
#include "expanded/weapons/kick_back.hpp"
#include "expanded/weapons/kitchen_knife_shot.hpp"
#include "expanded/weapons/osmond/snail.hpp"
#include "expanded/weapons/osmond/star_breaker.hpp"
#include "expanded/weapons/osmond/star_breaker/shooting_stars.hpp"
#include "expanded/weapons/ruby/bandits_ring.hpp"
#include "expanded/weapons/ruby/crystal_ring.hpp"
#include "expanded/weapons/ruby/pocklekul.hpp"
#include "expanded/weapons/ruby/pocklekul/fights.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/toan/agas_sword.hpp"
#include "expanded/weapons/toan/atlamillia_sword.hpp"
#include "expanded/weapons/toan/big_bang.hpp"
#include "expanded/weapons/toan/brave_ark.hpp"
#include "expanded/weapons/toan/choora.hpp"
#include "expanded/weapons/toan/chopper.hpp"
#include "expanded/weapons/toan/claymore.hpp"
#include "expanded/weapons/toan/cross_hinder.hpp"
#include "expanded/weapons/toan/dark_cloud.hpp"
#include "expanded/weapons/toan/dusack.hpp"
#include "expanded/weapons/toan/gladius.hpp"
#include "expanded/weapons/toan/heavens_cloud.hpp"
#include "expanded/weapons/toan/kitchen_knife.hpp"
#include "expanded/weapons/toan/lambs_sword.hpp"
#include "expanded/weapons/toan/macho_sword.hpp"
#include "expanded/weapons/toan/maneater.hpp"
#include "expanded/weapons/toan/sand_breaker.hpp"
#include "expanded/weapons/toan/serpent_sword.hpp"
#include "expanded/weapons/toan/seventh_heaven.hpp"
#include "expanded/weapons/toan/shamshir.hpp"
#include "expanded/weapons/toan/small_sword.hpp"
#include "expanded/weapons/toan/snake_backup.hpp"
#include "expanded/weapons/toan/sun_sword.hpp"
#include "expanded/weapons/toan/sword_of_zeus.hpp"
#include "expanded/weapons/toan/tsukikage.hpp"
#include "expanded/weapons/toan/wise_owl_sword.hpp"
#include "expanded/weapons/ungaga/babels_spear.hpp"
#include "expanded/weapons/ungaga/cactus.hpp"
#include "expanded/weapons/ungaga/cactus/absorb.hpp"
#include "expanded/weapons/ungaga/charge_shot.hpp"
#include "expanded/weapons/ungaga/de_sanga.hpp"
#include "expanded/weapons/ungaga/halberd.hpp"
#include "expanded/weapons/ungaga/hercules_wrath.hpp"
#include "expanded/weapons/ungaga/javelin.hpp"
#include "expanded/weapons/ungaga/mirage.hpp"
#include "expanded/weapons/ungaga/partisan.hpp"
#include "expanded/weapons/ungaga/scorpion.hpp"
#include "expanded/weapons/ungaga/terra_sword.hpp"
#include "expanded/weapons/xiao/angel_gear.hpp"
#include "expanded/weapons/xiao/angel_gear/shield_ring.hpp"
#include "expanded/weapons/xiao/angel_gear/shot_reflect.hpp"
#include "expanded/weapons/xiao/angel_shooter.hpp"
#include "expanded/weapons/xiao/bandit_slingshot.hpp"
#include "expanded/weapons/xiao/divine_beast_title.hpp"
#include "expanded/weapons/xiao/double_impact.hpp"
#include "expanded/weapons/xiao/dragons_y.hpp"
#include "expanded/weapons/xiao/firing_legs.hpp"
#include "expanded/weapons/xiao/flame_gun.hpp"
#include "expanded/weapons/xiao/flamingo.hpp"
#include "expanded/weapons/xiao/hardshooter.hpp"
#include "expanded/weapons/xiao/matador.hpp"
#include "expanded/weapons/xiao/steel_slingshot.hpp"
#include "expanded/weapons/zeus_shot.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"

namespace expanded {

void DungeonShotsStep(CMonstorUnit &unit) {
    CursedPrismStep(unit);
    ShotEffectsStep();
    BossCastStep(unit);
    IceQueenStep(unit);
    CrystalGemronStep(unit);
    AtlaGemronStep(unit);
    BlackKnightStep();
    GuardianReflectorStep();
    MirageStep();
    PocklekulShotsStep();
    ShootingStarsStep();
}

void DungeonFloorUnloaded() {
    PocklekulReset();
    SnakeCharmerReset();
    ShotEffectsClear();
    FloorCharactersClear();
    CursedPrismFloorEnds();
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
    ColdStorageVisitBegins();
    ShootingStarsVisitBegins();
    PumpedUpReset();
}

void DungeonFrameBegins() {
    StageFrame();
    PumpedUpFrame();
    FlameGunFrame();
}

void DungeonShadowsDrawn(const float *player) {
    AtlaDropsDrawOverShadows(player);
}

void DungeonMonstersDrawn() {
    AllyDraw();
    DesertBloomDraw();
    BigRockDraw();
    CurseOfBabelDraw();
    SnakeCharmerDraw();
}

void DungeonNpcsDrawn() {
    MirageCloneDraw();
}

void DungeonEffectsDrawn() {
    ShotEffectsDraw();
    ColdStorageDraw();
    ConfuseStarsDraw();
    SolarGlowDraw();
    BigBangShotDraw();
    ChargingBullDraw();
    SpiritBeastDraw();
    GuardianReflectorDraw();
    SnakeCharmerGlowDraw();
    BurningAuraFlameDraw();
    MidasDraw();
}

void DungeonRasterDrawn() {
    MirageHazeDraw();
    SnakeCharmerHazeDraw();
    BurningAuraHazeDraw();
}

void DungeonShotsDrawn() {
    BombGemronEffectsDraw();
    ShootingStarsDraw();
}

void DungeonRunDustDrawn() {
    UnderworldSparksDraw();
}

bool PlayerHitPassesBy(const COLLISION_HIT &hit) {
    return PlayerHitSkipped(hit) || RoughCounterPassesBy(hit) || PackHitPassesBy(hit) || ColossusPassesBy(hit) || InfernoFireproof(hit);
}

void PlayerHitReached(COLLISION_HIT &hit) {
    if (!MidasShrugs()) {
        BreakFreeze(&hit); // gold Goro's freeze holds
    }
    MidasPlayerHit(hit);
    RoughCounterHit(hit);
    DivineGuardHit(hit);
    HerosCourageHit(hit);
}

void PlayerHitReacted(bool blocked, const COLLISION_HIT &hit) {
    MidasPlayerReacted(hit);
    DivineGuardBlocked(blocked);
    ColossusShrugged(blocked);
    ColossusHitTaken(hit);
    LambGuardBlocked(blocked, hit);
    RoughCounterBlocked(blocked, hit);
}

void DungeonMonstersCleared(bool drop_stars) {
    PocklekulReset();
    ConfuseFloorReset(drop_stars);
    ShotEffectsReset();
    SolarFlashReset();
    BlindReset();
    JudgementReset();
    SceneLightReset();
    BigBangShotReset();
    GemShotReset();
    ZeusChargeReset();
    ZeusShotReset();
    OwnSoundsReset();
    ColdStorageCleared();
    ColossusQuiet();
}

void DungeonMonstersTick() {
    MonsterStepClockAdvance();
    angel_gear::TrackReturns();
    RoughCounterStep();
    OwnSoundsStep();
    ConfuseTick();
    CurseStep();
    MidasCurseStep();
    AtlamilliaStep();
    BloodPriceStep();
    GreatswordStep();
    BigHammerStep();
    MidasStep();
    TyphoonStep();
    HerosCourageStep();
    BlindTick();
    CircleSlowStep();
    SceneLightStep();
    JudgementStep();
    SolarFlashStep();
    BigBangShotStep();
    GemShotStep();
    ZeusChargeStep();
    ZeusShotStep();
    TyphoonShotStep();
    ChargingBullStep();
    UnderworldShotStep();
    DragonShotStep();
    TornadoChargeStep();
    GuardianGraceStep();
    DesertBloomStep();
    ColossusStep();
    SnakeCharmerStep();
    UnderworldSparksStep();
    UnderworldTintStep();
    BurningAuraStep();
    BigRockStep();
    CurseOfBabelStep();
    AirStrikeStep();
    ColdStorageStep();
    if (NowMonstorUnit != nullptr) {
        WiseOwlStep(*NowMonstorUnit);
    }
    StageStep();
    EventLogStep();
}

void DungeonMonstersStepped(int pause) {
    PocklekulStep(pause);
    PackStep(pause);
    AllyStep(pause);
}

void PlayerShotsStepped() {
    SpiritBeastStep();
}

void MonsterPackFile(std::string &filename, int model_no) {
    BossPackFile(filename, model_no);
    SpeciesPackFile(filename, model_no);
}

void MonsterDrawing(CMonstorUnit &unit, int monster) {
    if (AllyUnit(unit)) {
        return;
    }
    MidasDrawing(unit, monster);
    BombGemronDrawing(unit, monster);
    CrystalGemronDrawing(unit, monster);
    AtlaGemronDrawing(unit, monster);
}

void MonsterDrawn(CMonstorUnit &unit, int monster) {
    if (AllyUnit(unit)) {
        return;
    }
    MidasDrawn(unit, monster);
    BombGemronDrawn(unit, monster);
    AtlaGemronDrawn(unit, monster);
}

bool MonsterKilled(const CMonstorUnit &unit, int monster) {
    if (AllyUnit(unit)) {
        return false;
    }
    EventLogKilled(unit, monster);
    BossRecordKill(unit, monster);
    VampireKill(unit, monster);
    PumpedUpKill(unit, monster);
    return BossCastCountsKill(monster);
}

int MonsterDropItem(const CMonstorUnit &unit, int monster, int drop_item) {
    if (AllyUnit(unit)) {
        return drop_item;
    }
    return SolarHarvestDrop(unit, monster, MidasDropItem(unit, monster, drop_item));
}

int MonsterKillAbs(const CMonstorUnit &unit, int monster, int abs) {
    if (AllyUnit(unit)) {
        return abs;
    }
    int result = ArmorPiercerAbs(
        unit, monster, MarineHunterAbs(unit, monster, SanctifierAbs(unit, monster, WoodcutterAbs(unit, monster, UtilityKnifeAbs(unit, monster, JacketHunterAbs(unit, monster, abs))))));
    pocklekul::PockleKillAbs(monster, result);
    return result;
}

float MonsterWeaponDamage(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, float damage) {
    if (AllyUnit(unit)) {
        return damage;
    }
    return ArmorPiercerDamage(unit, monster, hit, SanctifierDamage(unit, monster, hit, WoodcutterDamage(unit, monster, hit, UtilityKnifeDamage(unit, monster, hit, damage))));
}

int MonsterDefense(const CMonstorUnit &unit, int monster, int defense) {
    if (AllyUnit(unit)) {
        return defense;
    }
    return MarineHunterDefense(unit, monster, defense);
}

int MonsterHardness(const CMonstorUnit &unit, int monster, int hardness) {
    if (AllyUnit(unit)) {
        return hardness;
    }
    return MarineHunterHardness(unit, monster, hardness);
}

int PlayerDefense(int chara, int defense) {
    return ColossusDefense(chara, DefensiveLegacyDefense(chara, HardShellDefense(chara, defense)));
}

float PlayerLockOnRange(float rate) {
    return LockOnDistanceRate(HolyReachRate(MirageLineReachRate(rate)));
}

bool MonsterStepTaken(CMonstorUnit &unit, int monster) {
    if (AllyUnit(unit)) {
        return false;
    }
    return BossDeathStep(unit, monster) || BombGemronStep(unit, monster) || ReviveStep(unit, monster) ||
           BlindStep(unit, monster) || SnakeCharmerMonsterStep(unit, monster, BurningAuraFeared(unit, monster));
}

char *MonsterScriptLoaded(CMonstorUnit &unit, int slot, int model_no, char *script, int size) {
    return SnakeBackupScript(unit, slot, model_no, script, size);
}

void MonsterScriptStepped(CMonstorUnit &unit, int monster) {
    if (!AllyUnit(unit)) {
        SnakeCharmerScriptStepped(unit, monster);
    }
}

void PelletFired(CSHOT &shots, int slot) {
    MoonlitFocusShot(shots.vector[slot]);
    SolarShotFired(shots, slot);
    BigBangShotFired(shots, slot);
    GemShotFired(shots, slot);
    ZeusShotFired(shots, slot);
    TyphoonShotFired(shots, slot);
    ColossusPelletFired(shots, slot);
    StealShotFired(shots, slot);
    RicochetShotFired(slot);
    DoubleImpactFired(shots, slot);
    ChargingBullFired(shots, slot);
    UnderworldShotFired(shots, slot);
    DragonShotFired(shots, slot);
    TornadoShotFired(shots, slot);
    SpiritBeastFired(shots, slot);
    AirStrikeFired(shots, slot);
}

void RubyOrbThrown(CSHOT_EFFECT &effect, int slot, float scale, bool repeat) {
    PrismOrbThrown(effect, slot, scale, repeat);
}

void PelletEnded(CSHOT &shots, int slot, const float *point, bool struck) {
    SolarShotEnded(shots, slot, point, struck);
    BigBangShotEnded(shots, slot, point);
    GemShotEnded(shots, slot);
    ZeusShotEnded(shots, slot, point, struck);
    TyphoonShotEnded(shots, slot, point, struck);
    RicochetShotEnded(shots, slot, point, struck);
    ChargingBullEnded(shots, slot);
    FireShotEnded(shots, slot, point); // before its pull lets it go
    UnderworldShotEnded(shots, slot);
    AirStrikeEnded(shots, slot, point);
}

void PelletHitSet(const CSHOT &shots, int slot, COLLISION_HIT &hit) {
    ChargingBullHit(shots, slot, hit);
    UnderworldShotHit(shots, slot, hit);
    FireShotHit(shots, slot, hit);
    PumpedUpPelletHit(hit);
}

void ShotEffectHitSet(CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit) {
    DragonShotHit(effect, slot, hit);
    angel_gear::NoteShotHit(effect, slot, hit);
    ColossusShotHitSet(effect, slot, hit);
    InfernoImpactHit(effect, slot, hit);
    BigHammerHitSet(effect, slot, hit);
}

void PelletStepping(CSHOT &shots, int slot) {
    RicochetStep(shots, slot);
}

bool PelletHidden(const CSHOT &shots, int slot) {
    return FireShotHidden(shots, slot);
}

bool PelletPierces(const CSHOT &shots, int slot) {
    return UnderworldShotPierces(shots, slot);
}

bool PelletHurts(const CSHOT &shots, int slot) {
    return !ZeusShotCallsBolt(shots, slot) && !BigBangShotBlasts(shots, slot);
}

bool TargetNameHidden() {
    return JudgementHanging() || JudgementFalling() || BigBangShotHanging();
}

bool RandomItemHidden(const CRandomItem &items, int slot) {
    return StarDropHidden(items, slot);
}

bool MonsterHitRefused(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit) {
    if (AllyUnit(unit)) {
        return pocklekul::PockleRefuses(hit);
    }
    return pocklekul::PockleHitMeantElsewhere(monster, hit) || DragonShotRefused(monster, hit) || UnderworldShotRefused(monster, hit) ||
           BlastAimedElsewhere(monster, hit) || ColdStorageHitRefused(hit);
}

void MonsterHitTaken(CMonstorUnit &unit, int monster, COLLISION_HIT &hit) {
    if (AllyUnit(unit)) {
        return;
    }
    SpiritBeastHitTaken(hit);
    MirageHitTaken(monster);
    BigRockHitTaken(unit, monster, hit);
    StarBreakerHitLanded(unit, monster, hit, false);
    DoubleTakeHitTaken(unit, monster, hit);
    MidasHitTaken(unit, monster, hit);
}

void MonsterHitBlocked(const CMonstorUnit &unit, int monster, COLLISION_HIT &hit) {
    if (AllyUnit(unit)) {
        return;
    }
    StarBreakerHitLanded(unit, monster, hit, true);
}

void MonsterPoisoned(const MONSTOR &target) {
    if (AllyStepping()) {
        return;
    }
    VenomPoisoned(target);
}

int MonsterHitAmount(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    if (AllyUnit(unit)) {
        return amount;
    }
    return SnakeSlayerAmount(unit, monster, hit, amount);
}

void MonsterDamaged(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount) {
    if (AllyUnit(unit)) {
        return;
    }
    EventLogDamaged(unit, monster, hit, amount);
    CactusMonsterDamaged(unit, monster, hit, amount);
    SnailMonsterDamaged(unit, monster, hit, amount);
    ColdStorageMonsterDamaged(unit, monster, hit, amount);
    pocklekul::PockleMonsterDamaged(monster, hit, amount);
    MidasDamaged(unit, monster, hit);
}

void MonsterSwingSet(CMonstorUnit &unit, int, int hit) {
    if (AllyUnit(unit)) {
        pocklekul::PockleSwingSet(unit, hit);
    }
}

std::optional<MonsterDamageResult> MonsterFriendlyHit(CMonstorUnit &unit, int sphere) {
    return AllyUnit(unit) ? pocklekul::PockleHitOn(unit, sphere) : ConfuseMonsterHit(unit, sphere);
}

bool HitWearsWeapon(const COLLISION_HIT &hit) {
    return !DesertBloomPrick(hit) && !CurseOfBabelCatch(hit) && !UngagaBlastHit(hit) && !BigRockHit(hit) && !ColossusTrample(hit);
}

bool MonsterPathBlockedBy(const CMonstorUnit &unit, int monster, int other) {
    return ConfuseTargetMonster(unit, monster) != other;
}

void MonsterMoveChecked(CMonstorUnit &unit, int monster) {
    pocklekul::PockleMoveChecked(unit, monster);
    if (!AllyUnit(unit)) {
        SolidColumnMonsterMove(unit, monster);
        SnakeCharmerMonsterMove(unit, monster);
        UnderworldMonsterMove(unit, monster);
    }
}

void PlayerMoveChecked(const float *position, float *velocity) {
    MidasPlayerMove(velocity);
    AllyPlayerMove(position, velocity);
    SolidColumnPlayerMove(position, velocity);
}

void DungeonShadowsDrawing() {
    AllyShadow();
    GroundShadowDraw();
}

void ScriptPlayerPosition(float *position) {
    if (NowMonstorUnit != nullptr && (PocklekulScriptTarget(position) || angel_gear::RingTarget(NowMonstorUnit->current_monster, position) ||
                                      MirageTarget(NowMonstorUnit->current_monster, position))) {
        return;
    }
    ConfuseScriptPlayerPosition(position);
}

float MonsterBlockRadius(float radius) {
    return angel_gear::RingBlockRadius(ColossusBlockRadius(radius));
}

float PlayerHurtHeight(float height) {
    return ColossusHeight(height);
}

float PlayerHurtReach() {
    return ColossusHurtReach();
}

bool PlayerHitShrugged(const COLLISION_HIT &hit) {
    return ColossusShrugsOff(hit.kind) || MidasShrugs();
}

int PlayerHitDamage(int damage) {
    return MidasPlayerDamage(damage);
}

int PlayerHitKind(const COLLISION_HIT &hit) {
    return ColossusHitKind(hit.kind);
}

bool HitSwitchSteppedOn(int chara_no, const float *pos, float radius) {
    return ColossusStepsOn(chara_no, pos, radius);
}

SHOT_COLLISION_RESULT ShotEffectCollision(CSHOT_EFFECT &effect, int slot, float *hit_position, float *position) {
    SHOT_COLLISION_RESULT result = angel_gear::ReflectCollision(effect, slot, hit_position, position);
    return ColossusShotCollision(effect, slot, SolidColumnShot(effect, slot, result, hit_position, position));
}

void ShotPlayerPoint(float *point) {
    ColossusShotPoint(point);
    GuardianReflectorPlayerPoint(point);
}

float ActionGaugeGain(float gain) {
    return FlameGunGaugeGain(GuardianReflectorGaugeGain(gain));
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
    FiringLegsStep(chara, chara_no);
    ColossusMotion(chara, chara_no);
}

void HealTicked() {
    AngelGearHealTick();
}

void PlayerHitsChecking() {
    GuardianReflectorSpend();
}

bool XiaoShotPlaying(CCharacter &chara, int aimed) {
    QuickDrawShot(chara, aimed);
    return FlameGunShot(aimed);
}

float WeaponWear(const WEAPON_HAVE &weapon, float amount) {
    return HardShellWear(weapon, amount);
}

float XiaoShotWear(float wear) {
    return ChargedShotWear(EnduranceUpShotWear(wear));
}

int MonsterSpherePercent(const CMonstorUnit &unit, int monster, int sphere, int owner, const COLLISION_HIT &hit, int percent) {
    if (AllyUnit(unit)) {
        return percent;
    }
    return SpiritBeastSpherePercent(unit.effect[monster], sphere, owner, hit, percent);
}

bool PlayerHitStaggers(const COLLISION_HIT &hit) {
    return SpiritBeastHit(hit);
}

bool LandedHitWearsWeapon(int attacker, const COLLISION_HIT &hit) {
    return !ChargeShotHit(attacker, &hit) && HitWearsWeapon(hit);
}

void MonsterKnockedBack(CMonstorUnit &unit, int monster, const COLLISION_HIT &hit) {
    if (AllyUnit(unit)) {
        return;
    }
    WeaponKickBack(unit, monster, hit);
}

void MonsterItemStolen(const CMonstorUnit &unit, int monster, int item) {
    if (AllyUnit(unit)) {
        return;
    }
    StealShotTaken(unit, monster, item);
    DoubleTakeStolen(unit, monster, item);
}

bool KillAbsTaken(WEAPON_HAVE &weapon, int abs, int max_abs) {
    return OvertrainingAbs(weapon, abs, max_abs);
}

bool AilmentAllowed(int ailment) {
    return CurseAllowsAilment(ailment) && HerosCourageAllowsAilment(ailment) && MidasAllowsAilment();
}

bool MonsterCanGuard(int monster) {
    return BlindGuardBlocks(monster);
}

bool MonsterGuardBlocks(const CMonstorUnit &unit, int monster, int window, const COLLISION_HIT &hit) {
    if (AllyUnit(unit)) {
        return true;
    }
    return MonsterCanGuard(monster) && !BlastHit(hit) && !ChargingBullHit(hit) && !SpiritBeastHit(hit) && !BigRockHit(hit) && !GuardCrushActive() &&
           !NoFoolsGoldOpens(unit, monster, window);
}

} // namespace expanded
