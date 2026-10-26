#pragma once

class CSHOT;

namespace expanded {

// Halberd "Tornado Charge Buff": with a weapon of the Halberd line in Ungaga's hand, the shot his held charge fires
// every 30 frames (UngagaKey_Play) flies faster and is drawn larger, its hit as much wider, at a tier that grows down
// the line. The shot's speed in each phase is the tier's travel times its own, so over its fixed flight it also goes
// that much farther; its model and the radius of its hit in each phase are the tier's size times their own. Ungaga's
// own charge is untouched. Super Steve carrying a sphere of the line takes the tier on a charged shot: the shot held
// drawn kTornadoShotFrames (the game's charge-complete flash; the shot costs a charged shot's weapon HP) lets go a
// pellet drawn the tier's size times larger, flying its travel times faster and hitting for kTornadoShotDamage times
// the damage. Its hit keeps the pellet's own radius. Times are game frames.
struct TornadoTier {
    float travel; // the shot's speed in every phase
    float size;   // its model and its hit's radius
};

inline constexpr TornadoTier kTornadoHalberd = {1.4f, 1.1f};
inline constexpr TornadoTier kTornadoScorpion = {1.7f, 1.3f};
inline constexpr TornadoTier kTornadoMirage = {2.0f, 1.5f};   // and the Cactus
inline constexpr TornadoTier kTornadoHercules = {2.5f, 1.7f}; // Hercules' Wrath, the Terra Sword and Babel's Spear
inline constexpr int         kTornadoShotFrames = 30;
inline constexpr float       kTornadoShotDamage = 1.5f;

// The tier a weapon of the line carries, or nullptr.
const TornadoTier *TornadoTierOf(int weapon_no);

// The tier Ungaga's charge shot flies at now (his weapon's), or nullptr; the tier Super Steve's sphere gives Xiao's
// charged shot, or nullptr.
const TornadoTier *TornadoChargeTier();
const TornadoTier *TornadoShotTier();

// Hooks: a dungeon step (Ungaga's charge effect put at his weapon's tier or back to its own, Super Steve's charge
// counted); a pellet let go into slot (BattleActionPlay_Jinn).
void TornadoChargeStep();
void TornadoShotFired(CSHOT &shots, int slot);

} // namespace expanded
