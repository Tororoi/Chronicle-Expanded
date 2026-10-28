#pragma once

namespace expanded::frozen_tuna {

// The Ice Queen's prison (ice_shots.hpp's kIcePrisonFile) around frozen bodies, every monster of the floor and the
// member in play at once: the one model entered as a floor character on first use, posed and drawn for each body at
// that body's own clip time, place and size. A prison stands where its body froze, on the floor under the body's root
// (its feet), turned to its yaw, scaled across and up about its base by the body's size (prison_scales.hpp); a body
// frozen above the floor (a flyer, a jump) stretches it up by that height over kPrisonHeight, Goro's. Where no floor
// is found under it the prison stands at the root. It forms over the forming clip, holds on its last frame while the
// body stays frozen (a monster's stop timer running, the member's Freeze on), and shatters over the shattering clip, with
// the shatter's sound, once it is not: the freeze run out or broken, the monster dead or gone, the member out of play.
// It all goes with the floor's monsters.
inline constexpr float kPrisonHeight = 14.0f;

// The floor is sought from this far above the root down.
inline constexpr float kPrisonFloorAbove = 5.0f;

enum class PrisonStage {
    kNone,
    kForming,
    kFormed,
    kShattering,
};

struct Prison {
    PrisonStage stage = PrisonStage::kNone;
    float       time = 0.0f;  // in its clip
    float       base[3] = {}; // where it stands
    float       yaw = 0.0f;
    float       horizontal = 1.0f;
    float       vertical = 1.0f;
    int         body = -1; // the monster's slot, or the member (CHARA_*)
};

// Encases a monster of the floor, or the member in play: a prison forms on a body not encased, or one whose prison is
// shattering, its place and size taken as it forms; one forming or formed stays as it is. False when the prison's model
// cannot be loaded. The caller plays the freeze's sounds (PlayIceFreeze), once for all it encases.
bool EncaseMonster(int monster);
bool EncaseMember(int chara);

// A monster's prison, and the member's.
const Prison &MonsterPrison(int monster);
const Prison &MemberPrison();

// Hooks: a dungeon step (clips stepped and freezes watched while the monsters step), the effects' draw, and the floor's
// monsters put back.
void IcePrisonsStep();
void IcePrisonsDraw();
void IcePrisonsClear();

} // namespace expanded::frozen_tuna
