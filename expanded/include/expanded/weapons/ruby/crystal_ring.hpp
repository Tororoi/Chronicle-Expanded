#pragma once

namespace dc::game {
class CMonstorUnit;
class CSHOT_EFFECT;
class CUserStatus;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Crystal Ring: a cursed ring. Equipped, it curses Ruby (weapons/curse.hpp), again on every floor and after a revival.
// An item lifting the curse costs nothing: her own shots and her choice of element are back until the floor ends or the
// ring leaves her hand.
//
// Until then the ring is held to Ice: the element picker offers Ice alone (and opens to show it), the weapon menu
// refuses any other element with the curse's warning in place of the 0-point one, and every Crystal Ring she carries
// keeps Ice selected, so the game's own loads of her per-element shot load the Ice one and the ball she charges is ice.
// A build-up into the ring in the dungeon's menu reloads her shot, as equipping it does.
//
// Cursed with the ring in her hand, every orb she lets go (her quick shot, a part charge's, a full charge's) flies on
// as the Crystal Gemron's ice arrow (enemies/ice_shots.hpp) at the orb's damage, along the orb's flight at the
// arrow's own speed and for its life, turned on the monsters with only its Ice element on its hit, her weapon's
// abilities riding on it as on the orb. A charged throw's arrow (a part or full charge) flies at kPrismChargedSpeed, a
// little faster than a quick shot's. A full charge's second orb, which the game draws shimmering over the first,
// goes out with no arrow of its own. Each frame of its flight an arrow turns kPrismTurnDegrees toward its target
// wherever that lies, so a miss comes round again: the locked-on monster, else the living monster nearest the arrow
// within kPrismMonsterReach, else her door crystal nearest it within kPrismCrystalReach. A monster's target is its
// lock-on point, or kPrismAimLift above its feet when it has none.
//
// Prism: an arrow's hit carries the floor's door key, so it opens a door crystal of any colour. An arrow plants its hit
// where it strikes, then keeps quiet kPrismStrikeWait frames; while every arrow in flight is on a crystal, the arrows
// plant a hit of kPrismCrystalRadius every frame of their flight, as a crystal opens on a hit landing near it.
// Super Steve has no form of it. Times are game frames.
inline constexpr float kPrismChargedSpeed = 1.5f;
inline constexpr float kPrismTurnDegrees = 1.5f;
inline constexpr float kPrismMonsterReach = 200.0f;
inline constexpr float kPrismCrystalReach = 250.0f;
inline constexpr float kPrismAimLift = 8.0f;
inline constexpr int   kPrismArrows = 6;
inline constexpr int   kPrismStrikeWait = 120;
inline constexpr float kPrismCrystalRadius = 1.0f;

// Whether Ruby is out with the ring in her hand, cursed, her orbs her weapon's own (not a broken one's).
bool CursedPrismActive();

// The element a curse holds weapon to (Ice for a Crystal Ring whose curse is not lifted), or -1.
int HeldElement(const WEAPON_HAVE &weapon);

// Whether the weapon menu refuses element on weapon: one its curse does not hold it to.
bool ElementRefused(const WEAPON_HAVE &weapon, int element);

// The arrows' effect, or null while it is not loaded.
CSHOT_EFFECT *PrismArrows();

// Every Crystal Ring of Ruby's in status, and the one in her hand, held to Ice while the curse holds them.
void HoldCrystalRings(CUserStatus &status);

// Hooks: an orb of Ruby's let go into slot of effect (BattleActionShotRuby); a dungeon step, before the shot effects
// step (the rings held, the arrows steered); a floor ending (a lifted curse with it); a frame of the weapon menu (the
// rings held); a weapon built up there, once its effect is done.
void PrismOrbThrown(CSHOT_EFFECT &effect, int slot, float scale, bool repeat);
void CursedPrismStep(CMonstorUnit &unit);
void CursedPrismFloorEnds();
void CrystalRingMenuStep();
void CrystalRingBuiltUp(WEAPON_HAVE &weapon);

} // namespace expanded
