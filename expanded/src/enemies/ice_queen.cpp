#include "expanded/enemies/ice_queen.hpp"

#include <cmath>
#include <cstdlib>

#include "dun/gameloop.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/boss_casts.hpp"
#include "expanded/enemies/bosses.hpp"
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

constexpr int kMonsters = 16;

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
BT_SHOT_EFFECT prison_shot = {"info", 1, 0, {}, {-100.0f, -100.0f, -100.0f, -100.0f}, 1800, 0, 0, 2, 1,
                              {0, 0, 1, 1}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT meteor_shot = {"info", 0, 0, {}, {0.0f, 30.0f, 0.0f, 0.0f}, 63, 69, 2, 2, 1,
                              {0, 1, 2, 2}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT aura_shot = {"info", 1, 0, {}, {60.0f, 0.0f, 0.0f, 0.0f}, -1, 74, 2, 3, 1,
                            {0, -1, -1, -1}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
BT_SHOT_EFFECT tornado_shot = {"info", 1, 0, {}, {0.0f, 20.0f, 0.0f, 0.0f}, -1, 74, 2, 3, 1,
                               {0, 1, 2, 2}, 0, {}, 0.0f, 0, {-1, -1, -1, -1}};
// clang-format on

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
    {&arrow_shot,   "dun/monstor/korinoya.chr",   2,          {}                      },
    {&prison_shot,  "dun/monstor/kori.chr",       2,          {}                      },
    {&meteor_shot,  "dun/monstor/i_meteo.chr",    2,          {0.1f}                  },
    {&aura_shot,    "dun/monstor/b3_reiki.chr",   2,          {0.2f}                  },
    {&tornado_shot, "dun/monstor/i_tatumaki.chr", kTornadoes, {0.2f, 0.2f, 0.2f, 0.2f}},
};

// The arrow: the Crystal Gemron's steering, turning toward the player's chest only within 60 degrees ahead.
constexpr float kArrowSpeed = 1.2f;
constexpr float kArrowTurn = 0.12f;
constexpr float kArrowCone = 0.5f;
constexpr float kChest = 10.1f;
constexpr float kArrowLead = 15.0f;
constexpr int   kArrowSettle = 2; // steps the arrow's hit takes to reach the player
constexpr float kMeteorHeight = 50.0f;
constexpr float kMeteorFall = 1.6f;
constexpr float kMeteorStrike = 36.0f;
constexpr float kShatterSpeed = 0.5f;
constexpr float kTornadoNear = 30.0f;
constexpr float kTornadoSpread = 80.0f;
constexpr float kTornadoLift = 2.0f;
constexpr float kTornadoLeash = 100.0f;

// Her companions' sounds (the arrow's freeze, the prison breaking, the meteor, a tornado rising).
constexpr int kFreezeSounds[] = {1772, 1777};
constexpr int kShatterSound = 1776;
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

bool Here(const CMonstorUnit &unit, int monster, Monster kind) {
    return monster >= 0 && monster < kMonsters && unit.monster[monster].state != -1 &&
           ModelKind(unit.monster[monster].base_model) == No(kind);
}

bool Frozen() {
    return UserStatus != nullptr && (UserStatus->ailments[UserStatus->cur_chara] & AILMENT_FREEZE) != 0;
}

bool Afflicted() {
    return UserStatus != nullptr && UserStatus->ailment_frames[UserStatus->cur_chara] > 0;
}

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
    if (effect->effect_data->stationary != 0) {
        effect->velocity[slot][0] = effect->velocity[slot][1] = effect->velocity[slot][2] = 0.0f;
    }
    return slot;
}

bool Live(Effect which, int slot) {
    CSHOT_EFFECT *effect = EffectOf(which);
    return effect != nullptr && slot >= 0 && effect->active[slot] != 0;
}

// Moves a shot to a phase as its impact would, ending it when the phase has no motion.
void EnterPhase(CSHOT_EFFECT &effect, int slot, int phase) {
    effect.phase[slot] = static_cast<s16>(phase);
    int motion = effect.effect_data->motion[phase];
    if (motion == -1) {
        effect.active[slot] = 0;
        return;
    }
    effect.chara[slot].motion_type.state.time = static_cast<float>(effect.chara[slot].motion_type.motion_info[motion].start);
    effect.chara[slot].SetMotion(motion, 6);
    effect.velocity[slot][0] = effect.velocity[slot][1] = effect.velocity[slot][2] = 0.0f;
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

void PlayerChest(float *chest) {
    sceVu0CopyVector(chest, CharaMain.pos);
    chest[1] += kChest;
    chest[3] = 1.0f;
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
        float position[4];
        float chest[4];
        effect->chara[slot].GetPosition(position);
        PlayerChest(chest);
        float heading[4];
        float wanted[4] = {chest[0] - position[0], chest[1] - position[1], chest[2] - position[2], 1.0f};
        sceVu0Normalize(heading, effect->velocity[slot]);
        sceVu0Normalize(wanted, wanted);
        if (sceVu0InnerProduct(heading, wanted) >= kArrowCone) {
            float step[4] = {wanted[0] - heading[0], wanted[1] - heading[1], wanted[2] - heading[2], 1.0f};
            float gap = std::sqrt(step[0] * step[0] + step[1] * step[1] + step[2] * step[2]);
            if (gap <= kArrowTurn) {
                sceVu0CopyVector(heading, wanted);
            } else {
                for (int i = 0; i < 3; i++) {
                    heading[i] += step[i] * (kArrowTurn / gap);
                }
                sceVu0Normalize(heading, heading);
            }
        }
        sceVu0ScaleVectorXYZ(effect->velocity[slot], heading, kArrowSpeed);
        sceVu0FMATRIX facing;
        LookAtMatrixZ(facing, effect->velocity[slot]);
        effect->chara[slot].frame->SetTransMatrix(facing);
        effect->chara[slot].SetPosition(position);
        return;
    }
    // Spent: burst, worn out or gone; once its hit has reached the player.
    if (++fight.arrow_landed < kArrowSettle) {
        return;
    }
    fight.arrow_landed = 0;
    fight.arrow = -1;
    if (Frozen()) {
        for (int sound : kFreezeSounds) {
            Play(sound);
        }
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
        sceVu0CopyVector(feet, CharaMain.pos);
        feet[3] = 1.0f;
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
        // Formed: hold its last frame.
        const MOTION_INFO &formed = effect->chara[slot].motion_type.motion_info[effect->effect_data->motion[1]];
        effect->chara[slot].motion_type.state.time = static_cast<float>(formed.end) - 0.01f;
        effect->chara[slot].SetMotionSpeed(0.0f);
        if (fight.mailbox[kPrison] == 4) {
            fight.mailbox[kPrison] = 3;
            fight.mailbox[kPhase] = 3;
        }
        if (fight.mailbox[kPrison] == 3 && !Afflicted()) {
            fight.mailbox[kPrison] = 2;
        }
    }
    if (fight.mailbox[kPrison] == 2 && effect->phase[slot] < 2) {
        Play(kShatterSound);
        fight.mailbox[kPrison] = 0;
        EnterPhase(*effect, slot, 2);
        effect->chara[slot].SetMotionSpeed(kShatterSpeed);
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
            float angle = 2.0f * PI * static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            float reach = kTornadoNear + kTornadoSpread * static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
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
                EnterPhase(*effect, slot, 3);
            }
        }
    }
}

// Her monster and her cast's mailbox this step, or false when she is not fighting off her floor.
bool Fighting(const CMonstorUnit &unit) {
    fight.queen = BossCastMonster(Monster::ICE_QUEEN);
    fight.mailbox = BossCastMailbox(Monster::ICE_QUEEN);
    return Here(unit, fight.queen, Monster::ICE_QUEEN) && BossOutOfPlace(fight.queen) && unit.monster[fight.queen].hp > 0;
}

void OnModel(CMonstorUnit &, int model_slot, int model_no) {
    if (model_slot == 0) {
        fight = {};
    }
    if (model_no != No(Monster::ICE_QUEEN) || LoadedRoster() == nullptr || !BossSupported(model_no)) {
        return;
    }
    for (int which = 0; which < kEffects; which++) {
        fight.effect[which] = EnterShotEffect(kSpecs[which].description, kSpecs[which].shots, kSpecs[which].path);
    }
}

[[maybe_unused]] const bool kListening = (AddEnemyModelListener(OnModel), true);

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
