#include "expanded/weapons/ungaga/hercules_wrath.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "bt_shot_effect.hpp"
#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/frames.hpp"
#include "expanded/dungeon/collision_hits.hpp"
#include "expanded/dungeon/floor_height.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/ungaga/mirage.hpp"
#include "expanded/weapons/ungaga/mirage/clone.hpp"
#include "expanded/weapons/wear.hpp"
#include "frame.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

constexpr int   kStopAndRestart = 6;   // motion flags: from the clip's first frame, held on its last
constexpr float kOwnRate = -1.0f;      // a clip at its own rate
constexpr float kClipLead = 0.5f;      // a clip is taken as over this short of its end, where its cursor stops
constexpr int   kClipEnded = 3;        // CCharacter::motion_state as a clip ends
constexpr char  kSpearTip[] = "dcol0"; // the spear's tip, 15.66 along it

// e508_ex.chr: played where it is put, no flight and no hit. Its first phase is its third clip, whose end the game ends
// it on; the strike plays the first two in its place, so the game never does.
// clang-format off
BT_SHOT_EFFECT sparkle = {"e508_ex", 0, 0, {}, {}, -1, 0, 0, 3, kHitsMonsters, {2, -1, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_HERCULES_WRATH, WeaponShotForm::Weapon, &sparkle, kSparkleShots, kSparkleFile),
     RegisterWeaponShotEffect(ITEM_WEAPON_HERCULES_WRATH, WeaponShotForm::Sphere, &sparkle, kSparkleShots, kSparkleFile),
     RegisterOwnSound(kAirStrikeSound), RegisterOwnSound(kAirStrikeTwinkle), true);

struct State {
    AirStrikePhase phase = AirStrikePhase::kIdle;
    int            chara = -1;    // the wielder
    GuardHold      guard;         // latched as a charge ends with R1 still down: no other until it is let go
    bool           swung = false; // this swing has called the strike, or come past its frame
    float          gold = 0.0f;
    float          dim = 0.0f;  // the room's dark from the charge
    int            release = 0; // frames left of the dark and the gold easing back after a charge let go
    float          release_dim = 0.0f;
    float          release_gold = 0.0f;
    float          place[4] = {};   // the decoy's spot (the last one's once it has gone)
    float          landed[4] = {};  // where Xiao's primed pellet is, then where it ended
    float          ground = 0.0f;   // the floor under that
    const CSHOT   *shots = nullptr; // her primed pellet while it flies
    int            pellet = -1;
    int            flown = 0;
    CSHOT_EFFECT  *effect = nullptr; // the sparkle playing
    int            slot = -1;
    int            clip = -1;
    int            sound_in = 0; // frames until the light's sound, or 0 once it has gone off
    float          spot[2] = {}; // where its second clip plays, across the ground
    bool           blasted = false;
};

State state;

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(AirStrikeReset), true);

bool Xiao() {
    return state.chara == CHARA_XIAO;
}

void Place() {
    if (MirageDecoyUp()) {
        MirageDecoyPlace(state.place);
    }
}

// The strike's ground: the floor under Xiao's pellet, or the decoy's own height.
float Ground() {
    return Xiao() ? state.ground : state.place[1];
}

// Where the sparkle's first clip plays: where Xiao's pellet ended, or the tip of the spear the decoy holds (its spot when
// the copy holds none).
void Tip(float *out) {
    const float *at = Xiao() ? state.landed : state.place;
    CCharacter  *held = Xiao() ? nullptr : mirage::CloneWeapon();
    CFrame      *tip = held != nullptr ? FrameNamed(held->frame, kSpearTip) : nullptr;
    float        world[4];
    if (tip != nullptr) {
        FrameOrigin(*tip, world);
        at = world;
    }
    std::copy(at, at + 3, out);
}

bool SparkleLive() {
    return state.effect != nullptr && state.slot >= 0 && state.effect->effect_data == &sparkle && state.effect->active[state.slot] != 0;
}

CCharacter &Model() {
    return state.effect->chara[state.slot];
}

void Clip(int key, float frame, float rate) {
    CCharacter &model = Model();
    model.SetMotion(key, kStopAndRestart, rate);
    model.motion_type.state.time = frame;
    state.clip = key;
}

// The sparkle at at, its first clip from its start; false when it is not loaded on this floor.
bool Burst(const float *at) {
    int index = ShotEffectIndex(&sparkle);
    if (index < 0) {
        return false;
    }
    // A shot that does not fly: aimed straight up at no speed.
    CSHOT_EFFECT *effect = ShotEffectAt(index);
    float         from[4] = {at[0], at[1], at[2], 1.0f};
    float         to[4] = {at[0], at[1] + 1.0f, at[2], 1.0f};
    effect->Set(from, to, 0, -1, 0, nullptr, 0);
    int slot = effect->current_slot;
    if (slot < 0) {
        return false;
    }
    state.effect = effect;
    state.slot = slot;
    effect->velocity[slot][0] = effect->velocity[slot][1] = effect->velocity[slot][2] = 0.0f;
    CCharacter &model = Model();
    model.SetScale(1.0f, 1.0f, 1.0f);
    FadeUnlitFrames(model.frame, 1.0f); // a slot an earlier sparkle faded, back to full
    Clip(0, kSparkleFirstStart, kOwnRate);
    PlayOwnSound(kAirStrikeTwinkle, false); // the glint
    return true;
}

// The strike over, or dropped: the sparkle put out and handed back at full, the gold gone; no other charge until R1 is
// let go.
void Finish() {
    if (SparkleLive()) {
        state.effect->active[state.slot] = 0;
    }
    if (state.effect != nullptr && state.slot >= 0) {
        FadeUnlitFrames(Model().frame, 1.0f);
    }
    state.phase = AirStrikePhase::kIdle;
    state.effect = nullptr;
    state.slot = state.clip = -1;
    state.shots = nullptr;
    state.pellet = -1;
    state.blasted = false;
    state.gold = 0.0f;
    if (state.guard.Held() >= 0) {
        state.guard.Latch();
    }
}

// A charge or a strike dropped: the dark lifts with it, unless the strike's flash has taken over the light.
void Cancel(const char *why) {
    if (state.phase == AirStrikePhase::kIdle) {
        return;
    }
    if (state.phase != AirStrikePhase::kStriking || !state.blasted) {
        SceneDimEnd();
    }
    Finish();
    std::fprintf(stderr, "air strike: off (%s)\n", why);
}

// The blast where the sparkle's second clip plays (before it does, the decoy's spot or where Xiao's pellet ended), its
// bill, the decoy dispelled and the flash.
void Blast() {
    state.blasted = true;
    Place();
    float point[4] = {state.place[0], Ground(), state.place[2], 1.0f};
    if (state.clip >= 1) {
        point[0] = state.spot[0], point[2] = state.spot[1];
    } else if (Xiao()) {
        point[0] = state.landed[0], point[2] = state.landed[2];
    }
    int reached = 0;
    if (NowMonstorUnit != nullptr && NowWeaponHave != nullptr) {
        BlastOptions options;
        options.reach = kAirStrikeReach;
        options.rumble = false;
        reached = PlantBlast(*NowMonstorUnit, point, NowWeaponHave->attack, options);
    }
    WearWeapon(kAirStrikeWhp);
    MirageDispel();
    SceneFlash(kZeusFlash.look);
    std::fprintf(stderr, "air strike: blast at (%.1f, %.1f, %.1f) on %d monsters\n", point[0], point[1], point[2], reached);
}

// The strike called: the sparkle where its first clip plays, or the blast at once with the sparkle not loaded.
void Unleash() {
    Place();
    state.blasted = false;
    state.clip = -1;
    float tip[3];
    Tip(tip);
    if (Burst(tip)) {
        state.phase = AirStrikePhase::kStriking;
        std::fprintf(stderr, "air strike: unleashed at (%.1f, %.1f, %.1f)\n", tip[0], tip[1], tip[2]);
        return;
    }
    std::fprintf(stderr, "air strike: the sparkle is not loaded, the blast at once\n");
    Blast();
    Finish();
}

// Xiao's primed pellet ended at point: the strike there, its ground the floor under it.
void Strike(const float *point) {
    std::copy(point, point + 3, state.landed);
    state.shots = nullptr;
    state.pellet = -1;
    Place();
    if (!FloorUnder(state.landed[0], state.landed[2], state.landed[1] + kAirStrikeFloorAbove, &state.ground)) {
        state.ground = state.place[1];
    }
    std::fprintf(stderr, "air strike: the pellet ended at (%.1f, %.1f, %.1f), the floor under it at %.1f\n", state.landed[0], state.landed[1],
                 state.landed[2], state.ground);
    Unleash();
}

// One frame of the sparkle: its first clip riding the spear's tip, its second where the tip was as the first ended, the
// plunge to black and the blast along the second, then its fade with the gold.
void Play() {
    if (!SparkleLive()) {
        // Gone early: the blast is never skipped.
        if (!state.blasted) {
            Blast();
        }
        Finish();
        return;
    }
    Place();
    CCharacter &model = Model();
    if (state.clip == 0) {
        float tip[3];
        Tip(tip);
        model.SetPosition(tip[0], tip[1], tip[2]);
    } else {
        model.SetPosition(state.spot[0], Ground(), state.spot[1]);
    }
    float frame = model.motion_type.state.time;
    bool  ended = model.motion_state == kClipEnded;
    if (state.clip == 0) {
        if (frame >= kSparkleFirstEnd - kClipLead || ended) {
            float tip[3];
            Tip(tip);
            state.spot[0] = tip[0];
            state.spot[1] = tip[2];
            Clip(1, kSparkleSecondStart, kSparkleSecondRate);
            state.sound_in = kAirStrikeSoundDelayFrames;
        }
        return;
    }
    if (state.sound_in > 0 && --state.sound_in == 0) {
        PlayOwnSound(kAirStrikeSound, false); // the light comes down
    }
    model.motion_speed = kSparkleSecondRate;
    if (!state.blasted) {
        SceneDimRamp(kAirStrikeDim, std::clamp((frame - kSparkleDarkFrom) / (kSparkleBlastFrame - kSparkleDarkFrom), 0.0f, 1.0f));
        if (frame >= kSparkleBlastFrame) {
            Blast();
        }
    }
    float shown = 1.0f - std::clamp((frame - kSparkleFadeFrom) / (kSparkleFadeTo - kSparkleFadeFrom), 0.0f, 1.0f);
    state.gold = shown;
    if (frame >= kSparkleFadeFrom) {
        FadeUnlitFrames(model.frame, shown);
    }
    if (frame >= kSparkleFadeTo || frame >= kSparkleSecondEnd - kClipLead || ended) {
        if (!state.blasted) {
            Blast();
        }
        Finish();
    }
}

void GuardStep() {
    state.guard.Step(GamePad.On(PAD_R1) != 0);
}

// Ungaga's swing reaching the strike's frame, once a swing.
bool SwingReached() {
    int action = BtActStatus.action_no;
    if (action < kUngagaFirstSwing || action > kUngagaLastSwing) {
        state.swung = false;
        return false;
    }
    float cursor = CharaMain.motion_type.state.time;
    if (state.swung || cursor < kAirStrikeSwingFrame || cursor >= kAirStrikeSwingEnd) {
        return false;
    }
    state.swung = true;
    return true;
}

// A frame of the dark and the gold easing back after a charge was let go.
void Release() {
    state.release--;
    float k = static_cast<float>(state.release) / kAirStrikeReleaseFrames;
    state.gold = state.release_gold * k;
    if (state.release > 0) {
        SceneDim(state.release_dim * k);
    } else {
        SceneDimEnd();
    }
}

float GoldRamp(float charge) {
    return static_cast<float>((std::exp(kAirStrikeGoldCurve * charge) - 1.0) / (std::exp(kAirStrikeGoldCurve) - 1.0));
}

} // namespace

bool AirStrikeActive() {
    return UserStatus != nullptr && (UserStatus->cur_chara == CHARA_UNGAGA || UserStatus->cur_chara == CHARA_XIAO) &&
           WeaponActive(ITEM_WEAPON_HERCULES_WRATH);
}

AirStrikePhase AirStrikeState() {
    return state.phase;
}

float AirStrikeGold() {
    return state.gold;
}

BT_SHOT_EFFECT *AirStrikeSparkle() {
    return &sparkle;
}

void AirStrikeStep() {
    bool active = AirStrikeActive();
    if (state.chara >= 0 && (!active || UserStatus->cur_chara != state.chara)) {
        Cancel("the weapon was put away");
        SceneLightReset();
        state = State{};
    }
    if (!active) {
        return;
    }
    state.chara = UserStatus->cur_chara;
    GuardStep();
    bool swing = SwingReached();
    switch (state.phase) {
        case AirStrikePhase::kIdle:
            if (!state.guard.Latched() && state.guard.Held() >= kMirageGuardFrames) {
                state.release = 0;
                state.phase = AirStrikePhase::kCharging;
                std::fprintf(stderr, "air strike: charging\n");
            } else if (state.release > 0) {
                Release();
            }
            break;

        case AirStrikePhase::kCharging: {
            if (state.guard.Held() < 0) {
                // Let go before it charged: the dark and the gold ease back from where they are.
                float dim = state.dim, gold = state.gold;
                Finish();
                state.release = kAirStrikeReleaseFrames;
                state.release_dim = dim;
                state.release_gold = state.gold = gold;
                std::fprintf(stderr, "air strike: off (the guard was let go before the charge)\n");
                break;
            }
            float charge = std::min(1.0f, static_cast<float>(state.guard.Held() - kMirageGuardFrames) / kAirStrikeChargeFrames);
            state.dim = kAirStrikeDim * charge;
            SceneDim(state.dim);
            state.gold = GoldRamp(charge);
            if (charge >= 1.0f) {
                state.phase = AirStrikePhase::kPrimed;
                ChargeCompleteFlash();
                std::fprintf(stderr, "air strike: primed\n");
            }
            break;
        }

        case AirStrikePhase::kPrimed: {
            if (!MirageDecoyUp()) {
                Cancel("the decoy went before the strike");
                break;
            }
            // Held at full while the decoy stands, going as it dissolves.
            float left = MirageOutroAlpha();
            SceneDim(kAirStrikeDim * left);
            state.gold = left;
            if (!Xiao() && swing && left > 0.0f) {
                Unleash();
            }
            break;
        }

        case AirStrikePhase::kFlying:
            SceneDim(kAirStrikeDim);
            state.gold = 1.0f;
            // Ended unseen, or still out: the strike where it was last.
            if (state.shots->used[state.pellet] != 0) {
                std::copy(state.shots->pos[state.pellet], state.shots->pos[state.pellet] + 3, state.landed);
            }
            if (state.shots->used[state.pellet] == 0 || ++state.flown >= kAirStrikeMissFrames) {
                Strike(state.landed);
            }
            break;

        case AirStrikePhase::kStriking:
            Play();
            break;
    }
}

void AirStrikeFired(const CSHOT &shots, int slot) {
    if (state.phase != AirStrikePhase::kPrimed || !Xiao() || !AirStrikeActive() || MirageOutroAlpha() <= 0.0f) {
        return;
    }
    state.phase = AirStrikePhase::kFlying;
    state.shots = &shots;
    state.pellet = slot;
    state.flown = 0;
    std::copy(shots.pos[slot], shots.pos[slot] + 3, state.landed);
}

void AirStrikeEnded(const CSHOT &shots, int slot, const float *point) {
    if (state.phase == AirStrikePhase::kFlying && state.shots == &shots && state.pellet == slot) {
        Strike(point);
    }
}

void AirStrikeLight(const CCharacter &character, float *ambient) {
    if (state.gold <= 0.0f || (&character != NowWeapon && &character != mirage::CloneWeapon())) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        ambient[c] += kAirStrikeGold[c] * state.gold;
    }
}

void AirStrikeReset() {
    state = State{};
}

} // namespace expanded
