#pragma once

namespace dc::game {
class CCharacter;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Big Bucks Hammer's curse, Midas Touch's price: Goro is cursed while it is equipped (curse.hpp), and Midas Touch turns
// monsters only while he is (midas_touch.hpp). When the curse is lifted, by an item or by the hit that would have
// knocked him out (which leaves him 1 life), Goro turns to gold: frozen, with no end to it (his freeze's frames 0)
// until an item heals the freeze, drawn as polished gold in the dungeon and on the item menu's model, and untouchable:
// every monster's hit and shot meets him as a guard that is never broken (the guard-breaking ones as a knockback, as
// 7th Heaven's Divine Guard has them) and that never moves him, the monster that struck within kGoldRepelReach thrown
// back from him instead (kGoldRepelSpeed, falling by kGoldRepelDecay a step, as its own knockback scales them), and no
// ailment takes. Once lifted, the curse stays off until the next floor.

inline constexpr float kGoldRepelReach = 40.0f;
inline constexpr float kGoldRepelSpeed = 2.0f;
inline constexpr float kGoldRepelDecay = 0.3f;

// Whether Goro carries the curse now.
bool MidasCursed();

// Whether Goro is gold now.
bool GoroGold();

// Turns Goro to gold (the curse lifted).
void TurnGoroGold();

// Hooks: a dungeon step (gold kept, and ended once his freeze is healed); the damage a monster's hit will deal the
// player (the hit that would knock cursed Goro out leaves him 1 life and turns him); a hit reaching the player (gold:
// the guard-breaking one comes in as a knockback, and the hit never breaks his freeze); whether the player shrugs a hit
// off as a guard would; whether an ailment may be set on the player.
void MidasCurseStep();
int  MidasPlayerDamage(int damage);
void MidasPlayerHit(COLLISION_HIT &hit);
bool MidasShrugs();
// Hook: a monster's hit on the player reacted to (PlayerHitReacted): gold Goro left where he stood, the monster thrown back.
void MidasPlayerReacted(const COLLISION_HIT &hit);
bool MidasAllowsAilment();

// Hooks: Goro's model and weapon drawing and drawn in the dungeon (Draw_MainUnit), and the item menu's model of chara
// (CHARA_*) drawing and drawn.
void GoroDrawing();
void GoroDrawn();
void MenuCharaDrawing(int chara);
void MenuCharaDrawn(int chara);

// Gold Goro's turning flash added to his light (character_light.cpp).
void GoroGoldLight(const CCharacter &character, float *ambient);

// Forgets the floor's lift and Goro's gold (tests).
void MidasCurseReset();

} // namespace expanded
