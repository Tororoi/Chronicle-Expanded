#pragma once

#include <string>
#include <string_view>

namespace dc::game {
struct RS_STACKDATA;
} // namespace dc::game

namespace expanded {

// A staged run (--stage FILE, DC_STAGE), for recording a feature: a save's game started on a dungeon floor with a
// party member out, a weapon in hand and monsters set out before it, or in a town at a fishing sign with a bait
// ready, so that an input script only has to play the feature. Every key is optional. A stage file is "key = value"
// lines; '#' starts a comment.
struct Stage {
    std::string roster;             // roster: the floor's monster kinds, as --roster takes them
    int         slot = 1;           // slot: the --save folder's save loaded (1 = saves/1)
    int         dungeon = -1;       // dungeon: 0-6, started in at once with that save's game
    int         floor = -1;         // floor: the floor its entrance takes, by the floor menu's index
    int         character = -1;     // character: the member out (toan, xiao, goro, ruby, ungaga, osmond or 0-5)
    int         weapon = -1;        // weapon: that member's weapon (item number), at its maximum attack
    int         attack = -1;        // attack: that weapon's attack instead
    int         sphere = -1;        // sphere: Xiao's Super Steve, carrying a synthesis sphere of this weapon
    long        seed = -1;          // seed: Expanded's random numbers
    int         place = 0;          // place: monsters in play set out in front of the member, the others sent away
    float       distance = 60.0f;   // distance: that far in front
    float       spread = 0.35f;     // spread: radians between them
    float       angle = 0.0f;       // angle: radians turned from the member's facing
    int         place_kind = -1;    // place_kind: only monsters of this kind
    int         place_after = 100;  // place_after: monster steps after the member is out
    bool        big_room = false;   // big_room: the member walked to the middle of the nearest big room first
    bool        keep_alive = false; // keep_alive: the party's life kept full
    int         circle = -1;        // circle: a magic circle of this kind (0-9) set out in front of the member
    int         town = -1;          // town: the map (MapNo) started in with that save's game, debug mode off
    int         fishing_spot = -1;  // fishing_spot: the town's fishing sign stood at (0 its first)
    int         bait = -1;          // bait: put on the hook as fishing starts (an item number, given if the bag lacks it)
    float       time = -1.0f;       // time: the game's clock (0-12) the save starts at
    std::string give;               // give: item and weapon numbers given, as --give takes them
};

// Parses a stage file's text. False, with why when given, naming the line, for an unknown key or a bad value.
bool ParseStage(std::string_view text, Stage &stage, std::string *why = nullptr);

// Reads and sets the run's stage (null or empty path: none), seeding Expanded's random numbers and setting its
// roster. False, with why, when it cannot be read or parsed or its roster is refused.
bool SetStage(const char *path, std::string *why = nullptr);
// The run's stage, or null.
const Stage *ActiveStage();

// Hooks. The run about to jump into the stage's dungeon or town: its save loaded and its clock set (false when it
// cannot be loaded). The floor the dungeon's entrance takes: the stage's the first time, else otherwise. The party's
// status taken up as a dungeon visit begins: the stage's weapon put in the member's hand. The top of a dungeon frame:
// the member brought out once the floor is in play. A step the monsters take: the member walked to a big room, the
// monsters and the magic circle set out, the party kept alive. A town step of the player: the bait given, the player
// stood at the fishing sign, the bait change asked for once fishing starts. The bait picker opening: true, with the
// stage's bait taken from the bag and chosen, when the stage asked for it.
bool StageLoadSave();
int  StageEntranceFloor(int otherwise);
void StageStatusReady();
void StageFrame();
void StageStep();
void StageTownStep();
bool StagePickBait(dc::game::RS_STACKDATA *choice);

// The stage's progress cleared (a new run, for tests too).
void StageReset();

} // namespace expanded
