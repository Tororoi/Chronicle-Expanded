#include "expanded/weapons/toan/wise_owl_sword.hpp"

#include <optional>
#include <string>

#include "clsmes.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "monstorunit.hpp"
#include "sysmes.hpp"

namespace expanded {

namespace {

constexpr int kWiseOwlFrames = 180;

// The line, a system message of its own: the key's name in it.
std::optional<std::string> WiseOwlLine() {
    return LocalizeText("expanded.wise_owl.key", "Wise Owl senses the {insert1} nearby...");
}

[[maybe_unused]] const bool kWiseOwlLine = (SetSystemMessage(kWiseOwlMessage, WiseOwlLine), true);

// The monster last named, -1 once none is in range.
int named = -1;

void OnFloor() {
    named = -1;
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

bool WiseOwlKey(int item_no) {
    return item_no == ITEM_SHINY_STONE || item_no == ITEM_RED_BERRY || item_no == ITEM_POINTY_CHESTNUT;
}

} // namespace

int WiseOwlKeyHolder(const CMonstorUnit &unit) {
    int   holder = -1;
    float nearest = kWiseOwlRange;
    for (int monster = 0; monster < kFloorMonsters; monster++) {
        const MONSTOR &live = unit.monster[monster];
        if (live.state != -1 && live.hp > 0 && WiseOwlKey(live.drop_item) && live.player_distance > 0.0f &&
            live.player_distance < nearest) {
            nearest = live.player_distance;
            holder = monster;
        }
    }
    return holder;
}

void WiseOwlStep(const CMonstorUnit &unit) {
    int holder = WiseOwlKeyHolder(unit);
    if (holder < 0) {
        named = -1;
        return;
    }
    // Another line on screen holds this one back until it is gone.
    if (holder == named || SystemMesCheck() || !WeaponOwned(ITEM_WEAPON_WISE_OWL_SWORD)) {
        return;
    }
    COM_ITEM_INFO *info = GetCommonItemInfo(unit.monster[holder].drop_item);
    int            args[4] = {info != nullptr ? info->msg + 100 : -1, -1, -1, -1};
    SetSystemMes(kWiseOwlMessage, kWiseOwlFrames, MES_POS_BOTTOM, 0, args, nullptr);
    named = holder;
}

} // namespace expanded
