#pragma once

#include <string>
#include <string_view>

namespace dc::game {
class CMonstorUnit;
struct COLLISION_HIT;
} // namespace dc::game

namespace expanded {

struct OwnSound;

// A run's event log (--event-log FILE, DC_EVENT_LOG), for a recording's captions: one JSON object a line, the main
// loop's frame (as --record numbers its images) and the event first, e.g.
//   {"frame":3612,"event":"damage","monster":0,"kind":44,"amount":136,"life":14,"owner":1,"attack":0}
// Events: floor (dungeon, floor), character (chara) as the member out changes, damage (monster, kind, amount, life
// left, and the hit's owner and attack: the member and which of their attacks, as CCollisionData::SetUserID names
// them) from a weapon's hit, kill (monster, kind, by: the member its kill is, whose weapon takes its ABS when they are
// out), life (chara, from, to) and whp (chara, weapon, from, to) as the
// member out's life and weapon HP change, sound (program, note) as an own sound plays, and a stage's progress (stage:
// out, walked, placed, circle), the frames an input script can be timed from. Features can add their own.

// Opens the log, truncating it (null or empty path: none). False, with why, when it cannot be opened.
bool SetEventLog(const char *path, std::string *why = nullptr);
bool EventLogOn();

// Writes an event; fields is the rest of the object's members, already JSON ("monster":0,"amount":136), or empty.
void LogEvent(std::string_view event, std::string_view fields = {});

// Hooks: a floor's monsters loaded, a monster damaged by a weapon's hit and killed, an own sound played, and a step
// the monsters take (the member out, its life and its weapon HP read for changes).
void EventLogFloorLoaded();
void EventLogDamaged(const CMonstorUnit &unit, int monster, const COLLISION_HIT &hit, int amount);
void EventLogKilled(const CMonstorUnit &unit, int monster);
void EventLogSound(const OwnSound &sound);
void EventLogStep();

} // namespace expanded
