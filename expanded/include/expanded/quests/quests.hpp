#pragma once

#include <vector>

#include "quest/framework.hpp"

namespace expanded {

// Expanded's quests (docs/QUEST_FRAMEWORK.md), registered after the port's own (RegisterAllQuests): their choices come
// after the port's on a villager's menu, and their progress is kept in the save's quests.json beside the port's.
void RegisterQuests();

// Every text Expanded's quests show, for the language files (expanded/lang).
std::vector<dc::quest::Text> QuestTexts();

} // namespace expanded
