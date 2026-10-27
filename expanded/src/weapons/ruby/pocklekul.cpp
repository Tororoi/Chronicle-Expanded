#include "expanded/weapons/ruby/pocklekul.hpp"

#include <libvu0.h>

#include <cmath>
#include <cstdio>
#include <format>
#include <optional>
#include <string>

#include "character.hpp"
#include "clsmes.hpp"
#include "dngmessageman.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/shot_effects.hpp"
#include "expanded/enemies/ally_unit.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/ruby/pocklekul/fights.hpp"
#include "expanded/weapons/summon_smoke.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "gamepad.hpp"
#include "gametext.hpp"
#include "itemdata.hpp"
#include "localize.hpp"
#include "menu_misc.hpp"
#include "mglib.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "sysmes.hpp"
#include "userstatus.hpp"

extern s32 lockOnTargetNo;

namespace expanded {

namespace {

using namespace pocklekul;

// A monster's name: CharaNameMes's name_no + 3000; the English ones where the names are not loaded.
constexpr int         kNameMessage = 3000;
constexpr const char *kPockleNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Hell Pockle"};
static_assert(std::size(kPockleNames) == kPockleKinds.size());

// The dungeon's thirst line, which a notice does not wait for.
constexpr int kThirstMessage = 171;

// Where its script waits while it idles: resumed each step, it yields at once (a hit's or its fall's label still runs).
vmcode_t park_wait[] = {
    {RS_OP_WAIT, 0, 0},
    {RS_OP_WAIT, 0, 0},
    {RS_OP_WAIT, 0, 0},
    {RS_OP_END,  0, 0}
};

struct Pockle {
    int       chara = -1; // the summoner, while the weapon is active
    GuardHold guard;      // latched once it acts on the charge, until R1 or the pose drops
    bool      up = false;
    int       kind = -1;   // its index in kPockleKinds
    int       grown = 0;   // frames of its growth shown
    int       target = -1; // its enemy, a floor monster
    bool      parked = false;
    int       settling = 0;
    float     point[4] = {}; // where its script finds the player
    bool      tinting = false;
    bool      pinned = false; // the smoke follows it
    int       notice = -1;    // frames its notice has waited for the dungeon's line, -1 with none due
};

Pockle pockle;

bool ControllerGuard() {
    return GamePad.On(PAD_R1) != 0;
}

PockleGuardButton guard_button = ControllerGuard;

bool Active() {
    return UserStatus != nullptr && WeaponActive(ITEM_WEAPON_POCKLEKUL);
}

// No menu opening, no hold and no event: a summon waits for such a frame.
bool QuietFloor() {
    return BtGameModeFlag == BT_GAME_MODE_PLAY && driveStepHold == 0 && frameCaputer == 0 && BtEventMode == 0;
}

std::string PockleName(int kind) {
    if (kind < 0 || kind >= static_cast<int>(kPockleKinds.size())) {
        return {};
    }
    const MONSTOR_MODEL &model = MonstorTable[No(kPockleKinds[kind])];
    if (CharaNameMes != nullptr) {
        const short *codes = CharaNameMes->GetTextLineDataTop(model.name_no + kNameMessage);
        std::string  name = codes != nullptr ? GameTextDecode(codes) : std::string{};
        if (!name.empty()) {
            return name;
        }
    }
    return kPockleNames[kind];
}

std::optional<std::string> SummonedLine() {
    if (pockle.kind < 0) {
        return std::nullopt;
    }
    std::string line = LocalizeText("expanded.pocklekul.summoned", "[monster] summoned");
    if (std::size_t at = line.find("[monster]"); at != std::string::npos) {
        line.replace(at, 9, PockleName(pockle.kind));
    }
    return line;
}

[[maybe_unused]] const bool kNotice = (SetSystemMessage(kPocklekulMessage, SummonedLine), true);

[[maybe_unused]] const bool kRegistered =
    (RegisterWeaponShotEffect(ITEM_WEAPON_POCKLEKUL, WeaponShotForm::Weapon, SummonSmokeEffect(), kSmokeShots, kSummonSmokeFile),
     RegisterWeaponShotEffect(ITEM_WEAPON_POCKLEKUL, WeaponShotForm::Sphere, SummonSmokeEffect(), kSmokeShots, kSummonSmokeFile), true);

float FlatDistance2(const float *a, const float *b) {
    float dx = a[0] - b[0], dz = a[2] - b[2];
    return dx * dx + dz * dz;
}

void Gone(const char *why) {
    if (pockle.up) {
        std::fprintf(stderr, "pocklekul: the pockle goes (%s)\n", why);
        LogEvent("pockle", std::format("\"step\":\"gone\",\"why\":\"{}\"", why));
    }
    AllyDismiss();
    FightsEnd();
    pockle.up = false;
    pockle.target = -1;
    pockle.parked = false;
    pockle.settling = 0;
    pockle.tinting = false;
    pockle.pinned = false;
}

// The weapon or its summoner gone: a standing pockle goes, and its smoke with it.
void Disarm() {
    if (pockle.up) {
        Gone("the Pocklekul left the hand");
    }
    SummonSmokeEnd(&pockle);
    pockle.chara = -1;
    pockle.guard.Reset();
}

// Its colour over the room's light: its palette's target the room's plus the tint, held whole until it is grown.
void TintStart(MONSTOR &live) {
    sceVu0FVECTOR ambient;
    MGGetAmbient(ambient);
    for (int i = 0; i < 3; i++) {
        live.palette_target[i] = ambient[i] + kPockleTint[i];
    }
    live.palette_step = 0.0f;
    live.palette_blend = 1.0f;
    live.palette_cycles = 1;
    pockle.tinting = true;
}

// Grown (the smoke's frame kPockleGrownFrame) or with no smoke, it fades back to the room's light by the smoke's end.
void Tint(MONSTOR &live) {
    if (!pockle.tinting) {
        return;
    }
    float frame = SummonSmokeFrame();
    if (frame >= 0.0f && frame < kPockleGrownFrame) {
        return;
    }
    live.palette_step = frame < 0.0f ? -1.0f : -kSmokeRate / (kSmokeClipEnd - kPockleGrownFrame);
    pockle.tinting = false;
}

void Summon() {
    int   kind = ModRoll(static_cast<int>(kPockleKinds.size()));
    float yaw = PlayerYaw();
    float spawn[4] = {CharaMain.pos[0] + std::sin(yaw) * kPockleSpawnAhead, CharaMain.pos[1], CharaMain.pos[2] + std::cos(yaw) * kPockleSpawnAhead, 1.0f};
    ChargeCompleteFlash();
    sceVu0CopyVector(pockle.point, spawn); // where its script finds the player as it is set up
    if (!AllySummon(No(kPockleKinds[kind]), spawn)) {
        std::fprintf(stderr, "pocklekul: no summon (%s did not load or stand)\n", kPockleNames[kind]);
        return;
    }
    CMonstorUnit &unit = *AllyStanding();
    MONSTOR      &live = unit.monster[kAllyMonster];
    // No drops but the money it steals, and no ABS.
    live.drops_items = 0;
    live.exp = 0;
    live.stolen_money = 0;
    pockle.up = true;
    pockle.kind = kind;
    pockle.grown = 0;
    pockle.target = -1;
    pockle.parked = false;
    pockle.settling = 0;
    FightsBegin();
    TintStart(live);
    pockle.pinned = SummonSmokeStart(spawn, yaw, kPockleSmokeScale, &pockle);
    pockle.notice = 0;
    std::fprintf(stderr, "pocklekul: %s summoned at (%.1f, %.1f)\n", kPockleNames[kind], spawn[0], spawn[2]);
    LogEvent("pockle", std::format("\"step\":\"summoned\",\"kind\":{},\"life\":{}", No(kPockleKinds[kind]), live.hp));
}

// The notice naming it, once the dungeon's own line (Steve's, say) is gone or has held it kPockleNoticeWait.
void Notice() {
    if (pockle.notice < 0) {
        return;
    }
    if (DngMessMan.message != -1 && DngMessMan.message != kThirstMessage && pockle.notice < kPockleNoticeWait) {
        pockle.notice++;
        return;
    }
    int args[4] = {-1, -1, -1, -1};
    SetSystemMes(kPocklekulMessage, kPockleNoticeFrames, MES_POS_BOTTOM, 0, args, nullptr);
    pockle.notice = -1;
}

// The guard held kPockleGuardFrames (its first frame counted 0): a summon on the first quiet frame, nothing while one
// stands.
void Charge() {
    pockle.guard.Step(guard_button() && Guarding());
    if (!pockle.guard.Reached(kPockleGuardFrames - 1)) {
        return;
    }
    if (pockle.up) {
        pockle.guard.Latch();
        std::fprintf(stderr, "pocklekul: a pockle already stands\n");
        return;
    }
    if (!QuietFloor()) {
        return;
    }
    pockle.guard.Latch();
    Summon();
}

// A floor monster it may go for within range² of the summoner (named, on the floor and alive): its distance² there.
bool Candidate(const CMonstorUnit &floor, int monster, float range2, float *distance2) {
    if (monster < 0 || monster >= kFloorMonsters) {
        return false;
    }
    const MONSTOR &live = floor.monster[monster];
    if (live.name_no == 0 || live.name_no == -1 || live.state <= 0 || live.hp <= 0) {
        return false;
    }
    float at[4];
    const_cast<CMonstorUnit &>(floor).chara[monster][0].GetPosition(at);
    *distance2 = FlatDistance2(at, CharaMain.pos);
    return !(range2 < *distance2);
}

int PickTarget(const CMonstorUnit &floor) {
    float distance2;
    if (Candidate(floor, lockOnTargetNo, kPockleLockRange * kPockleLockRange, &distance2)) {
        return lockOnTargetNo;
    }
    if (Candidate(floor, pockle.target, kPockleLockRange * kPockleLockRange, &distance2)) {
        return pockle.target;
    }
    int   nearest = -1;
    float range2 = kPockleNearRange * kPockleNearRange;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (Candidate(floor, monster, range2, &distance2)) {
            range2 = distance2;
            nearest = monster;
        }
    }
    return nearest;
}

// Its point in its enemy: kPockleAimDepth in from the edge of the hurt sphere nearest it toward it, at the enemy's height.
void Aim(CMonstorUnit &floor, int target, const float *at) {
    float root[4];
    floor.chara[target][0].GetPosition(root);
    const MONSTOR_EFFECT_STATE &body = floor.effect[target];
    int                         best = -1;
    float                       best_edge = 1.7e38f;
    for (int i = 0; i < kMonsterSpheres; i++) {
        if (body.timer[i] == 0) {
            continue;
        }
        float edge = std::hypot(at[0] - body.position[i][0], at[2] - body.position[i][2]) - body.radius[i];
        if (edge < best_edge) {
            best_edge = edge;
            best = i;
        }
    }
    sceVu0CopyVector(pockle.point, root);
    if (best < 0) {
        return;
    }
    const float *centre = body.position[best];
    float        dx = at[0] - centre[0], dz = at[2] - centre[2];
    float        distance = std::sqrt(dx * dx + dz * dz);
    float        depth = body.radius[best] - kPockleAimDepth;
    pockle.point[0] = centre[0];
    pockle.point[2] = centre[2];
    if (depth > 0.0f && depth < distance) {
        pockle.point[0] += dx * depth / distance;
        pockle.point[2] += dz * depth / distance;
    }
    pockle.point[3] = 1.0f;
}

// A clip of its own, blended in as any clip change when it changes, at rate (-1 its own).
void Clip(CCharacter &body, int clip, float rate) {
    if (body.motion_no != clip) {
        body.motion_no = clip;
        body.motion_flags = 0;
    }
    body.motion_speed = rate;
}

// No enemy: whether a move of its own is still playing out (its script running, finding the player far past the
// summoner); once it is not, it idles from then on.
bool Settle(CMonstorUnit &unit, const float *at) {
    int clip = unit.chara[kAllyMonster][0].motion_no;
    if (pockle.parked || clip == kPockleIdleClip || clip == kPockleWalkClip || ++pockle.settling >= kPockleSettleFrames) {
        pockle.settling = 0;
        pockle.parked = true;
        return false;
    }
    pockle.point[0] = at[0] + kPockleSettleReach * (CharaMain.pos[0] - at[0]);
    pockle.point[1] = CharaMain.pos[1];
    pockle.point[2] = at[2] + kPockleSettleReach * (CharaMain.pos[2] - at[2]);
    pockle.point[3] = 1.0f;
    return true;
}

// Its script waiting: resumed on a wait each step, its own movement stopped.
void Park(CMonstorUnit &unit) {
    unit.interpreter[kAllyMonster].pc = park_wait;
    unit.script_state[kAllyMonster] = 1;
    unit.monster[kAllyMonster].movement_speed = 0.0f;
}

// Its script afresh from its main label on its next step, its clips at their own rate.
void Release(CMonstorUnit &unit) {
    unit.interpreter[kAllyMonster].pc = nullptr;
    unit.interpreter[kAllyMonster].skip_wait = 0;
    unit.script_state[kAllyMonster] = 0;
    unit.chara[kAllyMonster][0].motion_speed = -1.0f;
}

// Beside the summoner: the point kPockleSideOffset to their right, walked to or idled at.
void Follow(CMonstorUnit &unit) {
    CCharacter &body = unit.chara[kAllyMonster][0];
    MONSTOR    &live = unit.monster[kAllyMonster];
    float       yaw = PlayerYaw();
    float       c = std::cos(yaw), s = std::sin(yaw);
    pockle.point[0] = CharaMain.pos[0] - c * kPockleSideOffset;
    pockle.point[1] = CharaMain.pos[1];
    pockle.point[2] = CharaMain.pos[2] + s * kPockleSideOffset;
    pockle.point[3] = 1.0f;
    float at[4];
    body.GetPosition(at);
    float dx = pockle.point[0] - at[0], dz = pockle.point[2] - at[2];
    float distance = std::sqrt(dx * dx + dz * dz);
    float face[2];
    if (!(kPockleIdleReach < distance)) {
        Clip(body, kPockleIdleClip, -1.0f);
        face[0] = at[0] + s * kPockleIdleLook;
        face[1] = at[2] + c * kPockleIdleLook;
    } else {
        float pace = distance * kPockleWalkPace;
        if (pace < kPockleMinPace) {
            pace = kPockleMinPace;
        } else if (kPockleMaxPace < pace) {
            pace = kPockleMaxPace;
        }
        float step = distance / 6.0f < pace ? distance / 6.0f : pace;
        at[0] += dx * (step / distance);
        at[2] += dz * (step / distance);
        body.SetPosition(at);
        if (CharaMain.motion_no == kSummonerRunMotion) {
            Clip(body, kPockleRunClip, -1.0f);
        } else {
            Clip(body, kPockleWalkClip, pace * kPockleWalkRate);
        }
        face[0] = pockle.point[0];
        face[1] = pockle.point[2];
    }
    live.turn_target[0] = face[0];
    live.turn_target[1] = at[1];
    live.turn_target[2] = face[1];
    live.turn_speed = kPockleTurnRate;
}

// A step of one standing: its end, its growth and smoke, its enemy or its walk.
void Mind() {
    CMonstorUnit *unit = AllyStanding();
    CMonstorUnit *floor = AllyFloor();
    if (unit == nullptr || floor == nullptr) {
        Gone("its unit was taken down");
        return;
    }
    if (BtEventMode != 0) {
        Gone("an event took the floor");
        return;
    }
    MONSTOR &live = unit->monster[kAllyMonster];
    if (live.state == -1) {
        Gone("it fell");
        return;
    }
    FightsStep();
    CCharacter &body = unit->chara[kAllyMonster][0];
    if (pockle.grown < static_cast<int>(kPockleGrowth.size())) {
        float scale = kPockleGrowth[pockle.grown++];
        body.SetScale(scale, scale, scale);
    }
    Tint(live);
    float at[4];
    body.GetPosition(at);
    if (pockle.pinned) {
        pockle.pinned = SummonSmokePlaying();
        if (pockle.pinned) {
            SummonSmokeMove(at, &pockle);
        }
    }
    if (live.hp <= 0) {
        FightsEnd(); // the monsters it provoked back on the player as it falls; its death plays out
        return;
    }
    if (kPockleLeash * kPockleLeash < FlatDistance2(at, CharaMain.pos)) {
        Gone("left behind");
        return;
    }
    int target = PickTarget(*floor);
    if (target != pockle.target && target >= 0) {
        LogEvent("pockle", std::format("\"step\":\"target\",\"monster\":{}", target));
    }
    pockle.target = target;
    if (target >= 0) {
        if (pockle.parked) {
            pockle.parked = false;
            Release(*unit);
        }
        Aim(*floor, target, at);
        return;
    }
    if (Settle(*unit, at)) {
        return;
    }
    Park(*unit);
    Follow(*unit);
}

// The next floor's kinds being set up: whatever stood went with the last floor (PocklekulReset took it down).
void OnFloor() {
    AllyForget();
    FightsBegin();
    pockle = Pockle{};
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

} // namespace

bool PockleUp() {
    return pockle.up;
}

Monster PockleKind() {
    return pockle.kind >= 0 ? kPockleKinds[pockle.kind] : Monster::SUNDAY;
}

void PocklekulStep(int pause) {
    if (UserStatus == nullptr || NowMonstorUnit == nullptr || AllyStepping()) {
        return;
    }
    bool active = Active();
    if (pockle.chara >= 0 && (!active || UserStatus->cur_chara != pockle.chara)) {
        Disarm();
    }
    if (!active) {
        return;
    }
    pockle.chara = UserStatus->cur_chara;
    if (pause != 0) {
        return;
    }
    SummonSmokeStep();
    Charge();
    Notice();
    if (pockle.up) {
        Mind();
    }
}

void PocklekulShotsStep() {
    if (pockle.up) {
        PockleShots();
    }
}

bool PocklekulScriptTarget(float *position) {
    if (AllyStepping()) {
        sceVu0CopyVector(position, pockle.point);
        return true;
    }
    return pockle.up && NowMonstorUnit != nullptr && PockleProvoked(NowMonstorUnit->current_monster, position);
}

void PocklekulReset() {
    Gone("the floor was put back");
    SummonSmokeEnd(&pockle);
    pockle = Pockle{};
}

void SetPocklekulGuardButton(PockleGuardButton held) {
    guard_button = held != nullptr ? held : ControllerGuard;
}

} // namespace expanded
