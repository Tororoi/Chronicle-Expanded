#include "expanded/weapons/ungaga/terra_sword.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/floor_height.hpp"
#include "expanded/dungeon/ground_shadow.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/status/confuse.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solid_column.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/ungaga/terra_sword/drop_hits.hpp"
#include "expanded/weapons/ungaga/terra_sword/shockwave.hpp"
#include "expanded/weapons/wear.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

using namespace terra_sword;

// The floor under a point: the highest at or below its height plus this.
constexpr float kFloorAbove = 20.0f;
// An hour: the nut's confusion is ended with the nut, not by time.
constexpr int kNutConfusionSteps = 60 * 60 * 60;

// What drops: its model, its size (of its own) and radius at that size, how far it sinks into the floor at rest, and
// how dark its monster gets.
struct Form {
    const char *name;
    const char *file;
    const char *cfg;
    float       scale;
    float       radius;
    float       sink;
    float       dim;
    bool        nut;
};

const Form kRock = {"boulder", kRockFile, kRockCfg, 1.0f, kRockRadius, kRockSink, kRockTargetDim, false};
const Form kNut = {"nut", kNutFile, kNutCfg, kNutScale, kNutRadius, kNutSink, kNutTargetDim, true};

// The nut's hop off the head, worked out as it falls: a frame's rise (below 0) and drift, and where it comes down.
struct Hop {
    float speed = 0.0f;
    float drift[2] = {};
    float stop = 0.0f;
    float ground = 0.0f;
};

struct Drop {
    bool  up = false;
    bool  falling = false;
    bool  hopping = false;
    bool  head = false;   // the nut's fall stops on its monster's head, the hop off it armed
    int   target = -1;    // the monster it follows down; -1 once it falls where it is
    int   dimmed = -1;    // the monster darkened under it
    int   bonked = -1;    // the monster the nut confused
    float at[3] = {};     // x, its centre's height, z
    float ground = 0.0f;  // the floor under it
    float yaw = 0.0f;     // the wielder's facing at the drop
    float start = 0.0f;   // its centre's height as it starts
    float span = 1.0f;    // from there to its rest
    float speed = 0.0f;   // falling this much a frame (below 0, rising)
    float gravity = 0.0f; // what that gains each frame
    float stop = 0.0f;    // the height it comes down at
    float head_lift = 0.0f;
    Hop   hop;
    float hop_side = 1.0f;
    int   aimed = 0; // frames since the hop was worked out
    float scale = 0.0f;
    float shadow = 0.0f;
    float alpha = 1.0f;
    int   age = -1; // frames at rest
};

struct State {
    BigRockPhase phase = BigRockPhase::kIdle;
    int          chara = -1; // the wielder
    const Form  *form = nullptr;
    GuardHold    guard;
    int          charge_from = 0; // the guard's frames as the charge started
    float        green = 0.0f;
    int          green_fade = -1; // frames the green has faded, -1 when it is not fading
    Drop         drop;
    CCharacter  *model = nullptr; // the form's, entered on this floor
};

State state;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(BigRockReset), true);

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_TERRA_SWORD, WeaponShotForm::Weapon, ShockwaveEffect(), kShockShots, kShockFile),
     RegisterOwnSound(kRockCrashSound), RegisterOwnSound(kNutBonkSound), true);

const Form &Shape() {
    return *state.form;
}

CMonstorUnit &Unit() {
    return *NowMonstorUnit;
}

// The form's model, entered on this floor; null when it cannot be.
CCharacter *Model() {
    if (state.model == nullptr) {
        state.model = EnterFloorCharacter(Shape().file, Shape().cfg);
        if (state.model == nullptr || state.model->frame == nullptr) {
            std::fprintf(stderr, "big rock: no %s, %s could not be loaded\n", Shape().name, Shape().file);
            state.model = nullptr;
        }
    }
    return state.model;
}

// The drop's centre at rest: its radius over the floor, less its sink.
float RestHeight() {
    return state.drop.ground + Shape().radius - Shape().sink;
}

void TargetAt(float *at) {
    Unit().chara[state.drop.target][0].GetPosition(at);
}

// Over its monster's root, and the floor under it (its root's height where there is none).
void TargetGround() {
    Drop &d = state.drop;
    float at[4];
    TargetAt(at);
    d.at[0] = at[0];
    d.at[2] = at[2];
    if (!FloorUnder(at[0], at[2], at[1] + kFloorAbove, &d.ground)) {
        d.ground = at[1];
    }
}

// The monster's own light eased down under the drop (the game's step eases it 0.08 a frame toward its floor).
void DimStart(int monster) {
    CCharacter &chara = Unit().chara[monster][0];
    chara.fade[3] = 1.0f;
    chara.fade_out = 1;
    state.drop.dimmed = monster;
}

void DimEnd() {
    Drop &d = state.drop;
    if (d.dimmed < 0 || NowMonstorUnit == nullptr) {
        d.dimmed = -1;
        return;
    }
    CCharacter &chara = Unit().chara[d.dimmed][0];
    chara.fade_out = 0;
    chara.fade[3] = 1.0f;
    d.dimmed = -1;
}

// The nut's hop off the head: up, drifting square to the wielder's line to the monster, down onto the floor where it
// comes out, worked out from where the monster is now.
void AimHop() {
    Drop &d = state.drop;
    d.aimed = 0;
    float at[4];
    TargetAt(at);
    float head = at[1] + d.head_lift;
    float lx = at[0] - CharaMain.pos[0], lz = at[2] - CharaMain.pos[2], length = std::hypot(lx, lz);
    if (length < 1e-3f) {
        lx = 1.0f, lz = 0.0f, length = 1.0f;
    }
    float dx = -lz / length * kNutHopSide * d.hop_side, dz = lx / length * kNutHopSide * d.hop_side;
    float rest = RestHeight();
    float frames = (kNutHopUp + std::sqrt(std::max(0.0f, kNutHopUp * kNutHopUp + 2.0f * kNutHopGravity * (head - rest)))) / kNutHopGravity;
    if (!FloorUnder(at[0] + dx * frames, at[2] + dz * frames, head + kFloorAbove, &d.hop.ground)) {
        d.hop.ground = d.ground;
    }
    d.hop.speed = -kNutHopUp;
    d.hop.drift[0] = dx;
    d.hop.drift[1] = dz;
    d.hop.stop = d.hop.ground + Shape().radius - Shape().sink;
}

void Place() {
    Drop &d = state.drop;
    if (state.model == nullptr) {
        return;
    }
    state.model->SetPosition(d.at[0], d.at[1], d.at[2]);
    state.model->SetRotation(0.0f, d.yaw, 0.0f);
    state.model->SetScale(d.scale, d.scale, d.scale);
    state.model->ambient_offset[3] = 128.0f * std::clamp(d.alpha, 0.0f, 1.0f);
}

void Shadow(bool on, float radius) {
    Drop &d = state.drop;
    if (on) {
        GroundShadowShow(d.at[0], d.ground, d.at[2], radius, &state);
    } else {
        GroundShadowHide(&state);
    }
}

// Solid where it rests: its radius, up to its top.
void Solid() {
    Drop &d = state.drop;
    float base[3] = {d.at[0], d.ground, d.at[2]};
    SolidColumnArm(base, Shape().radius, RestHeight() + Shape().radius, &state);
}

void Rest() {
    state.drop.age = 0;
    Solid();
    LogEvent("big_rock", std::format("\"step\":\"rest\",\"form\":\"{}\"", Shape().name));
}

void TakeDown() {
    Drop &d = state.drop;
    if (!d.up) {
        return;
    }
    SolidColumnDisarm(&state);
    GroundShadowHide(&state);
    DimEnd();
    if (d.bonked >= 0) {
        ConfuseEnd(d.bonked); // its monster's confusion goes with the nut, any other's stays
    }
    d = Drop{};
    std::fprintf(stderr, "big rock: the %s is gone\n", Shape().name);
    LogEvent("big_rock", std::format("\"step\":\"gone\",\"form\":\"{}\"", Shape().name));
}

void DropOn(int monster) {
    TakeDown();
    if (Model() == nullptr) {
        state.green_fade = 0;
        return;
    }
    Drop &d = state.drop;
    d = Drop{};
    d.target = monster;
    TargetGround();
    d.yaw = PlayerYaw();
    d.start = RestHeight() + kDropHeight + Shape().sink; // its bottom kDropHeight over the floor
    d.at[1] = d.start;
    d.span = std::max(1.0f, d.start - RestHeight());
    d.gravity = kDropGravity;
    d.stop = RestHeight();
    d.scale = kDropLeast * Shape().scale;
    d.shadow = kDropLeast * Shape().radius;
    d.up = d.falling = true;
    DimStart(monster);
    if (Shape().nut) {
        // It lands on the head, the frame it reaches it: the hop off it armed from the start.
        d.head = true;
        d.head_lift = MonsterHeight(Unit(), monster) + Shape().radius;
        d.hop_side = ModRoll(2) == 0 ? -1.0f : 1.0f;
        AimHop();
    }
    Place();
    std::fprintf(stderr, "big rock: the %s drops on monster %d at (%.0f, %.0f), floor %.1f\n", Shape().name, monster, d.at[0], d.at[2], d.ground);
    LogEvent("big_rock", std::format("\"step\":\"drop\",\"form\":\"{}\",\"monster\":{}", Shape().name, monster));
}

// Its size, its shadow's and its monster's darkness, from how far it has fallen.
void Rows() {
    Drop &d = state.drop;
    float fallen = d.start - d.at[1];
    d.scale = std::clamp(Shape().scale * fallen / kDropGrowDrop, kDropLeast * Shape().scale, Shape().scale);
    d.shadow = std::clamp(Shape().radius * fallen / kShadowGrowDrop, kDropLeast * Shape().radius, Shape().radius);
    if (d.dimmed >= 0) {
        float dark = 1.0f - Shape().dim;
        Unit().chara[d.dimmed][0].fade[3] = std::clamp(1.0f - dark * fallen / d.span, Shape().dim, 1.0f);
    }
}

// The fall ended on the floor: the boulder's landing, or the nut at rest with no head under it.
void Impact() {
    Drop &d = state.drop;
    d.falling = false;
    d.at[1] = RestHeight();
    d.scale = Shape().scale;
    DimEnd();
    state.green_fade = 0;
    if (Shape().nut) {
        Rest();
        std::fprintf(stderr, "big rock: the nut lands at (%.0f, %.0f, %.0f), no head under it\n", d.at[0], d.ground, d.at[2]);
        return;
    }
    d.age = 0;
    Solid();
    float spot[3] = {d.at[0], d.ground, d.at[2]};
    int   hit = RockBlast(Unit(), spot);
    ShockwavePlay(spot);
    PlayOwnSound(kRockCrashSound, true);
    RumbleBlast(BlastRumble::kKnockdown);
    WearWeapon(kRockWhp);
    std::fprintf(stderr, "big rock: impact at (%.0f, %.0f, %.0f) on %d monsters\n", spot[0], spot[1], spot[2], hit);
    LogEvent("big_rock", std::format("\"step\":\"impact\",\"monsters\":{}", hit));
}

// The nut came down on the head and the hop took it that frame: the hop's own gravity, the bonk, the monster confused.
void Bonk() {
    Drop &d = state.drop;
    int   monster = d.target;
    d.falling = false;
    d.hopping = true;
    d.head = false;
    d.target = -1;
    d.gravity = kNutHopGravity;
    d.ground = d.hop.ground;
    d.scale = Shape().scale;
    DimEnd();
    state.green_fade = 0;
    if (monster < 0 || !MonsterLiving(Unit().monster[monster])) {
        return;
    }
    NutBonk(Unit(), monster);
    PlayOwnSound(kNutBonkSound, false);
    ConfuseMonster(Unit(), monster, kNutConfusionSteps);
    d.bonked = monster;
    LogEvent("big_rock", std::format("\"step\":\"bonk\",\"monster\":{}", monster));
}

// A frame of the fall, after its monster (its floor the boulder's stop, its head the nut's; gone, it falls where it
// is).
void Fall() {
    Drop &d = state.drop;
    if (d.target >= 0) {
        if (MonsterLiving(Unit().monster[d.target])) {
            TargetGround();
            if (!d.head) {
                d.stop = RestHeight();
            } else if (++d.aimed >= kNutHopAimFrames) {
                AimHop();
            }
        } else {
            d.target = -1;
            d.head = false;
            d.stop = RestHeight();
        }
    }
    if (d.head) {
        float at[4];
        TargetAt(at);
        d.stop = at[1] + d.head_lift;
    }
    d.speed += d.gravity;
    d.at[1] -= d.speed;
    bool down = d.at[1] <= d.stop;
    if (down) {
        d.at[1] = d.stop;
    }
    Rows();
    if (down && d.head) {
        // The hop, this very frame: its rise and drift, where it comes down.
        d.speed = d.hop.speed;
        d.stop = d.hop.stop;
        d.at[0] += d.hop.drift[0];
        d.at[2] += d.hop.drift[1];
        Bonk();
        Shadow(true, Shape().radius);
        return;
    }
    if (down) {
        Impact();
        return;
    }
    Shadow(true, d.shadow);
}

// A frame of the nut's hop: its drift, its rise and fall, to rest where it comes down.
void HopStep() {
    Drop &d = state.drop;
    d.speed += d.gravity;
    d.at[1] -= d.speed;
    d.at[0] += d.hop.drift[0];
    d.at[2] += d.hop.drift[1];
    if (d.at[1] <= d.stop) {
        d.at[1] = d.stop;
        d.hopping = false;
        Rest();
        std::fprintf(stderr, "big rock: the nut comes to rest at (%.0f, %.0f, %.0f)\n", d.at[0], d.ground, d.at[2]);
        return;
    }
    Shadow(true, Shape().radius);
}

// A frame at rest: solid, its shadow, the boulder's pinned hits, then its fade.
void RestStep() {
    Drop &d = state.drop;
    d.age++;
    if (d.age >= kDropRestFrames + kDropFadeFrames) {
        TakeDown();
        return;
    }
    d.alpha = d.age < kDropRestFrames ? 1.0f : std::max(0.0f, 1.0f - static_cast<float>(d.age - kDropRestFrames) / kDropFadeFrames);
    Shadow(d.age < kDropRestFrames, Shape().radius);
    if (!Shape().nut && d.age % kRockPinFrames == 0) {
        float spot[3] = {d.at[0], d.ground, d.at[2]};
        RockPin(Unit(), spot);
    }
}

void DriveDrop() {
    Drop &d = state.drop;
    if (d.falling) {
        Fall();
    } else if (d.hopping) {
        HopStep();
    } else {
        RestStep();
    }
    Place();
}

// The green after the fall: held while it falls, then faded out.
void FadeGreen() {
    if (state.drop.falling) {
        state.green = 1.0f;
        return;
    }
    if (state.green_fade < 0) {
        return;
    }
    state.green_fade++;
    if (state.green_fade >= kRockGreenFadeFrames) {
        state.green = 0.0f;
        state.green_fade = -1;
        return;
    }
    state.green = 1.0f - static_cast<float>(state.green_fade) / kRockGreenFadeFrames;
}

void Charge() {
    state.guard.Step(GamePad.On(PAD_R1) != 0);
    switch (state.phase) {
        case BigRockPhase::kIdle:
            FadeGreen();
            // The charge counts from here, even for a guard held while a drop was up.
            if (state.guard.Held() >= 0 && !state.drop.up) {
                state.phase = BigRockPhase::kCharging;
                state.charge_from = state.guard.Held();
                state.green_fade = -1;
                std::fprintf(stderr, "big rock: charging\n");
                LogEvent("big_rock", "\"step\":\"charging\"");
            }
            break;

        case BigRockPhase::kCharging: {
            if (state.guard.Held() < 0) {
                state.phase = BigRockPhase::kIdle;
                state.green = 0.0f;
                std::fprintf(stderr, "big rock: the guard was let go before the charge was full\n");
                break;
            }
            state.green = std::min(1.0f, static_cast<float>(state.guard.Held() - state.charge_from) / kRockChargeFrames);
            if (state.green >= 1.0f) {
                state.phase = BigRockPhase::kPrimed;
                ChargeCompleteFlash();
                Model(); // loaded now, not as it drops
                std::fprintf(stderr, "big rock: primed\n");
                LogEvent("big_rock", std::format("\"step\":\"primed\",\"form\":\"{}\"", Shape().name));
            }
            break;
        }

        case BigRockPhase::kPrimed: {
            state.green = 1.0f;
            int target = LockedTarget();
            if (target >= 0) {
                state.phase = BigRockPhase::kIdle;
                DropOn(target); // the green holds until the fall ends
            }
            break;
        }
    }
}

void End() {
    ShockwaveStop();
    TakeDown();
    DropHitsReset();
    state = State{};
}

bool Active() {
    return NowMonstorUnit != nullptr && (Playing(CHARA_UNGAGA) || Playing(CHARA_XIAO)) && WeaponActive(ITEM_WEAPON_TERRA_SWORD);
}

} // namespace

BigRockPhase BigRockState() {
    return state.phase;
}

float BigRockGreen() {
    return state.green;
}

bool BigRockUp() {
    return state.drop.up;
}

bool BigRockFalling() {
    return state.drop.falling;
}

void BigRockAt(float *at) {
    std::copy(state.drop.at, state.drop.at + 3, at);
}

float BigRockGround() {
    return state.drop.ground;
}

float BigRockScale() {
    return state.drop.up ? state.drop.scale : 0.0f;
}

float BigRockAlpha() {
    return state.drop.up ? state.drop.alpha : 0.0f;
}

int BigRockRestAge() {
    return state.drop.up ? state.drop.age : -1;
}

bool BigRockHit(const COLLISION_HIT &hit) {
    return DropHit(hit);
}

BT_SHOT_EFFECT *BigRockShockwave() {
    return ShockwaveEffect();
}

void BigRockStep() {
    bool active = Active();
    if (state.chara >= 0 && (!active || UserStatus->cur_chara != state.chara)) {
        End(); // a change of wielder ends it: the next one's starts afresh
    }
    if (!active || !MonstersStepping()) {
        return;
    }
    if (state.chara < 0) {
        state.chara = UserStatus->cur_chara;
        state.form = state.chara == CHARA_XIAO ? &kNut : &kRock;
    }
    DropHitsStep(Unit());
    Charge();
    if (state.drop.up) {
        DriveDrop();
    }
    ShockwaveStep();
}

void BigRockDraw() {
    if (state.drop.up && state.model != nullptr && BtEventMode == 0) {
        DrawFloorCharacter(*state.model);
    }
}

void BigRockHitTaken(const CMonstorUnit &, int monster, const COLLISION_HIT &hit) {
    DropHitTaken(monster, hit);
}

void BigRockLight(const CCharacter &character, float *ambient) {
    if (state.green <= 0.0f || &character != NowWeapon) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += kRockGreen[c] * state.green;
    }
}

void BigRockReset() {
    ShockwaveReset();
    SolidColumnDisarm(&state);
    GroundShadowHide(&state);
    DropHitsReset();
    state = State{};
}

} // namespace expanded
