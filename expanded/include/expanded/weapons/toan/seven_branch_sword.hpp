#pragma once

namespace dc::game {
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// 7 Branch Sword "Sevenfold Rite": it refuses a Status Break below +kSevenfoldLevel (the menu offers it from
// level 5, as for any weapon, and warns when it is chosen), and its SynthSphere keeps kSevenfoldShare of its stats,
// elements and anti-monster values rather than the game's 60%. With one selected, the weapon menu's Status Break
// hint and confirmation say so. Its Swift Strikes (shamshir.hpp) and Fine Fare (sax.hpp) live with those weapons.
inline constexpr int   kSevenfoldLevel = 7;
inline constexpr float kSevenfoldShare = 0.77f;
inline constexpr float kStatusBreakShare = 0.6f;

// The weapon menu's warning for a refused break (WepMenu.warning_no, past the game's ten) and its allmenu message.
inline constexpr int kSevenfoldWarning = 10;
inline constexpr int kSevenfoldRefusedMessage = 4200;

// The allmenu messages of the Status Break hint and its confirmation.
inline constexpr int kStatusBreakHintMessage = 0x6D;
inline constexpr int kStatusBreakConfirmMessage = 0x76;

// Whether a Status Break of weapon is refused.
bool SevenfoldRefuses(const WEAPON_HAVE &weapon);

// The share of weapon's stats a Status Break keeps (SetStatusBreak).
float StatusBreakShare(const WEAPON_HAVE &weapon);

} // namespace expanded
