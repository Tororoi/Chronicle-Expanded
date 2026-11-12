#pragma once

namespace dc::game {
class CCharacter;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Big Bucks Hammer's curse, Midas Touch's price: its wielder is cursed while it is equipped (curse.hpp): Goro, and Xiao
// with its SynthSphere on Super Steve (her own, the same in every way). Midas Touch turns monsters only while the one
// in play is (midas_touch.hpp). When the curse is lifted, by an item or by the hit that would have knocked them out
// (which leaves them 1 life), they turn to gold: frozen, with no end to it (their freeze's frames 0) until an item heals
// the freeze, drawn as polished gold in the dungeon and on the item menu's model, and untouchable: every monster's hit
// and shot meets them as a guard that is never broken (the guard-breaking ones as a knockback, as 7th Heaven's Divine
// Guard has them) and that never moves them, the monster that struck within kGoldRepelReach thrown back instead
// (kGoldRepelSpeed, falling by kGoldRepelDecay a step, as its own knockback scales them), and no ailment takes. Once
// lifted, the curse stays off until the next floor.

inline constexpr float kGoldRepelReach = 40.0f;
inline constexpr float kGoldRepelSpeed = 2.0f;
inline constexpr float kGoldRepelDecay = 0.3f;

// Whether chara (CHARA_*) carries the curse now.
bool MidasCursed(int chara);

// Whether chara is gold now.
bool MidasGold(int chara);

// Turns chara to gold (the curse lifted).
void TurnGold(int chara);

// Hooks: a dungeon step (gold kept, and ended once a freeze is healed); the damage a monster's hit will deal the player
// (the hit that would knock a cursed one out leaves 1 life and turns them); a hit reaching the player (gold: the
// guard-breaking one comes in as a knockback, and the hit never breaks the freeze); whether the player shrugs a hit off
// as a guard would; whether an ailment may be set on the player.
void MidasCurseStep();
int  MidasPlayerDamage(int damage);
void MidasPlayerHit(COLLISION_HIT &hit);
bool MidasShrugs();
// Hook: a monster's hit on the player reacted to (PlayerHitReacted): gold, the player is left where they stood and the
// monster is thrown back.
void MidasPlayerReacted(const COLLISION_HIT &hit);
bool MidasAllowsAilment();

// Hooks: the player's model and weapon drawing and drawn in the dungeon (Draw_MainUnit), and the item menu's model of
// chara (CHARA_*) drawing and drawn.
void PlayerGoldDrawing();
void PlayerGoldDrawn();
void MenuCharaDrawing(int chara);
void MenuCharaDrawn(int chara);

// A gold player's light (character_light.cpp, through MidasLight): its turning flash, and its shine, on the player's
// model and the item menu's model of a gold one; false for any other character.
bool MidasGoldLight(const CCharacter &character, float *ambient);

// Forgets the floor's lifts and every gold (tests).
void MidasCurseReset();

} // namespace expanded
