#pragma once

namespace dc::game {
class CFrame;
class CMonstorUnit;
struct COLLISION_HIT;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Macho Sword "Overtraining": while a Macho Sword is owned (in hand, a bag or storage), every weapon's ABS keeps
// filling past its max, up to twice it, and on a level-up the surplus over the max carries into the new level (owned
// or not, so a weapon filled before the sword left still keeps it).
inline constexpr int kOvertrainingLimit = 2;

// Hooks: CMonstorUnit::Step paying a kill's abs into the active character's weapon (true when it paid it: the
// sword owned), and CWeaponLevelUp::SetLevelUpWeaponData's ABS for the level it opens (max_abs the level's it leaves).
bool OvertrainingAbs(WEAPON_HAVE &weapon, int abs, int max_abs);
int  OvertrainingCarry(const WEAPON_HAVE &weapon, int max_abs);

// Hook: the dungeon HUD's ABS gauge, full at the max however far past it the weapon has filled.
int OvertrainingGaugeAbs(int abs, int max_abs);

// Macho Sword "Pumped Up": every monster that goes down while a member has the Macho Sword in hand (or Super Steve
// carrying its SynthSphere) pumps that member up: each one multiplies the weapon's attack by kPumpedUpRate, without
// limit, and a monster that gets back up (a Mummy, a Gacious) counts again each time it goes down. Toan's arms swell
// with it, bone by bone (his upper arms, forearms and wrists thicker across their length, his hands larger about the
// wrist; the glove cuffs keep their size), evenly from nothing to kPumpedUpLook at kPumpedUpFullLook monsters, and stay
// there however far the attack goes on. It holds through the back floor and back and through changes of member, and is
// gone on a new floor, a new visit, or as soon as the member's equipped weapon changes.
inline constexpr float kPumpedUpRate = 1.15f;
inline constexpr int   kPumpedUpFullLook = 15;

// Toan's arm at its fullest: each part's size across (a hand's whole size), and the bones and frames it is drawn by
// (right, left).
enum PumpedUpPart {
    kPumpedUpper,
    kPumpedForearm,
    kPumpedWrist,
    kPumpedHand,
    kPumpedParts
};

inline constexpr float       kPumpedUpLook[kPumpedParts] = {1.6f, 2.02f, 1.54f, 1.6f};
inline constexpr const char *kPumpedUpBones[kPumpedParts - 1][2] = {
    {"jnt17_1", "jnt9_1" },
    {"jnt17_2", "jnt9_2" },
    {"jnt18_1", "jnt10_1"},
};
inline constexpr const char *kPumpedUpHands[] = {"r_hand", "l_hand", "renzu__m"};

// The monsters a member has downed on this floor with the weapon it holds now (0 once it changed).
int PumpedUpKills(int chara);
// The active member's attack multiplier.
float PumpedUpMultiplier();
// The size of a part of Toan's arm now (1 when not pumped up).
float PumpedUpSize(PumpedUpPart part);

// Hooks: a monster's life ran out (CheckDmg); a new floor's monsters are being placed (BtArrengeMonstor), and a new
// visit (GameInit); the top of a dungeon frame (a member's equipped weapon checked); Toan's swing's attack
// (ToanKey_Play); a Super Steve pellet's hit as it is set; Toan's model posed (SetMotionEX, before the skin: his hands'
// frames); and a bone's matrix as the skin takes it (MotionProc2, after the bone passed it unchanged to its children).
void PumpedUpKill(const CMonstorUnit &unit, int monster);
void PumpedUpReset();
void PumpedUpFrame();
int  PumpedUpAttack(int attack);
void PumpedUpPelletHit(COLLISION_HIT &hit);
void PumpedUpPosed(CFrame *frame);
void PumpedUpSkinBone(CFrame *frame, int bone, float (*matrix)[4]);

} // namespace expanded
