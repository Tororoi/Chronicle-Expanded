#pragma once

#include "userstatus.hpp"

namespace dc::game {
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Brave Ark "Hero's Courage": the character holding the Brave Ark, or Super Steve carrying its SynthSphere, takes no
// Freeze, Poison, Curse or Goo. A monster's hit carrying one does not give it (no message, and no amulet used up), no
// trap or script gives it, and one already on, or given by an item, is cleared on the next step. It also has the
// Dusack's No Fool's Gold (dusack.hpp).
inline constexpr int kHerosCourage = AILMENT_FREEZE | AILMENT_POISON | AILMENT_CURSE | AILMENT_GOO;

// The ailment bits of a monster's hit (COLLISION_HIT::flags) as BtCheckDamageProc reads them: Freeze, Freeze
// unless an Anti-Freeze Amulet stops it, then Poison, Curse and Goo unless their amulets do.
inline constexpr int kHitFreeze = 0x100000;
inline constexpr int kHitFreezeAmulet = 0x100;
inline constexpr int kHitPoison = 0x200;
inline constexpr int kHitCurse = 0x400;
inline constexpr int kHitGoo = 0x800;
inline constexpr int kHerosCourageHit = kHitFreeze | kHitFreezeAmulet | kHitPoison | kHitCurse | kHitGoo;

// Whether chara's equipped weapon gives Hero's Courage.
bool HerosCourage(int chara);

// Hook: a monster's hit reaching the active character, before its ailments are rolled.
void HerosCourageHit(COLLISION_HIT &hit);

// Hook: BtSetStatusErr: whether ailment may be set on the active character.
bool HerosCourageAllowsAilment(int ailment);

// Hook: a dungeon step: clears the four from every member who has it.
void HerosCourageStep();

} // namespace expanded
