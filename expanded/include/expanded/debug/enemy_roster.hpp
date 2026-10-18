#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "expanded/enemies/roster.hpp"

namespace expanded {

// Parses a roster: MonstorTable indexes split by commas, a trailing '!' spawning that kind at most once a
// floor ("20!,60"); bosses and their parts spawn once anyway. False, with why when given, for a bad index, a
// kind twice or more than nine kinds; an empty spec is an empty roster.
bool ParseRosterSpec(std::string_view spec, Roster &roster, std::string *why = nullptr);

// Why a floor could not be laid out from a roster, for any party (the party is known only in a dungeon); empty
// when it can. A kind dcdata has not built, a boss cast without room for its partner, and a roster of bosses alone
// that come only with a party member (the Ice Queen with Ruby, King's Curse with Ungaga), which would leave the floor
// empty without them.
std::string RosterProblem(const Roster &roster);

// Debug mode's enemy roster: every floor of every dungeon visit begun after it is set. An empty spec
// clears it. False, keeping the last, with why when given, for a bad spec or one with a RosterProblem.
bool               SetDebugRoster(std::string_view spec, std::string *why = nullptr);
const std::string &DebugRosterSpec();

// The developer menu's roster row. Hooks: the rows it adds, its text (into room bytes, cut to fit; the row selected or
// not; typing shows the entry) and its step (true while it takes the pad: Cross opens the entry, Enter sets it,
// Escape leaves it).
int  DevMenuRosterRows();
int  DevMenuRosterDraw(char *text, std::size_t room, bool selected);
bool DevMenuRosterStep(bool selected);

} // namespace expanded
