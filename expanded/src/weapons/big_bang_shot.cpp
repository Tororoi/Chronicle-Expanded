#include "expanded/weapons/big_bang_shot.hpp"

#include <algorithm>
#include <cmath>

#include "bt_shot_effect.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/glow_sprite.hpp"
#include "expanded/dungeon/scene_light.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/status/blind.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/pellets.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/solar_flash.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/wear.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "userstatus.hpp"


namespace expanded {

namespace {

constexpr float kFrameSeconds = 1.0f / 60.0f;
constexpr float kGravity = 500.0f;
constexpr int   kBombSlots = 6;
constexpr float kBombHeight = 1.22f;   // the bomb's mesh below its root, at its own size
constexpr float kDropExplosion = 2.0f; // the explosions, of the game's bomb's size
constexpr float kDropLift = 2.0f;      // the drop's explosion this far over the landing
constexpr float kChargedExplosion = 1.25f;
constexpr float kDropRing = 70.0f;     // the drop's shock ring
constexpr float kChargedRing = 45.0f;  // the charged shot's shock ring, out past its blast's 25
constexpr float kBombGlowPull = 14.0f; // the hanging bomb's glow over its front, not cut by it
constexpr float kPlainExplosion = 0.5f;
constexpr float kSlingWhite = 200.0f;
constexpr float kPrimedWhite = 45.0f;
constexpr int   kDropFrames = 20; // about how long the fall takes, for the room's darkening along it
// The hanging bomb's red, held while it fades in, then a cosine down to black and back once a second.
constexpr float kBombRed = 150.0f;
constexpr float kPulseStep = 2.0f * 3.14159265f / 60.0f;

// The bomb alone: Witch Illza's apple shot drawing the bomb (the Bomb Gemron's throw), at no speed, put where it is
// wanted, no hit.
// clang-format off
BT_SHOT_EFFECT bomb_shot = {"ringo_ex", 0, 0, {}, {}, -1, 0, 0, 3, 0, {-1, 0, -1, -1}, 0, {0, 0}, 1.0f, 0, {-1, -1, -1, -1}};
// clang-format on

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_BIG_BANG, WeaponShotForm::Sphere, &bomb_shot, kBombSlots, kBombThrowFile), true);

enum class Charge {
    kIdle,
    kCharging,
    kPrimed,
    kFalling,
    kFlashing, // the room plunging before a flash from where she stood
};

struct Flight {
    const CSHOT *shots = nullptr;
    int          pellet = -1;
    int          bomb = -1;
    float        scale = 1.0f;
    bool         charged = false;
};

struct State {
    int    effect = -1; // the bomb's effect, as last asked
    Charge charge = Charge::kIdle;
    int    frames = 0; // in the phase
    int    charged = 0;
    int    unguarded = 0;
    ShotCharge draw{kChargedShotFrames}; // the slingshot drawn and held
    int    target = -1;
    int    hover = -1; // the hanging bomb's slot
    float  hover_pos[4] = {};
    float  ground = 0.0f;
    float  speed = 0.0f;
    float  shown = 0.0f; // the hanging bomb, 0..1
    int    pulse = 0;    // frames it has hung fully in
    float  flash_at[4] = {};
    Flight flights[kPelletSlots];
};

State state;

void OnFloor() {
    state = {};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

// The bomb's effect, loaded with the floor while the sphere is with the party (asked each time, never loading).
CSHOT_EFFECT *Bombs() {
    state.effect = ShotEffectIndex(&bomb_shot);
    return state.effect >= 0 ? ShotEffectAt(state.effect) : nullptr;
}

int ShowBomb(const float *at, float scale, bool still) {
    CSHOT_EFFECT *bombs = Bombs();
    if (bombs == nullptr) {
        return -1;
    }
    // A shot that does not fly: aimed straight up at no speed (Set lets it go of the model it was copied from).
    sceVu0FVECTOR from, to;
    sceVu0CopyVector(from, const_cast<float *>(at));
    from[3] = 1.0f;
    sceVu0CopyVector(to, from);
    to[1] += 1.0f;
    bombs->Set(from, to, 1, -1, 0, nullptr, 1);
    int slot = bombs->current_slot;
    if (slot < 0) {
        return -1;
    }
    bombs->velocity[slot][0] = bombs->velocity[slot][1] = bombs->velocity[slot][2] = 0.0f;
    bombs->chara[slot].SetScale(scale, scale, scale);
    bombs->chara[slot].ambient_offset[3] = -1.0f;
    // The apple's flight motion turns it; a hanging bomb holds still.
    bombs->chara[slot].motion_flags = still ? bombs->chara[slot].motion_flags | 1 : bombs->chara[slot].motion_flags & ~1;
    return slot;
}

void MoveBomb(int slot, const float *at, float scale) {
    CSHOT_EFFECT *bombs = state.effect >= 0 ? ShotEffectAt(state.effect) : nullptr;
    if (bombs == nullptr || slot < 0 || bombs->active[slot] == 0) {
        return;
    }
    bombs->chara[slot].SetPosition(const_cast<float *>(at));
    bombs->chara[slot].SetScale(scale, scale, scale);
}

void HideBomb(int &slot) {
    CSHOT_EFFECT *bombs = state.effect >= 0 ? ShotEffectAt(state.effect) : nullptr;
    if (bombs != nullptr && slot >= 0) {
        bombs->active[slot] = 0;
        bombs->chara[slot].ambient_offset[3] = -1.0f;
        bombs->chara[slot].motion_flags &= ~1;
    }
    slot = -1;
}

void Explode(const float *point, float explosion, float blast, float ring, float whp) {
    if (NowMonstorUnit != nullptr && NowWeaponHave != nullptr && blast > 0.0f) {
        BlastOptions options;
        options.damage = options.reach = options.kick = blast;
        options.guardable = true; // only the drop's blast passes a guard
        PlantBlast(*NowMonstorUnit, point, NowWeaponHave->attack, options);
    }
    ExplosionAt(point, explosion, ring, BlastSound::kSteve);
    WearWeapon(whp);
}

// Where the hanging bomb's root goes over the target: its bottom kHoverMargin over the target's height.
void HoverOver(int target) {
    sceVu0FVECTOR at;
    NowMonstorUnit->chara[target][0].GetPosition(at);
    state.ground = at[1];
    state.hover_pos[0] = at[0];
    state.hover_pos[1] = at[1] + MonsterHeight(*NowMonstorUnit, target) + kHoverMargin + kBombHeight * kHoverBomb;
    state.hover_pos[2] = at[2];
    state.hover_pos[3] = 1.0f;
}

// Locked on, the bomb over the target (taken over by a new one at once); else none.
void Hang() {
    int target = LockedTarget();
    if (target < 0) {
        HideBomb(state.hover);
        state.target = -1;
        state.shown = 0.0f;
        return;
    }
    HoverOver(target);
    if (state.hover < 0 || state.target != target) {
        HideBomb(state.hover);
        state.hover = ShowBomb(state.hover_pos, kHoverBomb, true);
        state.target = target;
    }
    MoveBomb(state.hover, state.hover_pos, kHoverBomb);
}

void Spend() {
    HideBomb(state.hover);
    state.charge = Charge::kIdle;
    state.target = -1;
    state.shown = 0.0f;
    state.frames = 0;
}

void Land() {
    sceVu0FVECTOR point = {state.hover_pos[0], state.ground, state.hover_pos[2], 1.0f};
    Spend();
    if (NowMonstorUnit != nullptr && NowWeaponHave != nullptr) {
        PlantBlast(*NowMonstorUnit, point, NowWeaponHave->attack);
    }
    sceVu0FVECTOR burst = {point[0], point[1] + kDropLift, point[2], 1.0f};
    ExplosionAt(burst, kDropExplosion, kDropRing, BlastSound::kSteve);
    SolarBlastFlash(kBigBangFlash, point, kBombDropWhp);
}

} // namespace

bool BigBangShotActive() {
    return UserStatus != nullptr && UserStatus->cur_chara == CHARA_XIAO && WeaponWielded(ITEM_WEAPON_SUPER_STEVE) &&
           SuperSteveSphere() == ITEM_WEAPON_BIG_BANG;
}

bool BigBangShotHanging() {
    return state.hover >= 0;
}

void BigBangShotStep() {
    if (!BigBangShotActive()) {
        if (state.charge != Charge::kIdle) {
            Spend();
        }
        return;
    }
    Bombs();
    // The bombs in the air ride their pellets.
    for (Flight &flight : state.flights) {
        if (flight.shots != nullptr && flight.bomb >= 0) {
            MoveBomb(flight.bomb, flight.shots->pos[flight.pellet], flight.scale);
        }
    }
    // How long the slingshot was last held drawn: the shot it lets go of reads it.
    state.draw.Step(true);
    state.frames++;

    switch (state.charge) {
        case Charge::kIdle:
            if (Guarding() && !Blinding()) {
                state.charge = Charge::kCharging;
                state.charged = state.unguarded = state.frames = 0;
            }
            break;
        case Charge::kCharging:
            state.unguarded = Guarding() ? 0 : state.unguarded + 1;
            if (state.unguarded > kGuardGrace || Blinding()) {
                Spend();
                break;
            }
            if (state.unguarded == 0) {
                state.charged++;
            }
            // The bomb fades in over the target with the charge.
            Hang();
            state.shown = std::min(1.0f, static_cast<float>(state.charged) / kBombChargeFrames);
            if (state.charged >= kBombChargeFrames) {
                ChargeCompleteFlash();
                state.charge = Charge::kPrimed;
                state.frames = state.pulse = 0;
            }
            break;
        case Charge::kPrimed:
            Hang();
            state.shown = state.hover >= 0 ? 1.0f : 0.0f;
            state.pulse++;
            if (state.frames >= kBombPrimedFrames) {
                Spend();
            }
            break;
        case Charge::kFalling: {
            if (state.target >= 0 && NowMonstorUnit != nullptr && NowMonstorUnit->monster[state.target].hp > 0) {
                sceVu0FVECTOR at;
                NowMonstorUnit->chara[state.target][0].GetPosition(at);
                state.hover_pos[0] = at[0];
                state.hover_pos[2] = at[2];
                state.ground = at[1];
            }
            // The room darkens along the fall, to black at the landing.
            SceneDimRamp(0.0f, std::min(1.0f, static_cast<float>(state.frames) / kDropFrames));
            state.speed += kGravity * kFrameSeconds * kFrameSeconds;
            state.hover_pos[1] -= state.speed;
            float stop = state.ground + kBombHeight * kHoverBomb;
            if (state.hover_pos[1] <= stop) {
                state.hover_pos[1] = stop;
                Land();
            } else {
                MoveBomb(state.hover, state.hover_pos, kHoverBomb);
            }
            break;
        }
        case Charge::kFlashing:
            SceneDimRamp(0.0f, static_cast<float>(state.frames) / kRampFrames);
            if (state.frames >= kRampFrames) {
                state.charge = Charge::kIdle;
                SolarFlashAt(kBigBangFlash, state.flash_at, -1, kBombFlashWhp);
            }
            break;
    }
}

void BigBangShotReset() {
    state = {};
}

void BigBangShotFired(CSHOT &shots, int slot) {
    if (!BigBangShotActive()) {
        return;
    }
    if (state.charge == Charge::kPrimed) {
        // The charge's shot: it lets the hanging bomb go, or with nothing hanging is the flash from where she stands.
        shots.size[slot] = 0.0f;
        shots.damage[slot] = 0;
        shots.life[slot] = 1;
        state.frames = 0;
        if (state.hover >= 0) {
            state.charge = Charge::kFalling;
            state.speed = 0.0f;
        } else {
            sceVu0CopyVector(state.flash_at, CharaMain.pos);
            state.charge = Charge::kFlashing;
        }
        return;
    }
    Flight &flight = state.flights[slot];
    HideBomb(flight.bomb);
    flight.shots = &shots;
    flight.pellet = slot;
    flight.charged = state.draw.Ready();
    state.draw.Reset();
    flight.scale = flight.charged ? kChargedBomb : kPlainBomb;
    flight.bomb = ShowBomb(shots.pos[slot], flight.scale, false);
    if (flight.bomb >= 0) {
        shots.size[slot] = 0.0f; // the bomb is the pellet
    }
    if (flight.charged) {
        shots.damage[slot] = 0; // the blast is its damage
    }
}

void BigBangShotEnded(const CSHOT &shots, int slot, const float *point) {
    Flight &flight = state.flights[slot];
    if (flight.shots != &shots || flight.pellet != slot) {
        return;
    }
    HideBomb(flight.bomb);
    if (flight.charged) {
        Explode(point, kChargedExplosion, kChargedBlast, kChargedRing, kChargedShotWhp);
    } else {
        ExplosionAt(point, kPlainExplosion);
        RumbleBlast(BlastRumble::kLight);
    }
    flight = {};
}

void BigBangShotLight(const CCharacter &character, float *ambient) {
    if (!BigBangShotActive()) {
        return;
    }
    // The hanging bomb: red while it fades in, then a cosine to black and back once a second.
    CSHOT_EFFECT *bombs = state.hover >= 0 && state.effect >= 0 ? ShotEffectAt(state.effect) : nullptr;
    if (bombs != nullptr && &character == &bombs->chara[state.hover]) {
        float red = state.charge == Charge::kPrimed ? kBombRed * (0.5f + 0.5f * std::cos(state.pulse * kPulseStep)) : kBombRed;
        ambient[0] += red;
        bombs->chara[state.hover].ambient_offset[3] = 128.0f * state.shown;
        return;
    }
    if (state.charge != Charge::kCharging && state.charge != Charge::kPrimed) {
        return;
    }
    float k = state.charge == Charge::kCharging ? static_cast<float>(state.charged) / kBombChargeFrames : 1.0f;
    float add = &character == NowWeapon ? kSlingWhite * k : &character == &CharaMain ? kPrimedWhite * k
                                                                                     : 0.0f;
    for (int c = 0; c < 3; c++) {
        ambient[c] += add;
    }
}

void BigBangShotDraw() {
    if (state.hover < 0 || state.shown <= 0.0f) {
        return;
    }
    DrawGlowDisc(GlowLook::kFire, state.hover_pos, kBombGlowSize * state.shown, kBombGlowPull);
}

bool BigBangShotBlasts(const CSHOT &shots, int slot) {
    if (slot < 0 || slot >= kPelletSlots) {
        return false;
    }
    const Flight &flight = state.flights[slot];
    return flight.shots == &shots && flight.pellet == slot && flight.charged;
}

} // namespace expanded
