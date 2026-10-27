#pragma once

namespace dc::game {
struct WEAPON_DATA;
} // namespace dc::game

namespace expanded {

// Whether the weapon builds up into anything.
bool BuildsUp(int weapon_no, const WEAPON_DATA *data);

// Whether the weapon builds up into the weapon at this position of its owner's chain.
bool BuildsUpInto(int weapon_no, const WEAPON_DATA *data, int position);

} // namespace expanded
