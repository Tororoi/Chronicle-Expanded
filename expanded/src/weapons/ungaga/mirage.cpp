#include "expanded/weapons/ungaga/mirage.hpp"

#include <algorithm>
#include <cstdio>
#include <initializer_list>

#include "btactstatus.hpp"
#include "character.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/charge.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/ungaga/mirage/clone.hpp"
#include "expanded/weapons/ungaga/mirage/heat_haze.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

namespace {

using namespace mirage;

struct Decoy {
    bool  up = false;
    int   chara = -1; // the party member who planted it
    int   age = 0;    // frames it has stood
    int   life = kMirageFrames;
    float at[4] = {}; // where it was planted
    float yaw = 0.0f;
    bool  handoff = false;
    int   handoff_age = 0;          // frames since the hand-off began
    float old_at[4] = {};           // the old decoy's spot, its copy dissolving there
    int   hold = 0;                 // frames left the monsters keep to the old spot
    bool  hit[kFloorMonsters] = {}; // the monsters hit since they last took to a decoy
};

Decoy     decoy;
GuardHold guard; // latched once it plants (or hands off): no more until R1 is let go

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(MirageReset), true);

float PlayerYaw() {
    float rotation[4];
    CharaMain.GetRotation(rotation);
    return rotation[1];
}

bool Live(int monster) {
    const MONSTOR *live = FloorMonster(monster);
    return live != nullptr && MonsterLiving(*live);
}

// The clock runs from planting: alpha and gain from the frames it has stood and those it has left.
float Envelope(int lead, int fade) {
    float a = 1.0f;
    float in = static_cast<float>(decoy.age - lead);
    float out = static_cast<float>(decoy.life - decoy.age - lead);
    if (in < fade) {
        a = in / fade;
    }
    if (out < fade) {
        a = std::min(a, out / fade);
    }
    return std::clamp(a, 0.0f, 1.0f);
}

void End(const char *why) {
    if (decoy.up) {
        std::fprintf(stderr, "mirage: the decoy goes (%s)\n", why);
    }
    decoy = Decoy{};
    CloneDown();
}

// Every monster takes to the decoy again, those that were hit too.
void Retarget() {
    std::fill(std::begin(decoy.hit), std::end(decoy.hit), false);
}

// A decoy at the player's spot, standing from now: its time, its shimmer and its lure all start here.
void PlaceAtPlayer() {
    sceVu0CopyVector(decoy.at, CharaMain.pos);
    decoy.at[3] = 1.0f;
    decoy.yaw = PlayerYaw();
    decoy.up = true;
    decoy.chara = UserStatus->cur_chara;
    decoy.age = 0;
    decoy.life = WeaponActive(ITEM_WEAPON_HERCULES_WRATH) ? kHerculesMirageFrames : kMirageFrames;
}

void Plant() {
    PlaceAtPlayer();
    Retarget();
    CloneSpawn(decoy.at, decoy.yaw);
    std::fprintf(stderr, "mirage: decoy planted at (%.1f, %.1f) for %d frames\n", decoy.at[0], decoy.at[2], decoy.life);
}

// A plant with a decoy standing: the new one starts now; the old copy dissolves where it stands, and the monsters keep to the
// old spot (those hit go on after the player) until the new copy has fully appeared.
void HandOff() {
    sceVu0CopyVector(decoy.old_at, decoy.at);
    decoy.handoff = true;
    decoy.handoff_age = 0;
    decoy.hold = kMirageAggroHoldFrames;
    PlaceAtPlayer();
    std::fprintf(stderr, "mirage: handed off to (%.1f, %.1f); the monsters keep to the old spot %d frames\n", decoy.at[0], decoy.at[2],
                 decoy.hold);
}

// A frame of a standing decoy (fresh: planted this frame, its first).
void Update(bool fresh) {
    if (!fresh) {
        decoy.age++;
        if (decoy.handoff) {
            decoy.handoff_age++;
        }
        if (decoy.hold > 0 && --decoy.hold == 0) {
            Retarget();
            std::fprintf(stderr, "mirage: the new copy has appeared; the monsters take to it\n");
        }
    }
    if (decoy.handoff && decoy.handoff_age >= kMirageHandoffFrames) {
        decoy.handoff = false;
        CloneSpawn(decoy.at, decoy.yaw); // the copy back at the new spot, appearing from clear
    }
    if (decoy.age >= decoy.life) {
        End("its time is up");
        return;
    }
    HazeStep();
    CloneStep(MirageCloneAlpha());
}

// Whether the decoy can be planted: the Mirage or Hercules' Wrath in hand, or as Super Steve's sphere.
bool Active() {
    return WeaponActive(ITEM_WEAPON_MIRAGE) || WeaponActive(ITEM_WEAPON_HERCULES_WRATH);
}

bool LineReachActive() {
    for (int weapon : {ITEM_WEAPON_MIRAGE, ITEM_WEAPON_TERRA_SWORD, ITEM_WEAPON_HERCULES_WRATH, ITEM_WEAPON_BABEL_S_SPEAR}) {
        if (WeaponActive(weapon)) {
            return true;
        }
    }
    return false;
}

} // namespace

float MirageLineReachRate(float rate) {
    return LineReachActive() ? rate * kMirageLineReach : rate;
}

void MirageStep() {
    if (UserStatus == nullptr) {
        return;
    }
    // The copy is the planter's model: another party member out takes it down.
    if (decoy.up && UserStatus->cur_chara != decoy.chara) {
        End("the party member changed");
    }
    if (!Active()) {
        if (decoy.up) {
            End("the weapon is put away");
        }
        return;
    }
    CloneLoad(); // ahead of a plant

    guard.Step(GamePad.On(PAD_R1) != 0);
    bool fresh = false;
    if (guard.Reached(kMirageGuardFrames)) {
        ChargeCompleteFlash();
        if (!decoy.handoff) {
            if (decoy.up) {
                HandOff();
            } else {
                Plant();
            }
            fresh = true;
        }
        guard.Latch();
    }
    if (decoy.up) {
        Update(fresh);
    }
}

void MirageHitTaken(int monster) {
    if (decoy.up && monster >= 0 && monster < kFloorMonsters && !decoy.hit[monster]) {
        decoy.hit[monster] = true;
        std::fprintf(stderr, "mirage: monster %d was hit and turns back on the player\n", monster);
    }
}

bool MirageTarget(int monster, float *position) {
    if (!decoy.up || NowMonstorUnit == nullptr || monster < 0 || monster >= kFloorMonsters) {
        return false;
    }
    if (Live(monster) && !decoy.hit[monster]) {
        MirageDecoyTarget(position);
    } else {
        sceVu0CopyVector(position, CharaMain.pos);
    }
    return true;
}

void MirageCloneDraw() {
    CloneDraw();
}

void MirageHazeDraw() {
    float root[3];
    if (decoy.up && BtEventMode == 0 && CloneRoot(root)) {
        HazeDraw(root, MirageHazeGain());
    }
}

bool MirageDecoyUp() {
    return decoy.up;
}

bool MirageHandingOff() {
    return decoy.handoff;
}

void MirageDecoyTarget(float *out) {
    sceVu0CopyVector(out, decoy.hold > 0 ? decoy.old_at : decoy.at);
}

void MirageDecoyPlace(float *out) {
    sceVu0CopyVector(out, decoy.at);
}

int MirageDecoyAge() {
    return decoy.age;
}

int MirageDecoyLife() {
    return decoy.life;
}

float MirageCloneAlpha() {
    if (!decoy.up) {
        return 0.0f;
    }
    if (decoy.handoff) { // the old copy dissolving where it stands; the new one is still clear
        return std::clamp(1.0f - static_cast<float>(decoy.handoff_age) / kMirageHandoffFrames, 0.0f, 1.0f);
    }
    return Envelope(kMirageHazeRampFrames, kMirageFadeFrames);
}

float MirageHazeGain() {
    return decoy.up ? Envelope(0, kMirageHazeRampFrames) : 0.0f;
}

float MirageOutroAlpha() {
    if (!decoy.up) {
        return 0.0f;
    }
    if (decoy.handoff) {
        return 1.0f;
    }
    float out = static_cast<float>(decoy.life - decoy.age - kMirageHazeRampFrames);
    return std::clamp(out / kMirageFadeFrames, 0.0f, 1.0f);
}

void MirageDispel() {
    End("dispelled by the strike");
}

void MirageReset() {
    decoy = Decoy{};
    guard.Reset();
    CloneReset();
    HazeReset();
}

} // namespace expanded
