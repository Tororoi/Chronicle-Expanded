#pragma once

#include <string>
#include <string_view>

#include "expanded/enemies/roster.hpp"

namespace expanded {

// Parses a roster: MonstorTable indexes split by commas, a trailing '!' spawning that kind at most once a
// floor ("20!,60"); bosses and their parts spawn once anyway. False for a bad index, a kind twice or more
// than nine kinds; an empty spec is an empty roster.
bool ParseRosterSpec(std::string_view spec, Roster &roster);

// Debug mode's enemy roster: every floor of every dungeon visit begun after it is set. An empty spec
// clears it. False, keeping the last, for a bad spec.
bool               SetDebugRoster(std::string_view spec);
const std::string &DebugRosterSpec();

// The developer menu's roster row. Hooks: the rows it adds, its text (the row selected or not; typing
// shows the entry) and its step (true while it takes the pad: Cross opens the entry, Enter sets it,
// Escape leaves it).
int  DevMenuRosterRows();
int  DevMenuRosterDraw(char *text, bool selected);
bool DevMenuRosterStep(bool selected);

} // namespace expanded
