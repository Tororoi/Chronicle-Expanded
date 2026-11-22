#pragma once

#include <string>
#include <vector>

#include "quest/framework.hpp"

namespace expanded {

// The quest tracker: the journal's Quests page (after its Miracle Chests), listing the quests under way, each with
// who gave it and where they are, what it wants and how far it has come, or that it is done and its giver waits.

// A quest under way, as the page shows it.
struct TrackedQuest {
    std::string giver;
    std::string town;
    std::string task;
    int         done = 0;
    int         total = 1;
    bool        complete = false; // done, its giver to be told
    int         item = -1;        // the item drawn beside it (its reward, or the item sought), -1 for none
};

std::vector<TrackedQuest> TrackedQuests();

// The page's lines for its giver: "Done! Go back to Macho." once complete.
std::string TrackedQuestStatus(const TrackedQuest &quest);

// The page, from the journal (menu_manual.cpp): opened from its category, the pad while it is open (true when the
// player backs out to the categories), the pointer at a point of the screen, and its draw.
void QuestTrackerOpen();
bool QuestTrackerKey();
void QuestTrackerPointer(float x, float y);
void QuestTrackerDraw();

// The journal's category for the page, as its menu lists it.
std::string QuestTrackerCategory();

// Every text the page shows.
std::vector<dc::quest::Text> QuestTrackerTexts();

} // namespace expanded
