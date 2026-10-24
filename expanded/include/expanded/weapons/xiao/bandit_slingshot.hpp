#pragma once

#include <optional>
#include <string>

class CMonstorUnit;
class CSHOT;
struct BT_SHOT_EFFECT;

namespace expanded {

// Bandit Slingshot "Steal Shot": with the Bandit Slingshot in Xiao's hand, or Super Steve carrying its SynthSphere or
// the Bandit's Ring's, a steal that lands on a monster with a projectile takes that too. Until she steals another or
// the floor ends, every pellet she fires is that monster's shot, as its kind fires it (its flags and element with it)
// turned on the monsters, at kStealShotDamage times the pellet's attack. A kind's first shot is taken, or its second
// when the first is a self-burst (zibaku: the monster bursts where it stands, nothing flies); a kind with neither
// gives the item alone. The stolen item's "acquired" notice gets a second line: "[monster]'s projectile is now yours".
inline constexpr float kStealShotDamage = 2.0f;
inline constexpr int   kStealShots = 6;

bool StealShotActive();

// The shot held, or nullptr.
const BT_SHOT_EFFECT *StealShotHeld();

// System message 10, 20 or 30 (the "acquired" notices), with the second line while the stolen item's is made.
std::optional<std::string> StealShotNoticeText(int message);

// Hooks: CheckDmg's steal from monster (its item now flying to the player); a pellet let go (it is replaced by the
// stolen shot); the stolen item arriving (named: the notice is being made).
void StealShotTaken(const CMonstorUnit &unit, int monster, int item);
void StealShotFired(CSHOT &shots, int slot);
void StealShotNotice(bool naming, int item);

} // namespace expanded
