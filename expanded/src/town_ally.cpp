#include "expanded/town_ally.hpp"

#include <cstring>
#include <iterator>

#include "battlemenu.hpp"
#include "character.hpp"
#include "dataread.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "editpartsinfo.hpp"
#include "main.hpp"
#include "menu_draw.hpp"

namespace expanded {

namespace {

constexpr int kAllyCount = static_cast<int>(std::size(kTownAllyBodies));

// The town's plain entry event, which an interior starts with when its door names none.
constexpr int kInteriorEntryEvent = 128;

// Party member the player walks the town as.
int current_ally;

// Party member committed in the menu, swapped in when it closes; -1 for none.
int pending_ally = -1;

// Party member whose town body is loaded, or -1 for another body.
int loaded_body;

// Whether Toan stands in for the party member through an event that poses him.
bool stand_in;

// Holds a body's pack while it loads, as events keep their own packs in read_buffer.
alignas(64) u_char pack_buffer[0x200000];

const TownAllyBody &Body(int ally) {
    return kTownAllyBodies[ally];
}

bool IsTownBody(const char *pack, const char *info, int ally) {
    return std::strcmp(pack, Body(ally).pack) == 0 && std::strcmp(info, Body(ally).info) == 0;
}

// Loads a party member's body where the current one stands, facing the same way.
void SwapBody(int ally) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;

    Chara->GetPosition(position);
    Chara->GetRotation(rotation);
    u_int *events_buffer = read_buffer;
    read_buffer = reinterpret_cast<u_int *>(pack_buffer);
    EdLoadMainChara(const_cast<char *>(Body(ally).pack), const_cast<char *>(Body(ally).info), nullptr);
    read_buffer = events_buffer;
    Chara->SetPosition(position);
    Chara->SetRotation(rotation);
    Chara->SetMotion(0, 0);
    Chara->Step();
    Chara->ClothStep(-1);
    Chara->ShadowStep();
    loaded_body = ally;
}

} // namespace

int TownAllyCurrent() {
    return current_ally;
}

int TownAllyLoaded() {
    return loaded_body;
}

void TownAllyReset() {
    stand_in = false;
    current_ally = 0;
    pending_ally = -1;
    loaded_body = 0;
}

bool TownAllyCommit(int chara, int party_size) {
    if (chara < 0 || chara >= kAllyCount || chara > party_size - 1 || MapNo == TOWN_YELLOW_DROPS) {
        return false;
    }

    pending_ally = chara == current_ally ? -1 : chara;
    return true;
}

void TownAllyMenuCommit(int chara, int party_size) {
    if (!TownAllyCommit(chara, party_size)) {
        ComMenuSePlay(MENU_SOUND_REFUSE);
        return;
    }

    ComMenuSePlay(MENU_SOUND_CONFIRM);

    if (pending_ally >= 0) {
        MenuChara.state = MENU_CHARA_CHANGE_LOAD;
        MenuChara.change_counter = 0;
    }
}

void TownAllyWalk() {
    if (stand_in && pending_ally < 0) {
        pending_ally = current_ally;
    }

    if (pending_ally < 0) {
        return;
    }

    current_ally = pending_ally;
    pending_ally = -1;
    stand_in = false;
    SwapBody(current_ally);
}

void TownAllyEventBody() {
    if (loaded_body > 0) {
        SwapBody(0);
        stand_in = true;
    }
}

void TownAllyMainCharaPack(char **pack, char **info, CDataAlloc2<1> *arena) {
    if (arena != nullptr || !IsTownBody(*pack, *info, 0)) {
        loaded_body = -1;
        return;
    }

    *pack = const_cast<char *>(Body(current_ally).pack);
    *info = const_cast<char *>(Body(current_ally).info);
    loaded_body = current_ally;
    stand_in = false;
}

bool TownAllyBlocksLadder() {
    return loaded_body > 0;
}

bool TownAllySkipsCutscene(int event_no) {
    return current_ally != 0 && event_no > 0;
}

int TownAllyInteriorStartEvent(int event_no, int house) {
    if (current_ally == 0) {
        return event_no;
    }

    EDITPARTS_INFO *info = house >= 0 ? EditPartsInfo.GetPartsInfo(house) : nullptr;

    if (info != nullptr && !(info->completion_flags & 1) && EditPartsInfo.CheckComplete(house) != 0) {
        return 0;
    }

    return kInteriorEntryEvent;
}

int TownAllyMotionIndex(const CCharacter *chara, const tagMOTION_TYPE *motion, int index) {
    if (chara != &MainChara || loaded_body <= 0 || motion->motion_info == nullptr) {
        return index;
    }

    for (int i = 0; i <= index; i++) {
        if (motion->motion_info[i].start < 0 && motion->motion_info[i].end < 0) {
            return 0;
        }
    }

    return index;
}

float TownAllyMarkLift() {
    return loaded_body > 0 ? Body(loaded_body).mark_lift : 0.0f;
}

float TownAllyEyeHeight() {
    return loaded_body > 0 ? Body(loaded_body).eye_height : Body(0).eye_height;
}

} // namespace expanded
