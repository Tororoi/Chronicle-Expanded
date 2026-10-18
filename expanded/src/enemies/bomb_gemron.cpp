#include "expanded/enemies/bomb_gemron.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "dun/gameloop.hpp"
#include "expanded/core/frame_tint.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/scaling.hpp"
#include "expanded/enemies/species.hpp"
#include "frame.hpp"
#include "hitmark.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "shot_effect.hpp"
#include "shot_effect_pack.hpp"
#include "shot_utils.hpp"

namespace expanded {

namespace {

constexpr int kMonsters = 16;

// The record: Holy Gemron's, its model the built one, weak to fire (150 %) and immune to the rest.
constexpr int kName = 320;
constexpr s16 kAbs = 40;

void BuildRecord(MONSTOR_MODEL &record) {
    std::memset(record.model_name[0], 0, sizeof(record.model_name[0]));
    std::strcpy(record.model_name[0], "e167a");
    record.attachment_weight[0] = 150;
    for (int i = 1; i < 5; i++) {
        record.attachment_weight[i] = 0;
    }
    record.shot_effect[0] = record.shot_effect[1] = -1; // its own, entered when it is set up
    record.exp = kAbs;
    record.name_no = kName;
    record.steal_item = ITEM_BOMB;
    record.rare_item = ITEM_REPAIR_POWDER;
}

[[maybe_unused]] const bool kSpecies =
    AddSpecies({Monster::BOMB_GEMRON, Monster::GEMRON_HOLY, kBombGemronModelFile, nullptr, BuildRecord});

// Its two shots, both ending in the item bomb's blast (no hit of their own): the thrown bomb, the apple shot's flight
// at the Big Bang pellet's blast scale, and the death and self-destruct blast, no flight and no life, at the Big Bang
// drop's. Each blast's hit radius and shock ring are 25 per unit of scale (unk_056), its damage the Gemron's shot on
// the floor (its normalized stats), set as it throws or blows up.
constexpr float kAimHeight = 14.0f, kHolyAimHeight = 7.4f;
constexpr int   kBlastDamage = 150;
// clang-format off
BT_SHOT_EFFECT throw_shot = {"ringo_ex", 0, 1, {0.2f, 1.5f, 0.0f, 0.0f}, {}, 160, 0, 0, 3, 1, {-1, 0, -1, -1}, SHOT_END_EFFECT_BOMB, {25, 0}, 0.5f, kBlastDamage, {-1, -1, -1, -1}};
BT_SHOT_EFFECT blast_shot = {"ringo_ex", 0, 1, {}, {}, 0, 0, 0, 3, 1, {-1, 0, -1, -1}, SHOT_END_EFFECT_BOMB, {25, 0}, 2.0f, kBlastDamage, {-1, -1, -1, -1}};
// clang-format on
constexpr int kShots = 4;

// The death clip (motion 11, 105-125) blows up at 122, the blast 11 over the Gemron's feet.
constexpr int   kDeathMotion = 11;
constexpr float kDeathFrom = 105.0f, kBlastFrame = 122.0f, kBlastHeight = 11.0f;
// The self-destruct: under a quarter of its life with the player within 22.
constexpr int   kSelfDestructPercent = 25;
constexpr float kSelfDestructRange = 22.0f;
constexpr float kFadeStep = 4.0f;
// A hit's reaction takes the fuse over; it may be lit again this many steps later.
constexpr int kRelight = 60;

// The fuse: the big bomb's wick centreline, tip first, in its bone's frame (the built bomb's placement), the spark at
// its tip until a fuse burns it down; the flash (the machine-gun hit's, at 3 across, its three cells 4 frames each)
// and a guard spark burst every 15 frames (20 marks at twice the size, scattered 0.75, bouncing 6 below the wick).
constexpr const char *kBombFrame = "tama1__m";
constexpr float       kWick[][3] = {
    {-4.502f,  0.4241f, 0.163f },
    {-4.7109f, 0.4496f, 0.1215f},
    {-4.7651f, 0.3937f, 0.651f },
    {-4.9872f, 0.3606f, 1.1262f},
    {-5.2539f, 0.3325f, 1.5962f},
    {-5.3204f, 0.2763f, 2.1387f},
    {-5.0037f, 0.194f,  2.5808f},
    {-4.4755f, 0.1267f, 2.7136f},
    {-3.9403f, 0.0833f, 2.633f },
    {-3.4238f, 0.0494f, 2.4863f},
};
constexpr int   kWickPoints = static_cast<int>(std::size(kWick));
constexpr float kRedden = 150.0f; // added to the ambient's red at the blast
constexpr float kFlashSize = 3.0f;
constexpr int   kFlashCell = 4;
constexpr int   kBurstEvery = 15;
constexpr int   kBurstMarks = 20;
constexpr float kBurstSpread = 0.75f, kBurstSize = 2.0f, kBurstShrink = 0.005f, kBurstGravity = 0.02f, kBurstSpeed = 1.3f;
constexpr float kBurstDrop = 6.0f;
constexpr int   kBurstFirst = 12; // the hit marks the bursts take, as CheckDmg cycles through them
constexpr int   kDeathLoopMotion = 12;
constexpr float kSelfDestructBlast = kBombSelfDestructEnd - 1;

enum class Fuse {
    None,
    Lit,    // the self-destruct playing
    Fading, // blown, fading out
};

struct Gemron {
    bool bomb = false;
    bool dying = false; // the death clip seen below its blast frame
    bool blown = false;
    Fuse fuse = Fuse::None;
    int  relight = 0;
    // The wick's spark as the unit's draw placed it, and the effect draw it was placed for.
    float spark[4] = {};
    int   drawn = -1;
};

Gemron gemrons[kMonsters];
int    blast_effect = -1;
int    effect_draws = 0;

void OnModel(CMonstorUnit &unit, int model_slot, int model_no) {
    if (model_slot == 0) {
        blast_effect = -1;
        ClearFrameTints();
    }
    if (model_no != No(Monster::BOMB_GEMRON)) {
        return;
    }
    int thrown = EnterShotEffect(&throw_shot, kShots, kBombThrowFile);
    blast_effect = EnterShotEffect(&blast_shot, kShots, kBombThrowFile);
    if (thrown >= 0) {
        unit.model[model_slot].shot_effect[0] = static_cast<s16>(thrown);
    }
}

void OnSpawn(CMonstorUnit &unit, int monster) {
    gemrons[monster] = {};
    gemrons[monster].bomb = ModelKind(unit.monster[monster].base_model) == No(Monster::BOMB_GEMRON);
}

[[maybe_unused]] const bool kListening = (AddEnemyModelListener(OnModel), AddEnemySpawnListener(OnSpawn), true);

// Both blasts deal the Gemron's shot damage.
void ArmBlasts(const CMonstorUnit &unit, int monster) {
    int damage = MonsterStats(unit, monster).shot;
    if (damage > 0) {
        throw_shot.bomb_damage = blast_shot.bomb_damage = damage;
    }
}

void Blast(CMonstorUnit &unit, int monster) {
    if (blast_effect < 0) {
        return;
    }
    ArmBlasts(unit, monster);
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    at[1] += kBlastHeight;
    at[3] = 1.0f;
    NowShotEffect->Set(blast_effect, at, at);
    NowShotEffect->SetUserID2(monster);
}

void PlayMotion(CMonstorUnit &unit, int monster, int motion, int flags) {
    float speed = unit.chara[monster][0].motion_type.motion_info[motion].speed;
    unit.monster[monster].requested_motion = static_cast<s16>(motion);
    unit.monster[monster].requested_motion_flags = static_cast<s16>(flags);
    unit.monster[monster].requested_motion_speed = speed;
    for (int i = 0; i <= unit.monster[monster].attachment_count; i++) {
        unit.chara[monster][i].SetMotion(motion, flags);
        unit.chara[monster][i].SetMotionSpeed(speed);
    }
}

} // namespace

void BombGemronAim(const CMonstorUnit &unit, int monster, float *target) {
    if (monster >= 0 && monster < kMonsters && gemrons[monster].bomb && unit.monster[monster].hp > 0) {
        target[1] += kAimHeight - kHolyAimHeight;
        ArmBlasts(unit, monster);
    }
}

bool BombGemronStep(CMonstorUnit &unit, int monster) {
    if (monster < 0 || monster >= kMonsters || !gemrons[monster].bomb) {
        return false;
    }
    Gemron           &gemron = gemrons[monster];
    MONSTOR          &live = unit.monster[monster];
    const CCharacter &body = unit.chara[monster][0];
    float             frame = body.motion_type.state.time;
    switch (gemron.fuse) {
        case Fuse::None:
            if (live.hp <= 0) {
                // The death clip blows up at its blast frame, once seen running below it (the first steps can still read
                // the last clip's frame).
                if (body.motion_no == kDeathMotion && !gemron.blown) {
                    gemron.dying = gemron.dying || (frame >= kDeathFrom && frame < kBlastFrame);
                    if (gemron.dying && frame >= kBlastFrame) {
                        Blast(unit, monster);
                        gemron.blown = true;
                    }
                }
                return false;
            }
            if (gemron.relight > 0) {
                gemron.relight--;
                return false;
            }
            if (live.hp * 100 >= live.max_hp * kSelfDestructPercent || live.player_distance > kSelfDestructRange) {
                return false;
            }
            live.movement_speed = 0.0f;
            live.turn_speed = 0.0f;
            PlayMotion(unit, monster, kBombSelfDestructKey, 2 | 4);
            gemron.fuse = Fuse::Lit;
            return true;
        case Fuse::Lit:
            // Hittable throughout: a hit's reaction (its script's label 110) or a kill takes over.
            if (live.hp <= 0 || body.motion_no != kBombSelfDestructKey) {
                gemron.fuse = Fuse::None;
                gemron.relight = kRelight;
                return false;
            }
            if (body.motion_no == kBombSelfDestructKey && frame >= kBombSelfDestructEnd - 1) {
                Blast(unit, monster);
                gemron.blown = true;
                live.palette_alpha_step = kFadeStep;
                live.palette_delay = 0;
                gemron.fuse = Fuse::Fading;
            }
            return true;
        case Fuse::Fading:
            if (live.palette_alpha <= 0.0f && live.state != -1) {
                live.state = -1;
                unit.alive_count--;
            }
            return true;
    }
    return false;
}

namespace {

// How far a burning fuse is, 0 at the tip to 1 at the blast; -1 unburnt. Its own motion and frame: every unit of the
// species shares one frame tree, so only its own draw has its pose.
float Burnt(const CMonstorUnit &unit, int monster) {
    const CCharacter &body = unit.chara[monster][0];
    float             frame = body.motion_type.state.time;
    if (body.motion_no == kDeathMotion && unit.monster[monster].hp <= 0) {
        return frame >= kDeathFrom ? std::min(1.0f, (frame - kDeathFrom) / (kBlastFrame - kDeathFrom)) : 0.0f;
    }
    if (body.motion_no == kDeathLoopMotion && unit.monster[monster].hp <= 0) {
        return 1.0f;
    }
    if (body.motion_no == kBombSelfDestructKey && gemrons[monster].fuse != Fuse::None) {
        return frame >= kBombSelfDestructStart ? std::min(1.0f, (frame - kBombSelfDestructStart) / (kSelfDestructBlast - kBombSelfDestructStart)) : 0.0f;
    }
    return -1.0f;
}

// Whether its spark still shows: not blown, not fading, not in its death loop.
bool Lit(const CMonstorUnit &unit, int monster) {
    return !gemrons[monster].blown && unit.monster[monster].palette_alpha_step <= 0.0f && unit.chara[monster][0].motion_no != kDeathLoopMotion;
}

CFrame *BombFrame(CMonstorUnit &unit, int monster) {
    CFrame *root = unit.chara[monster][0].frame;
    return root != nullptr ? root->SearchFrame(const_cast<char *>(kBombFrame)) : nullptr;
}

void Burst(int monster, const float *at) {
    CHitMark &mark = HitMark[kBurstFirst + monster % 4];
    float     direction[4] = {0.0f, 1.0f, 0.0f, 0.0f}; // the engine scatters a burst about one axis of it: here none
    float     position[4] = {at[0], at[1], at[2], 1.0f};
    mark.Set(position, direction, HIT_MARK_GUARD, kBurstSpread, kBurstShrink * kBurstSize, kBurstGravity, kBurstSpeed, kBurstMarks,
             at[1] - kBurstDrop);
    for (int i = 0; i < HIT_MARK_MAX; i++) {
        mark.size[i] *= kBurstSize;
        mark.used[i] = i < kBurstMarks ? 1 : 0;
    }
    mark.count = kBurstMarks;
}

} // namespace

void BombGemronDrawing(CMonstorUnit &unit, int monster) {
    if (monster < 0 || monster >= kMonsters || !gemrons[monster].bomb) {
        return;
    }
    if (CFrame *bomb = BombFrame(unit, monster)) {
        float burnt = Burnt(unit, monster);
        SetFrameTint(bomb, burnt > 0.0f ? kRedden * burnt : 0.0f, 0.0f, 0.0f);
    }
}

void BombGemronDrawn(CMonstorUnit &unit, int monster) {
    if (monster < 0 || monster >= kMonsters || !gemrons[monster].bomb || !Lit(unit, monster)) {
        return;
    }
    CFrame *bomb = BombFrame(unit, monster);
    if (bomb == nullptr) {
        return;
    }
    float s = std::max(0.0f, Burnt(unit, monster)) * (kWickPoints - 1);
    int   i = std::min(kWickPoints - 2, static_cast<int>(s));
    float f = s - static_cast<float>(i);
    float local[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    for (int c = 0; c < 3; c++) {
        local[c] = kWick[i][c] + (kWick[i + 1][c] - kWick[i][c]) * f;
    }
    bomb->GetWorldPosition(gemrons[monster].spark, local);
    gemrons[monster].drawn = effect_draws;
}

void BombGemronEffectsDraw() {
    for (int monster = 0; NowMonstorUnit != nullptr && monster < kMonsters; monster++) {
        Gemron &gemron = gemrons[monster];
        if (!gemron.bomb || gemron.drawn != effect_draws || !Lit(*NowMonstorUnit, monster)) {
            continue;
        }
        float at[4] = {gemron.spark[0], gemron.spark[1], gemron.spark[2], 1.0f};
        set3DCellModel(at, const_cast<char *>("basefx00"), kFlashSize, ((effect_draws / kFlashCell) % 3) << 4, 0x70, 0x10, 0x10, 0x80);
        if ((effect_draws + monster) % kBurstEvery == 0) {
            Burst(monster, at);
        }
    }
    effect_draws++;
}

} // namespace expanded
