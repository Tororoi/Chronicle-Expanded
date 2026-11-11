#include "expanded/weapons/goro/inferno.hpp"

#include <algorithm>
#include <cmath>

#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/status/poison.hpp"
#include "expanded/weapons/blast.hpp"
#include "expanded/weapons/goro/inferno/rising_flame.hpp"
#include "expanded/weapons/goro/satans_ax.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/ungaga/mirage/heat_haze.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"
#include "weaponelement.hpp"

namespace expanded {

namespace {

int pulled_frames = 0; // the pull on, without a break
int red_frames = 0;    // of kBurningAuraFogFrames

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(BurningAuraReset), true);

// Goro's own pull, not Xiao's pellet's.
bool Pulling() {
    return InfernoWielded() && UnderworldStrength() > 0.0f;
}

void Burn(int seconds) {
    if (NowMonstorUnit == nullptr) {
        return;
    }
    int spared = 0;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        if (!UnderworldPulling(monster)) {
            spared |= 1 << monster;
        }
    }
    if (spared == (1 << kFloorMonsters) - 1) {
        return;
    }
    BlastOptions options;
    options.damage = kBurningAuraStep * static_cast<float>(seconds);
    options.reach = 2.0f * kUnderworldReach / kBlastFalloff[0].reach; // every monster it draws, and more
    options.kick = 0.0f;
    options.spared = spared;
    options.element = WEAPON_ELEMENT_FIRE;
    options.flat = true;
    options.rumble = false;
    PlantBlast(*NowMonstorUnit, CharaMain.pos, NowWeaponHave->attack, options);
}

// A colour moved red of the way to the fog's red.
void Redden(unsigned char *colour, float red) {
    for (int c = 0; c < 3; c++) {
        float own = colour[c];
        colour[c] = static_cast<unsigned char>(std::lround(own + (kBurningAuraFog[c] - own) * red));
    }
}

} // namespace

bool InfernoWielded() {
    return Playing(CHARA_GORO) && WeaponWielded(ITEM_WEAPON_INFERNO);
}

void BurningAuraStep() {
    if (!MonstersStepping()) {
        return;
    }
    RisingFlameStep(Pulling(), Pulling() && UnderworldStrength() >= 1.0f);
    if (!Pulling()) {
        pulled_frames = 0;
        red_frames = std::max(0, red_frames - kBurningAuraFogFrames / kBurningAuraFogFade);
        if (BurningAuraHaze() > 0.0f) {
            mirage::HazeStep();
        }
        return;
    }
    red_frames = std::min(kBurningAuraFogFrames, red_frames + 1);
    if (BurningAuraHaze() > 0.0f) {
        mirage::HazeStep();
    }
    if (++pulled_frames % kBurningAuraFrames == 0) {
        Burn(pulled_frames / kBurningAuraFrames);
    }
}

int BurningAuraSeconds() {
    return pulled_frames / kBurningAuraFrames;
}

float BurningAuraRed() {
    return static_cast<float>(red_frames) / kBurningAuraFogFrames;
}

float BurningAuraHaze() {
    return std::clamp((BurningAuraRed() - kBurningAuraHazeFrom) / (1.0f - kBurningAuraHazeFrom), 0.0f, 1.0f);
}

bool InfernoFireproof(const COLLISION_HIT &hit) {
    return InfernoWielded() && (hit.flags & kFireElementBit) != 0;
}

bool BurningAuraFeared(CMonstorUnit &unit, int monster) {
    if (!Pulling() || &unit != NowMonstorUnit || !UnderworldPulling(monster)) {
        return false;
    }
    float at[4];
    unit.chara[monster][0].GetPosition(at);
    return std::hypot(at[0] - CharaMain.pos[0], at[2] - CharaMain.pos[2]) > kBurningAuraBrave;
}

void BurningAuraLight(float *ambient, float (*colours)[4]) {
    float red = BurningAuraRed();
    if (red <= 0.0f) {
        return;
    }
    for (int c = 0; c < 3; c++) {
        float scale = 1.0f + (kBurningAuraLight[c] - 1.0f) * red;
        ambient[c] = std::min(255.0f, ambient[c] * scale);
        for (int row = 0; row < 4; row++) {
            colours[row][c] = std::min(255.0f, colours[row][c] * scale);
        }
    }
}

void BurningAuraFog(float *fog_rate, unsigned char *fog_colour) {
    float red = BurningAuraRed();
    if (red <= 0.0f) {
        return;
    }
    // A floor without fog keeps none: its colour alone carries the red.
    if (fog_rate[1] > fog_rate[0]) {
        float pull = 1.0f + (kBurningAuraFogPull - 1.0f) * red;
        fog_rate[0] *= pull;
        fog_rate[1] *= pull;
    }
    Redden(fog_colour, red);
}

void BurningAuraBackground(unsigned char *background) {
    Redden(background, BurningAuraRed());
}

void BurningAuraFlameDraw() {
    if (InfernoWielded()) {
        RisingFlameDraw();
    }
}

void BurningAuraHazeDraw() {
    if (BtEventMode == 0) {
        mirage::HazeDrawOver(CharaMain.pos, BurningAuraHaze(), kBurningAuraHazeSway);
    }
}

void BurningAuraReset() {
    RisingFlameReset();
    pulled_frames = 0;
    red_frames = 0;
}

} // namespace expanded
