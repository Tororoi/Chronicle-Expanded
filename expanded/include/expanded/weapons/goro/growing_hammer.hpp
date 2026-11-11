#pragma once

#include "itemdata.hpp"

namespace dc::game {
class CSHOT_EFFECT;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

// Big Bucks Hammer's Growing Hammer, and the hammers it builds up into (Magical Hammer, Gaia Hammer, Last Judgement and
// the Inferno: kGrowingHammers): while Goro holds the guard with one in hand the hammer grows, evenly, to its full size
// (the hammer's tier's, kGrowingHammers) over kGrowingHammerGrowFrames; let go before it is full and it shrinks back at
// the same rate (over as long as it grew). Full (the game's charge-complete flash on the party as it gets there), it
// keeps that size kGrowingHammerFullFrames, then shrinks back as it grew, and grows again only from a fresh hold of the
// guard. Its size carries his swings' hits out with the hammer's head and widens them, his charge attack's ground
// impact and that impact's reach with it, and his attack by as much. Super Steve's spheres lend none of it. Times are
// game frames.
struct GrowingHammerTier {
    int   weapon_no;
    float full; // times its own size
};

inline constexpr GrowingHammerTier kGrowingHammers[] = {
    {ITEM_WEAPON_BIG_BUCKS_HAMMER, 1.3f},
    {ITEM_WEAPON_MAGICAL_HAMMER,   1.4f},
    {ITEM_WEAPON_GAIA_HAMMER,      1.5f},
    {ITEM_WEAPON_LAST_JUDGEMENT,   1.5f},
    {ITEM_WEAPON_INFERNO,          1.5f},
};
inline constexpr int kGrowingHammerGrowFrames = 120;
inline constexpr int kGrowingHammerFullFrames = 900;

// Whether Goro is in play with one of kGrowingHammers in hand, and its full size (1 with none).
bool  GrowingHammerWielded();
float GrowingHammerFull();

// The hammer's size now (1 its own), and what it makes of his attack and a swing's hit radius.
float GrowingHammerScale();
int   GrowingHammerAttack(int attack);
float GrowingHammerHitRadius(float radius);

// Hooks: a step (with the monsters'); his charge attack's impact set in slot of effect (GoroKey_Play); a shot effect
// planting its hit (ShotEffectHitSet), the impact's reaching its size times as far.
void GrowingHammerStep();
void GrowingHammerImpact(CSHOT_EFFECT &effect, int slot);
void GrowingHammerHitSet(const CSHOT_EFFECT &effect, int slot, COLLISION_HIT &hit);

void GrowingHammerReset();

} // namespace expanded
