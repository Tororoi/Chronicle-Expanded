#pragma once

namespace dc::game {
struct RS_STACKDATA;
} // namespace dc::game

namespace expanded {

// A dungeon door a weapon opens without its key: the locked room's gate, whose script asks for key alone
// (_ITEM_USE_WINDOW). While active() holds, the window is answered with the key, none shown, and the system message
// message is shown for kDoorKeyFrames.
struct DoorKey {
    int key;
    bool (*active)();
    int message;
};

inline constexpr int kDoorKeyFrames = 240;

// Registers, once, a door a weapon opens.
void AddDoorKey(const DoorKey &door);

// Hook: _ITEM_USE_WINDOW, its list of the items it takes (-1 at the end): a window that takes a registered key alone,
// its weapon active, is answered with the key into result; true then.
bool DoorKeyAnswers(const int *items, RS_STACKDATA *result);

} // namespace expanded
