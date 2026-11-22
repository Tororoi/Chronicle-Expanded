#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

#include "quest/framework.hpp"

namespace expanded {

// The villagers who give Expanded's quests (docs/QUEST_FRAMEWORK.md), by town (MapNo) and number, as the dialog
// framework numbers a town's villagers: their PEOPLE2 line in the town's mapinfo.cfg (the mod named them by model,
// p01a.chr Macho...).
struct Giver {
    int             map_no;
    int             npc_id;
    dc::quest::Text name;

    dc::quest::NpcMatch Match() const { return {.map_no = map_no, .npc_id = npc_id, .mes_file_key = {}}; }
};

inline constexpr int kNorune = 0, kMatataki = 1, kQueens = 2, kMuskaLacka = 3, kYellowDrops = 23;

// A giver's choice on their talk menu, added to their conversation while they have a quest to give (kOfferLabel) or
// one is under way (kAboutLabel): the quest's offered and ongoing stages. A villager with neither has no choice, and
// with nothing else added keeps retail's conversation.
inline constexpr dc::quest::Text kOfferLabel = {.key = "expanded.quest.offer", .english = "Got anything for me?"};
inline constexpr dc::quest::Text kAboutLabel = {.key = "expanded.quest.about", .english = "About the quest."};

dc::quest::Builder &Offers(dc::quest::Builder &quest, const Giver &giver, std::initializer_list<std::string_view> offered,
                           dc::quest::SceneFn offer, std::initializer_list<std::string_view> ongoing,
                           dc::quest::SceneFn about);

// A text in the player's language, as a quest shows it.
std::string QuestText(dc::quest::Text text);

// Logs a quest moved on in play (the event log's "quest": its id, stage and what it has left).
void LogQuest(const dc::quest::Context &ctx);

// Shows text at the bottom of the screen, as the dungeon tells of an item found: a quest's progress told in play.
void QuestNotice(std::string text);

// The text QuestNotice last showed, for tests.
const std::string &QuestNoticeShown();

} // namespace expanded
