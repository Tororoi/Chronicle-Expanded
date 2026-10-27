#include "expanded/enemies/faster.hpp"

#include "character.hpp"
#include "expanded/core/options.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "monstorunit.hpp"

namespace expanded {

namespace {

[[maybe_unused]] const bool kRegistered = AddToggle({.key = "faster_enemies",
                                                     .label = "Faster Enemies",
                                                     .help = "\"Faster Enemies\"\nMonsters move and\nattack twice as fast."});

} // namespace

bool FasterEnemiesOn() {
    return OptionOn("faster_enemies");
}

float FasterMoveSpeed(float speed) {
    return speed > 0.0f && FasterEnemiesOn() && !AllyStepping() ? speed * kFasterMoveScale : speed;
}

bool FasterInHitWindow(const CMonstorUnit &unit, int monster_no, float time) {
    const MONSTOR_EFFECT_STATE2 &hits = unit.effect2[monster_no];

    for (int i = 0; i < 16; i++) {
        if (hits.active[i] != 0 && time >= hits.motion_start[i] - kFasterHitWindowMargin &&
            time <= hits.motion_end[i] + kFasterHitWindowMargin) {
            return true;
        }
    }

    return false;
}

void FasterMotionStep(CMonstorUnit &unit, int monster_no, CCharacter &character) {
    float speed = character.motion_speed;

    // Motions on their own rate (-1) keep it; attacks keep theirs through the hit window so the hit lands.
    if (speed <= 0.0f || !FasterEnemiesOn() || AllyUnit(unit) ||
        FasterInHitWindow(unit, monster_no, unit.chara[monster_no][0].motion_type.state.time)) {
        character.Step();
        return;
    }

    character.motion_speed = speed * kFasterMotionScale;
    character.Step();
    character.motion_speed = speed;
}

} // namespace expanded
