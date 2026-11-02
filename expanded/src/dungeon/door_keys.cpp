#include "expanded/dungeon/door_keys.hpp"

#include <vector>

#include "clsmes.hpp"
#include "dun/gameloop.hpp"
#include "runscript.hpp"
#include "sysmes.hpp"

namespace expanded {

namespace {

std::vector<DoorKey> &Doors() {
    static std::vector<DoorKey> doors;
    return doors;
}

} // namespace

void AddDoorKey(const DoorKey &door) {
    for (const DoorKey &added : Doors()) {
        if (added.key == door.key) {
            return;
        }
    }
    Doors().push_back(door);
}

bool DoorKeyAnswers(const int *items, RS_STACKDATA *result) {
    if (result == nullptr || items[0] == -1 || items[1] != -1) {
        return false;
    }

    for (const DoorKey &door : Doors()) {
        if (door.key != items[0] || !door.active()) {
            continue;
        }
        result->i = door.key;
        BtEventInfo.item_select_result = 0;
        int args[4] = {-1, -1, -1, -1};
        SetSystemMes(door.message, kDoorKeyFrames, MES_POS_BOTTOM, 0, args, nullptr);
        return true;
    }
    return false;
}

} // namespace expanded
