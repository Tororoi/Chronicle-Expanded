#include "expanded/town/ally.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>

#include "actionseq.hpp"
#include "battlemenu.hpp"
#include "character.hpp"
#include "dataread.hpp"
#include "ebattle.hpp"
#include "editground.hpp"
#include "editloop.hpp"
#include "editpartsinfo.hpp"
#include "expanded/town/ladder.hpp"
#include "main.hpp"
#include "menu_draw.hpp"
#include "platform/paths.hpp"
#include "snd.hpp"

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

// Whether the ally boards the submarine in its own body, playing its own jump.
bool boarding;

// The ally riding the submarine to its other end, or 0.
int riding;

// The climb out of the hatch: asked for and waiting for the sequence to stand at the hatch, and its frames as they play.
bool       climb_asked;
TownLadder climb;
size_t     climb_frame;

// Frames the town body has stood still while walking the town.
int standing_frames;

// Whether the data has each party member's built model.
bool has_model[kAllyCount];

// Whether the missing built models were logged.
bool logged_missing;

// Holds a body's pack while it loads, as events keep their own packs in read_buffer.
alignas(64) u_char pack_buffer[0x200000];

const TownAllyBody &Body(int ally) {
    return kTownAllyBodies[ally];
}

bool IsTownBody(const char *pack, const char *info, int ally) {
    return std::strcmp(pack, Body(ally).pack) == 0 && std::strcmp(info, Body(ally).info) == 0;
}

// Looks for the built models in the data, logging once those it lacks.
void FindModels() {
    std::string missing;

    for (int ally = 0; ally < kAllyCount; ally++) {
        std::error_code error;
        const char     *model = Body(ally).model;
        has_model[ally] = model != nullptr && std::filesystem::is_regular_file(PathsDataRoot() / model, error);
        if (model != nullptr && !has_model[ally]) {
            missing += missing.empty() ? model : std::string(", ") + model;
        }
    }

    if (!missing.empty() && !logged_missing) {
        logged_missing = true;
        std::fprintf(stderr, "town: the data lacks %s; those allies walk in their disc bodies (dcdata expanded <data> builds them)\n",
                     missing.c_str());
    }
}

// Loads a party member's body where the current one stands, facing the same way.
void SwapBody(int ally) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;

    TownAllyLadderCancel();
    Chara->GetPosition(position);
    Chara->GetRotation(rotation);
    u_int *events_buffer = read_buffer;
    read_buffer = reinterpret_cast<u_int *>(pack_buffer);
    auto [pack, info] = TownAllyPack(ally);
    EdLoadMainChara(const_cast<char *>(pack), const_cast<char *>(info), nullptr);
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

std::pair<const char *, const char *> TownAllyPack(int ally) {
    if (has_model[ally]) {
        return {Body(ally).model, Body(ally).model_info};
    }

    return {Body(ally).pack, Body(ally).info};
}

int TownAllyCurrent() {
    return current_ally;
}

int TownAllyLoaded() {
    return loaded_body;
}

void TownAllyReset() {
    int ally = std::ranges::contains(kSubmarineMaps, MapNo) ? riding : 0;
    stand_in = false;
    boarding = false;
    if (MapNo != kSubmarineRide) {
        riding = 0;
    }
    current_ally = 0;
    pending_ally = -1;
    loaded_body = 0;
    TownAllyLadderCancel();
    climb_asked = false;
    climb = {};
    climb_frame = 0;
    FindModels();
    // Off the submarine in its own body, ready for the arrival's climb out.
    if (ally > 0 && has_model[ally]) {
        current_ally = ally;
        riding = ally;
        SwapBody(ally);
    }
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
    boarding = false;
    if (MapNo != kSubmarineRide) {
        riding = 0;
    }
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

bool TownAllyKeepsBody(const char *cfg) {
    boarding = loaded_body > 0 && has_model[loaded_body] &&
               std::ranges::any_of(kBoardingCfgs, [cfg](const char *own) { return std::strcmp(cfg, own) == 0; });
    if (boarding) {
        riding = loaded_body;
    }
    return boarding;
}

bool TownAllyEventMotion(int &motion, float &speed) {
    if (boarding && motion == kClimbOutMotion) {
        climb_asked = true;
        return true;
    }
    int key = motion - kBoardingFirstMotion;
    if (!boarding || key < 0 || key >= 3) {
        return false;
    }
    const BoardingMotion &own = kBoardingMotions[loaded_body][key];
    motion = own.motion;
    if (own.speed > 0.0f) {
        speed = own.speed;
    }
    return false;
}

void TownAllyClimbStep(CActionSeq &sequence) {
    CCharacter *chara = sequence.character;
    if (chara == nullptr || chara != EdEventInfo.main_character) {
        return;
    }
    if (climb_asked) {
        // The sequence has taken its place at the top of the ramp: the climb ends there.
        climb_asked = false;
        climb = TownHatchClimb(loaded_body, sequence.position, sequence.rotation[1]);
        climb_frame = 0;
    }
    if (climb_frame >= climb.frames.size()) {
        return;
    }
    const TownLadderFrame &frame = climb.frames[climb_frame++];
    sceVu0FVECTOR          position = {frame.x, frame.y, frame.z, 1.0f};
    sceVu0FVECTOR          rotation = {0.0f, frame.yaw, 0.0f, 0.0f};
    chara->SetPosition(position);
    chara->SetRotation(rotation);
    if (frame.motion >= 0) {
        chara->SetMotion(frame.motion, frame.flags);
        if (frame.speed > 0.0f) {
            chara->SetMotionSpeed(frame.speed);
        }
    }
    if (frame.sound >= 0) {
        SndSePlay(frame.sound, -1, 0);
    }
}

bool TownAllyClimbing(const CActionSeq &sequence) {
    return sequence.character != nullptr && sequence.character == EdEventInfo.main_character &&
           (climb_asked || climb_frame < climb.frames.size());
}

void TownAllyMainCharaPack(char **pack, char **info, CDataAlloc2<1> *arena) {
    if (arena != nullptr || !IsTownBody(*pack, *info, 0)) {
        loaded_body = -1;
        return;
    }

    auto [ally_pack, ally_info] = TownAllyPack(current_ally);
    *pack = const_cast<char *>(ally_pack);
    *info = const_cast<char *>(ally_info);
    loaded_body = current_ally;
    stand_in = false;
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

int TownAllyWalkMotion(int motion) {
    constexpr int kXiao = 1;
    if (motion != 0 || loaded_body != kXiao || !has_model[kXiao] || EdMoveCharaInfo.fishing != 0) {
        standing_frames = 0;
        return motion;
    }
    standing_frames = std::min(standing_frames + 1, kTownAllySitFrames);
    return standing_frames >= kTownAllySitFrames ? kTownAllySitMotion : motion;
}

float TownAllyMarkLift() {
    return loaded_body > 0 ? Body(loaded_body).mark_lift : 0.0f;
}

float TownAllyEyeHeight() {
    return loaded_body > 0 ? Body(loaded_body).eye_height : Body(0).eye_height;
}

} // namespace expanded
