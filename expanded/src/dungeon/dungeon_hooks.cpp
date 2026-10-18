#include "expanded/dungeon/dungeon_hooks.hpp"

#include "dun/gameloop.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/dungeon/navigation.hpp"
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
#include "expanded/status/confuse.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/toan/gladius.hpp"
#include "expanded/weapons/toan/wise_owl_sword.hpp"

namespace expanded {

void DungeonShotsStep(CMonstorUnit &unit) {
    ShotEffectsStep();
    BossCastStep(unit);
    IceQueenStep(unit);
    CrystalGemronStep(unit);
    AtlaGemronStep(unit);
    BlackKnightStep();
}

void DungeonFloorLoaded(CMonstorUnit &unit) {
    ConfuseFloorLoaded();
    ShotEffectsFloorLoaded();
    NavigationFloorLoaded();
    MonsterSoundsFloorLoaded(unit);
    SteveLinesFloorLoaded(unit);
    AtlaDropsFloorLoaded();
}

void DungeonMonstersCleared(bool drop_stars) {
    ConfuseFloorReset(drop_stars);
    ShotEffectsReset();
}

void DungeonMonstersTick() {
    MonsterStepClockAdvance();
    ConfuseTick();
    if (NowMonstorUnit != nullptr) {
        WiseOwlStep(*NowMonstorUnit);
    }
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

int MonsterKillAbs(const CMonstorUnit &unit, int monster, int abs) {
    return JacketHunterAbs(unit, monster, abs);
}

bool MonsterStepTaken(CMonstorUnit &unit, int monster) {
    return BossDeathStep(unit, monster) || BombGemronStep(unit, monster) || ReviveStep(unit, monster);
}

} // namespace expanded
