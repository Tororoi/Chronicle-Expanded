#pragma once

#include <utility>

#include "dataalloc_fwd.hpp"

namespace expanded {

// The body a party member walks the town in, and its draw tweaks.
struct TownAllyBody {
    const char *model;      // built town model (expanded/docs/town-models.md), loaded when the data has it
    const char *model_info; // its .cfg
    const char *pack;       // .chr loaded as the main character without the built model
    const char *info;       // its .cfg
    float       mark_lift;  // added to the height of the "!" over the player
    float       eye_height; // first-person eye above the feet
};

// The town bodies, by party slot (Toan, Xiao, Goro, Ruby, Ungaga, Osmond).
inline constexpr TownAllyBody kTownAllyBodies[6] = {
    {nullptr,                    nullptr,          "chara/c01d.chr",              "info.cfg", 0.0f, 14.0f},
    {"expanded/town/xiao.chr",   "info.cfg",       "gedit/e01/chara/c04pcat.chr", "info.cfg", 4.0f, 5.0f },
    {"expanded/town/goro.chr",   "info.cfg",       "gedit/s01/chara/c06p.chr",    "info.cfg", 0.0f, 14.0f},
    {"expanded/town/ruby.chr",   "info.cfg",       "gedit/e03/chara/c05a.chr",    "info.cfg", 2.5f, 14.0f},
    {"expanded/town/ungaga.chr", "e323_2c10a.cfg", "gedit/s32/chara/c10p.chr",    "info.cfg", 6.0f, 14.0f},
    {"expanded/town/osmond.chr", "info.cfg",       "gedit/e05/chara/c18p.chr",    "info.cfg", 0.0f, 14.0f},
};

// The .chr and .cfg party member ally's town body loads: its built model when the data has it.
std::pair<const char *, const char *> TownAllyPack(int ally);

// Party member the player walks the town as.
int TownAllyCurrent();

// Party member whose town body the main character has, or -1 for another body (fishing, events).
int TownAllyLoaded();

// Starts a town at Toan, as the town loads him, and looks for the built models in the data.
void TownAllyReset();

// Party-menu commit in a town: queues the swap to party member chara; false refuses it.
bool TownAllyCommit(int chara, int party_size);

// Party-menu commit in a town: the dungeon's confirm, spread and close for a swap; the reject beep for a refusal.
void TownAllyMenuCommit(int chara, int party_size);

// Each walking frame: swaps in the committed party member, or back from Toan after an event.
void TownAllyWalk();

// Before an event poses the player with Toan's motions: Toan stands in for the party member.
void TownAllyEventBody();

// East Harbor's submarine boarding loads Toan's jump (kBoardingCfg) for the player: an ally in a built body keeps it, and
// the event's KEYs 200-202 (takeoff, air, landing) play the ally's own (kBoardingMotions: slot and speed, or -1 for the
// event's), its own ladder-down leap or Toan's jump retargeted onto it.
inline constexpr const char *kBoardingCfg = "e202c01d.cfg";
inline constexpr int         kBoardingFirstMotion = 200;

struct BoardingMotion {
    int   motion;
    float speed;
};

inline constexpr BoardingMotion kBoardingMotions[6][3] = {
    {},
    {{11, -1.0f}, {8, -1.0f}, {9, -1.0f}}, // Xiao: the crouch, the leap, the landing
    {{13, -1.0f}, {8, -1.0f}, {9, -1.0f}}, // Goro: down into the crouch, the fall, the landing
    {{13, -1.0f}, {14, -1.0f}, {15, -1.0f}}, // Ruby: Toan's jump
    {{11, -1.0f}, {12, -1.0f}, {13, -1.0f}}, // Ungaga: Toan's jump
    {{11, -1.0f}, {8, -1.0f}, {9, 0.30f}}, // Osmond: the dive, the fall, the landing
};

// _LOAD_CHARA of cfg for the player: true when the ally keeps its body (the boarding), which skips the load.
bool TownAllyKeepsBody(const char *cfg);

// An event's motion for the player through the boarding: the ally's own, its speed set where the table gives one.
void TownAllyEventMotion(int &motion, float &speed);

// Points a script's load of Toan's town body at the current party member's.
void TownAllyMainCharaPack(char **pack, char **info, CDataAlloc2<1> *arena);

// Whether a story cutscene the town fades into is skipped, as it is while an ally walks the town.
bool TownAllySkipsCutscene(int event_no);

// The event an interior starts with: an ally gets the plain entry, and none at a house ready to complete.
int TownAllyInteriorStartEvent(int event_no, int house);

// Hook: EdMoveChara as it sets the walking motion: the motion played, motion (0 standing, else moving) as a rule, but
// Xiao's cat, in its built town body, sits (kTownAllySitMotion, its looping sit) once it has stood kTownAllySitFrames
// (a second and a half) while the player walks the town, until it moves.
inline constexpr int kTownAllySitMotion = 6, kTownAllySitFrames = 90;
int                  TownAllyWalkMotion(int motion);

// Height added to the "!" over the player.
float TownAllyMarkLift();

// First-person eye height above the player's feet.
float TownAllyEyeHeight();

} // namespace expanded
