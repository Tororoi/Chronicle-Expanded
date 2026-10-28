#pragma once

namespace expanded {

// Blessing Gun "Blessed Bait": while a Blessing Gun is owned (any bag or the storage), bait is only spent on a fight.
// The two rolls that otherwise lose the cast's bait, 1 in 5 as the float sinks and 3 in 10 as a hooked fish gets away
// (EdMoveChara), leave it on the hook.

// Hook: EdMoveChara's roll lost the cast's bait: whether it stays on the hook.
bool BlessedBaitKept();

} // namespace expanded
