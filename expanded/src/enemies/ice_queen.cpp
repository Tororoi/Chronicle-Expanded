#include "expanded/enemies/ice_queen.hpp"

#include <cmath>

#include "dun/gameloop.hpp"
#include "expanded/core/random.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/boss_stats.hpp"
#include "expanded/enemies/bosses.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/ice_shots.hpp"
#include "expanded/enemies/normalize.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/scaling.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "snd.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {


// Her mailbox, as her script and the companions' use it (values past retail's mark a shot under way).
enum Mail {
    kPhase = 0,   // her state: 0 attack, 1 arrow flying, 3 player imprisoned, 4 meteor done, 5 barrier hit
    kBarrier = 1, // 1 raise the barrier, 2 broken
    kArrow = 2,   // 1 fire the arrow
    kPrison = 3,  // 1 imprison, 4 forming, 3 imprisoned, 2 shatter
    kMeteor = 4,  // 1 drop the meteor, 2 falling
    kAura = 5,    // 1 the aura's blast, 2 blasting
    kTornado = 6, // 6-8: 1 raise a tornado, 3 rising, 2 up
};

constexpr int kTornadoes = 3;

// The companions' packs, played as shots: model "info" names each pack's own info.cfg.
// clang-format off
BT_SHOT_EFFECT arrow_shot = {"info", 0, 1, {0.0f, 1.2f, 0.0f, 0.0f}, {0.0f, 10.0f, 0.0f, 0.0f}, 360, 69, 0x100, 2, 1,
                             {-1, 0, 1, 1}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT meteor_shot = {"info", 0, 0, {}, {0.0f, 30.0f, 0.0f, 0.0f}, 63, 69, 2, 2, 1,
                              {0, 1, 2, 2}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT aura_shot = {"info", 1, 0, {}, {60.0f, 0.0f, 0.0f, 0.0f}, -1, 74, 2, 3, 1,
                            {0, -1, -1, -1}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT tornado_shot = {"info", 1, 0, {}, {0.0f, 20.0f, 0.0f, 0.0f}, -1, 74, 2, 3, 1,
                               {0, 1, 2, 2}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
// clang-format on
BT_SHOT_EFFECT prison_shot = IcePrisonShot();

enum Effect {
    kArrowShot,
    kPrisonShot,
    kMeteorShot,
    kAuraShot,
    kTornadoShot,
    kEffects,
};

struct EffectSpec {
    BT_SHOT_EFFECT *description;
    const char     *path;
    int             shots;    // two each, so the next fires while the last one's burst still plays
    float           speed[4]; // each phase's motion speed as the companion's script sets it; 0 keeps the KEY's
};

const EffectSpec kSpecs[kEffects] = {
    {&arrow_shot,   kIceArrowFile,                2,          {}                      },
    {&prison_shot,  kIcePrisonFile,               2,          {}                      },
    {&meteor_shot,  "dun/monstor/i_meteo.chr",    2,          {0.1f}                  },
    {&aura_shot,    "dun/monstor/b3_reiki.chr",   2,          {0.2f}                  },
    {&tornado_shot, "dun/monstor/i_tatumaki.chr", kTornadoes, {0.2f, 0.2f, 0.2f, 0.2f}},
};

// The arrow, fired this far ahead of her.
constexpr float kArrowLead = 15.0f;
constexpr float kChest = 10.1f;
constexpr int   kArrowSettle = 2; // steps the arrow's hit takes to reach the player
constexpr float kMeteorHeight = 50.0f;
constexpr float kMeteorFall = 1.6f;
constexpr float kMeteorStrike = 36.0f;
constexpr float kTornadoNear = 30.0f;
constexpr float kTornadoSpread = 80.0f;
constexpr float kTornadoLift = 2.0f;
constexpr float kTornadoLeash = 100.0f;

// Her companions' sounds (the meteor, a tornado rising).
constexpr int kMeteorSound = 1764;
constexpr int kMeteorHitSound = 1774;
constexpr int kTornadoSounds[] = {1767, 1766};

struct Fight {
    int  queen = -1;
    int *mailbox = nullptr;
    int  effect[kEffects] = {-1, -1, -1, -1, -1};
    int  arrow = -1;
    int  arrow_landed = 0;
    int  prison = -1;
    int  meteor = -1;
    int  aura = -1;
    int  tornado[kTornadoes] = {-1, -1, -1};
};

Fight fight;


void Play(int sound) {
    SndSePlay(sound, -1, 0);
}

CSHOT_EFFECT *EffectOf(Effect which) {
    return fight.effect[which] >= 0 ? ShotEffectAt(fight.effect[which]) : nullptr;
}

// Fires one of her effects from position toward target as her own shot; the shot's slot, or -1.
int Fire(Effect which, float *position, float *target) {
    CSHOT_EFFECT *effect = EffectOf(which);
    if (effect == nullptr) {
        return -1;
    }
    NowShotEffect->Set(fight.effect[which], position, target);
    int slot = effect->current_slot;
    if (slot == -1) {
        return -1;
    }
    NowShotEffect->SetUserID2(fight.queen);
    EnemyShotFired(fight.queen);
    if (UserStatus != nullptr) {
        effect->damage[slot] = IceQueenPieceDamage(effect->damage[slot], EnemyRegion(selectMapNo, UserStatus->cur_floor));
    }
    if (effect->effect_data->stationary != 0) {
        effect->velocity[slot][0] = effect->velocity[slot][1] = effect->velocity[slot][2] = 0.0f;
    }
    return slot;
}

bool Live(Effect which, int slot) {
    CSHOT_EFFECT *effect = EffectOf(which);
    return effect != nullptr && slot >= 0 && effect->active[slot] != 0;
}

// Each phase plays at the speed the companion's script gave its motion.
void PaceMotions() {
    for (int which = 0; which < kEffects; which++) {
        CSHOT_EFFECT *effect = EffectOf(static_cast<Effect>(which));
        if (effect == nullptr) {
            continue;
        }
        for (int slot = 0; slot < effect->slot_count; slot++) {
            float speed = kSpecs[which].speed[effect->phase[slot]];
            if (effect->active[slot] != 0 && speed > 0.0f) {
                effect->chara[slot].SetMotionSpeed(speed);
            }
        }
    }
}

float Distance(const float *a, const float *b) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// The arrow: fired 15 ahead of her toward the player, steered while flying; once it is spent, a frozen
// player is imprisoned, else she attacks again.
void StepArrow(CMonstorUnit &unit) {
    CSHOT_EFFECT *effect = EffectOf(kArrowShot);
    if (fight.mailbox[kArrow] == 1 && effect != nullptr && !Live(kArrowShot, fight.arrow)) {
        float queen[4];
        unit.chara[fight.queen][0].GetPosition(queen);
        float toward[4] = {CharaMain.pos[0] - queen[0], 0.0f, CharaMain.pos[2] - queen[2], 1.0f};
        float length = std::sqrt(toward[0] * toward[0] + toward[2] * toward[2]);
        if (length > 0.0f) {
            toward[0] /= length;
            toward[2] /= length;
        }
        float from[4] = {queen[0] + toward[0] * kArrowLead, queen[1] + kChest, queen[2] + toward[2] * kArrowLead, 1.0f};
        float chest[4];
        PlayerChest(chest);
        fight.arrow = Fire(kArrowShot, from, chest);
        if (fight.arrow >= 0) {
            fight.mailbox[kArrow] = 0;
            fight.mailbox[kPhase] = 1;
        }
        return;
    }
    if (fight.arrow < 0 || effect == nullptr) {
        return;
    }
    int slot = fight.arrow;
    if (effect->active[slot] != 0 && effect->phase[slot] == 1) {
        SteerIceArrow(*effect, slot);
        return;
    }
    // Spent: burst, worn out or gone; once its hit has reached the player.
    if (++fight.arrow_landed < kArrowSettle) {
        return;
    }
    fight.arrow_landed = 0;
    fight.arrow = -1;
    if (PlayerFrozen()) {
        PlayIceFreeze();
        fight.mailbox[kPrison] = 1;
    } else {
        fight.mailbox[kPhase] = 0;
    }
}

// The prison: forms at the frozen player's feet, holds while the freeze lasts, then shatters.
void StepPrison() {
    CSHOT_EFFECT *effect = EffectOf(kPrisonShot);
    if (effect == nullptr) {
        return;
    }
    if (fight.mailbox[kPrison] == 1 && !Live(kPrisonShot, fight.prison)) {
        float feet[4];
        PlayerFeet(feet);
        fight.prison = Fire(kPrisonShot, feet, feet);
        if (fight.prison < 0) {
            return;
        }
        fight.mailbox[kPrison] = 4; // forming
    }
    int slot = fight.prison;
    if (!Live(kPrisonShot, slot)) {
        fight.prison = -1;
        if (fight.mailbox[kPrison] == 2) {
            fight.mailbox[kPrison] = 0;
        }
        return;
    }
    if (effect->phase[slot] == 1) {
        HoldIcePrison(*effect, slot);
        if (fight.mailbox[kPrison] == 4) {
            fight.mailbox[kPrison] = 3;
            fight.mailbox[kPhase] = 3;
        }
        if (fight.mailbox[kPrison] == 3 && !PlayerAfflicted()) {
            fight.mailbox[kPrison] = 2;
        }
    }
    if (fight.mailbox[kPrison] == 2 && effect->phase[slot] < 2) {
        fight.mailbox[kPrison] = 0;
        ShatterIcePrison(*effect, slot);
    }
}

// The meteor: forms above the player and falls; when it lands her script carries on.
void StepMeteor() {
    CSHOT_EFFECT *effect = EffectOf(kMeteorShot);
    if (effect == nullptr) {
        return;
    }
    if (fight.mailbox[kMeteor] == 1 && !Live(kMeteorShot, fight.meteor)) {
        float above[4];
        sceVu0CopyVector(above, CharaMain.pos);
        above[1] += kMeteorHeight;
        above[3] = 1.0f;
        float below[4] = {above[0], above[1] - 2.0f * kMeteorHeight, above[2], 1.0f};
        fight.meteor = Fire(kMeteorShot, above, below);
        if (fight.meteor >= 0) {
            fight.mailbox[kMeteor] = 2; // falling
        }
        return;
    }
    if (fight.mailbox[kMeteor] != 2) {
        return;
    }
    int slot = fight.meteor;
    if (Live(kMeteorShot, slot) && effect->phase[slot] < 2) {
        if (effect->phase[slot] == 1) {
            effect->velocity[slot][0] = 0.0f;
            effect->velocity[slot][1] = -kMeteorFall;
            effect->velocity[slot][2] = 0.0f;
        }
        return;
    }
    float position[4];
    float chest[4];
    effect->chara[slot].GetPosition(position);
    PlayerChest(chest);
    Play(kMeteorSound);
    if (Distance(position, chest) <= kMeteorStrike) {
        Play(kMeteorHitSound);
    }
    fight.meteor = -1;
    fight.mailbox[kMeteor] = 0;
    fight.mailbox[kPhase] = 4;
}

// The aura: a blast at her feet that throws the player off; then she attacks again.
void StepAura(CMonstorUnit &unit) {
    if (fight.mailbox[kAura] == 1 && !Live(kAuraShot, fight.aura)) {
        float feet[4];
        unit.chara[fight.queen][0].GetPosition(feet);
        feet[1] += 1.0f;
        feet[3] = 1.0f;
        fight.aura = Fire(kAuraShot, feet, feet);
        if (fight.aura >= 0) {
            fight.mailbox[kAura] = 2;
        }
        return;
    }
    if (fight.mailbox[kAura] == 2 && !Live(kAuraShot, fight.aura)) {
        fight.aura = -1;
        fight.mailbox[kPhase] = 0;
        fight.mailbox[kAura] = 0;
    }
}

// A tornado: rises 30 to 110 from the player and whirls until it hits the player or the player gets away.
void StepTornadoes() {
    CSHOT_EFFECT *effect = EffectOf(kTornadoShot);
    if (effect == nullptr) {
        return;
    }
    for (int i = 0; i < kTornadoes; i++) {
        int &mail = fight.mailbox[kTornado + i];
        int &slot = fight.tornado[i];
        if (mail == 1 && !Live(kTornadoShot, slot)) {
            float angle = 2.0f * PI * GameFraction();
            float reach = kTornadoNear + kTornadoSpread * GameFraction();
            float at[4] = {CharaMain.pos[0] + std::cos(angle) * reach, CharaMain.pos[1] + kTornadoLift,
                           CharaMain.pos[2] + std::sin(angle) * reach, 1.0f};
            slot = Fire(kTornadoShot, at, at);
            if (slot >= 0) {
                mail = 3; // rising
                for (int sound : kTornadoSounds) {
                    Play(sound);
                }
            }
            continue;
        }
        if (mail < 2) {
            continue;
        }
        if (!Live(kTornadoShot, slot)) {
            slot = -1;
            mail = 0;
            continue;
        }
        if (effect->phase[slot] == 1) {
            mail = 2;
            float at[4];
            effect->chara[slot].GetPosition(at);
            float dx = CharaMain.pos[0] - at[0];
            float dz = CharaMain.pos[2] - at[2];
            if (std::sqrt(dx * dx + dz * dz) > kTornadoLeash) {
                EnterShotPhase(*effect, slot, 3);
            }
        }
    }
}

// Her monster and her cast's mailbox this step, or false when she is not fighting off her floor.
bool Fighting(const CMonstorUnit &unit) {
    fight.queen = BossCastMonster(Monster::ICE_QUEEN);
    fight.mailbox = BossCastMailbox(Monster::ICE_QUEEN);
    return IsKind(unit, fight.queen, Monster::ICE_QUEEN) && BossOutOfPlace(fight.queen) && unit.monster[fight.queen].hp > 0;
}

void OnFloor() {
    fight = {};
}

void OnModel(CMonstorUnit &, int, int model_no) {
    if (model_no != No(Monster::ICE_QUEEN) || LoadedRoster() == nullptr || !BossSupported(model_no)) {
        return;
    }
    for (int which = 0; which < kEffects; which++) {
        fight.effect[which] = EnterShotEffect(kSpecs[which].description, kSpecs[which].shots, kSpecs[which].path);
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), AddEnemyModelListener(OnModel), true);

} // namespace

void IceQueenStep(CMonstorUnit &unit) {
    if (!Fighting(unit)) {
        return;
    }
    StepArrow(unit);
    StepPrison();
    StepMeteor();
    StepAura(unit);
    StepTornadoes();
    PaceMotions();
}

void IceQueenDefeated() {
    for (int which = 0; which < kEffects; which++) {
        if (CSHOT_EFFECT *effect = EffectOf(static_cast<Effect>(which))) {
            effect->OffEffect(-1);
        }
    }
    Fight cleared;
    for (int which = 0; which < kEffects; which++) {
        cleared.effect[which] = fight.effect[which];
    }
    fight = cleared;
}

} // namespace expanded
