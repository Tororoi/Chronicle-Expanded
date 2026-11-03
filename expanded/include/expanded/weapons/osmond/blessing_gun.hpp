#pragma once

namespace dc::game {
struct ATTACH_LIST;
struct WEAPON_HAVE;
} // namespace dc::game

namespace expanded {

// Blessing Gun "Blessed Bait": while a Blessing Gun is owned (any bag or the storage), bait is only spent on a fight.
// The two rolls that otherwise lose the cast's bait, 1 in 5 as the float sinks and 3 in 10 as a hooked fish gets away
// (EdMoveChara), leave it on the hook.

// Hook: EdMoveChara's roll lost the cast's bait: whether it stays on the hook.
bool BlessedBaitKept();

// Blessing Gun "Blessing Boost": every element stat an attachment raises (Fire to Holy, the gems, a SynthSphere) is
// worth one more on a Blessing Gun, and on Super Steve while he carries its SynthSphere, as Buster Boost does for anti
// stats (buster_sword.hpp): the entry holds the value, which the menus, damage, build-up checks and the level-up's
// absorb all read; off those weapons it is the attachment's own again. A SynthSphere's values are its own, so its entry
// carries kBlessingBoostedSphere while it holds the point.
inline constexpr int kBlessingBoost = 1;
inline constexpr int kBlessingBoostedSphere = 0x02; // in ATTACH_LIST::unk_1F, beside kBusterBoostedSphere

// Whether the weapon's entries take the boost: the Blessing Gun, or Super Steve carrying its sphere.
bool BlessingBoosted(const WEAPON_HAVE *weapon);

// An attachment entry with the boost on or off.
void BlessingBoostEntry(ATTACH_LIST &entry, bool boosted);

// The weapon's attachments boosted as it now takes them.
void BlessingBoostWeapon(WEAPON_HAVE &weapon);

} // namespace expanded
