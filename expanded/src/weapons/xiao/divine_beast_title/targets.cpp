#include "expanded/weapons/xiao/divine_beast_title/targets.hpp"

#include <cmath>

#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/monster_scripts.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/miniboss.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/weapons/xiao/divine_beast_title.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

extern s32 lockOnTargetNo;

namespace expanded::divine_beast_title {

namespace {

constexpr float kTallScale = 1.25f; // a monster drawn this large or larger gets the higher arc
// Flyers, hoverers and the tall or large, and their stronger kinds: the higher arc.
constexpr Monster kTallLeapers[] = {
    Monster::HORNET,
    Monster::WITCH_HELLZA,
    Monster::WITCH_HELLZA_ENHANCED,
    Monster::WITCH_ILLZA,
    Monster::GHOST,
    Monster::LICH,
    Monster::LICH_ENHANCED,
    Monster::PHANTOM,
    Monster::CAVE_BAT,
    Monster::CAVE_BAT_ENHANCED,
    Monster::EVIL_BAT,
    Monster::EVIL_BAT_ENHANCED,
    Monster::NIKAPOUS,
    Monster::CRESCENT_BARON,
    Monster::CRESCENT_BARON_ENHANCED,
    Monster::MASK_OF_PRAJNA,
    Monster::MASK_OF_PRAJNA_ENHANCED,
    Monster::ALEXANDER,
    Monster::ALEXANDER_ENHANCED,
    Monster::ARTHUR,
    Monster::ARTHUR_ENHANCED,
    Monster::BLACK_KNIGHT,
    Monster::BLACK_KNIGHT_MOUNT,
    Monster::DARK_GENIE,
    Monster::DARK_GENIE_RIGHT_HAND,
    Monster::DARK_GENIE_FINAL,
    Monster::MINOTAUR_JOE,
    Monster::KINGS_CURSE,
    Monster::KINGS_CURSE_COFFIN,
    Monster::MASTER_UTAN,
    Monster::DRAN,
    Monster::GOL,
    Monster::GOL_ENHANCED,
    Monster::SIL,
    Monster::SIL_ENHANCED,
    Monster::BLIZZARD,
    Monster::STEEL_GIANT,
    Monster::STEEL_GIANT_ENHANCED,
    Monster::TITAN,
    Monster::TITAN_ENHANCED,
    Monster::GOLEM,
    Monster::BLACK_DRAGON,
    Monster::BLUE_DRAGON,
    Monster::DRAGON,
    Monster::OPAR,
};

// The percentage of the cat's hit a sphere takes: the one it names for the cat's kick, else Xiao's.
int CatPercent(const MONSTOR_EFFECT_STATE &effect, int sphere) {
    return effect.body_parameter[sphere][1] == kSpiritBeastSphereKick ? effect.body_parameter[sphere][0] : effect.parameter[sphere][CHARA_XIAO];
}

} // namespace

bool Targetable(int monster) {
    const MONSTOR *live = FloorMonster(monster);
    return live != nullptr && MonsterLiving(*live);
}

bool Dormant(int monster) {
    if (!Targetable(monster)) {
        return false;
    }
    const MONSTOR &live = NowMonstorUnit->monster[monster];
    return (live.kind == MONSTER_KIND_MIMIC_SMALL || live.kind == MONSTER_KIND_MIMIC_LARGE) && live.state != 2;
}

bool Untouchable(int monster) {
    return Dormant(monster) || NowMonstorUnit->monster[monster].invincible_timer > 0;
}

const CCharacter &Body(int monster) {
    return NowMonstorUnit->chara[monster][0];
}

int LockOnCandidate() {
    return Targetable(lockOnTargetNo) ? lockOnTargetNo : -1;
}

int PickTarget() {
    if (NowMonstorUnit == nullptr) {
        return -1;
    }
    int locked = LockOnCandidate();
    if (locked >= 0) {
        return locked;
    }
    int   best = -1;
    float nearest = INFINITY;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        float d = std::hypot(Body(monster).pos[0] - CharaMain.pos[0], Body(monster).pos[2] - CharaMain.pos[2]);
        if (Targetable(monster) && d < nearest && d <= kSpiritBeastSight) {
            best = monster;
            nearest = d;
        }
    }
    return best;
}

void AimPoint(int monster, float *out) {
    const CCharacter           &body = Body(monster);
    const MONSTOR_EFFECT_STATE &effect = NowMonstorUnit->effect[monster];
    out[0] = body.pos[0], out[1] = body.pos[1], out[2] = body.pos[2], out[3] = 1.0f;
    if (NowMonstorUnit->monster[monster].state != 2) {
        return; // its spheres are not set up until it is in play
    }
    float fx = std::sin(body.rotation.y), fz = std::cos(body.rotation.y);
    float best = -1.0f, best_forward = -INFINITY;
    for (int pass = 0; pass < 2 && best < 0; pass++) {
        for (int sphere = 0; sphere < kMonsterSpheres; sphere++) {
            if (effect.timer[sphere] == 0 || (pass == 0 && CatPercent(effect, sphere) <= 0)) {
                continue;
            }
            const float *c = effect.position[sphere];
            float        r = effect.radius[sphere], forward = (c[0] - body.pos[0]) * fx + (c[2] - body.pos[2]) * fz;
            if (r > best + 0.01f || (std::fabs(r - best) <= 0.01f && forward > best_forward)) {
                best = r;
                best_forward = forward;
                out[0] = c[0], out[1] = c[1], out[2] = c[2];
            }
        }
    }
}

bool TallLeap(int monster) {
    for (Monster kind : kTallLeapers) {
        if (IsKind(*NowMonstorUnit, monster, kind)) {
            return true;
        }
    }
    return IsMiniboss(monster) || Body(monster).scale[0] >= kTallScale;
}

} // namespace expanded::divine_beast_title
