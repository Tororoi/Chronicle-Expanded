#include "expanded/quests/item_hunts.hpp"

#include <array>
#include <format>
#include <string>

#include "dungeonmap.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "itemdata.hpp"

namespace expanded {

using dc::quest::Context;
using dc::quest::Scene;
using dc::quest::Text;

namespace {

constexpr std::array kHunts = {
    ItemHunt{.quest = "hunt_laura",
             .giver = {.map_no = kNorune, .npc_id = 10, .name = {.key = "expanded.quest.giver.laura", .english = "Laura"}},
             .dungeon = 0,
             .item = ITEM_MEDUSA_POWDER   },
    ItemHunt{.quest = "hunt_ro",
             .giver = {.map_no = kMatataki, .npc_id = 10, .name = {.key = "expanded.quest.giver.ro", .english = "Ro"}},
             .dungeon = 1,
             .item = ITEM_WARP_POWDER     },
    ItemHunt{.quest = "hunt_phil",
             .giver = {.map_no = kQueens, .npc_id = 12, .name = {.key = "expanded.quest.giver.phil", .english = "Phil"}},
             .dungeon = 2,
             .item = ITEM_SHELL_RING      },
    ItemHunt{.quest = "hunt_zabo",
             .giver = {.map_no = kMuskaLacka, .npc_id = 2, .name = {.key = "expanded.quest.giver.zabo", .english = "Zabo"}},
             .dungeon = 3,
             .item = ITEM_HARDENING_POWDER},
};

// Each giver's rumour, word while it is on, and word once the item is found, in kHunts' order.
constexpr Text kRumours[] = {
    {.key = "expanded.quest.hunt.rumour.laura",
     .english = "I heard whispers that if you venture to the back floors of the Divine Beast Cave, you might come "
                "across a rare item.{page}Apparently it's some kind of powder."                             },
    {.key = "expanded.quest.hunt.rumour.ro",
     .english = "One of the Wise Owl Forest's owls told me that somewhere deep on the forest's back floors, you can find "
                "a mysterious item.{page}It's said to be an ancient item whose purpose was never fulfilled."},
    {.key = "expanded.quest.hunt.rumour.phil",
     .english = "There are some more details I didn't tell you about the Queen.{page}"
                "It's rumoured that when she sank to the bottom with the Shipwreck, her wedding ring was lost.{page}"
                "The ring might still be somewhere deep in the Shipwreck, on its back floors. Do you think you can find "
                "it?"                                                                                       },
    {.key = "expanded.quest.hunt.rumour.zabo",
     .english = "A long time ago there was a rare and mysterious yellow powder, and no, I'm not talking about Repair "
                "Powder.{page}No one knows the truth of it, but the ancients might have left one on the dark side of "
                "the Sun & Moon Temple: its back floors."                                                   },
};

constexpr Text kStill[] = {
    {.key = "expanded.quest.hunt.still.laura",
     .english = "Did you find that rare item yet? Apparently it can be found somewhere on the back floors of the Divine "
                "Beast Cave."},
    {.key = "expanded.quest.hunt.still.ro",
     .english = "Have you found the ancient item yet? It's said to be somewhere on the back floors of the Wise Owl "
                "Forest."    },
    {.key = "expanded.quest.hunt.still.phil",
     .english = "The Queen's wedding ring could be hiding somewhere on the back floors of the Shipwreck. Good luck "
                "searching!" },
    {.key = "expanded.quest.hunt.still.zabo",
     .english = "Did you find the powder yet? There might still be one somewhere on the dark side of the Sun & Moon "
                "Temple."    },
};

constexpr Text kFound[] = {
    {.key = "expanded.quest.hunt.found.laura",
     .english = "Oh, you found it? Great! It's a Medusa Powder?{page}You're asking me how to use it? Sorry, I don't know "
                "what it is."                                                           },
    {.key = "expanded.quest.hunt.found.ro",
     .english = "You've found the ancient item? You say it's a Warp Powder?{page}Hmm, something like that sounds too "
                "powerful. Maybe that's why it's never been able to show its potential."},
    {.key = "expanded.quest.hunt.found.phil",
     .english = "You found the Queen's wedding ring? So the rumours were true! Interesting.{page}You can keep the ring. "
                "Who knows where you might need it on your adventure."                  },
    {.key = "expanded.quest.hunt.found.zabo",
     .english = "I see you've found the mysterious powder. I believe the ancients called it Hardening Powder.{page}"
                "I wish we knew more about it."                                         },
};

const ItemHunt &HuntOf(const std::string &quest) {
    for (const ItemHunt &hunt : kHunts) {
        if (hunt.quest == quest) {
            return hunt;
        }
    }
    return kHunts[0];
}

int IndexOf(const ItemHunt &hunt) {
    return static_cast<int>(&hunt - kHunts.data());
}

void Rumour(Scene &scene) {
    scene.Say(kRumours[IndexOf(HuntOf(scene.QuestId()))]);
    scene.SetStage("heard");
}

void About(Scene &scene) {
    const int index = IndexOf(HuntOf(scene.QuestId()));
    if (scene.AtStage("found")) {
        scene.Say(kFound[index]);
        scene.SetStage("done");
        return;
    }
    scene.Say(kStill[index]);
}

dc::quest::Definition Quest(const ItemHunt &hunt) {
    const int          item = hunt.item;
    dc::quest::Builder quest(hunt.quest);
    quest.Stages({"unheard", "heard", "found", "done"});
    quest.Advance("heard", [item](const Context &ctx) { return ctx.Game().HasItemAnywhere(item); }, "found");
    return Offers(quest, hunt.giver, {"unheard"}, Rumour, {"heard", "found"}, About).Build();
}

} // namespace

std::span<const ItemHunt> ItemHunts() {
    return kHunts;
}

std::vector<Text> ItemHuntTexts() {
    std::vector<Text> texts;
    for (const ItemHunt &hunt : kHunts) {
        texts.push_back(hunt.giver.name);
    }
    texts.insert(texts.end(), std::begin(kRumours), std::end(kRumours));
    texts.insert(texts.end(), std::begin(kStill), std::end(kStill));
    texts.insert(texts.end(), std::begin(kFound), std::end(kFound));
    return texts;
}

void RegisterItemHunts() {
    for (const ItemHunt &hunt : kHunts) {
        dc::quest::Register(Quest(hunt));
    }
}

bool ItemHuntOn(const ItemHunt &hunt) {
    const dc::quest::Progress *progress = dc::quest::FindProgress(hunt.quest);
    const dc::quest::IGame    *game = dc::quest::Game();
    return progress != nullptr && progress->stage == "heard" && (game == nullptr || !game->HasItemAnywhere(hunt.item));
}

void ItemHuntChests(CDungeonMap &map, int dungeon, bool back_floor) {
    if (!back_floor) {
        return;
    }
    for (const ItemHunt &hunt : kHunts) {
        if (hunt.dungeon != dungeon || !ItemHuntOn(hunt) || !ModChance(kHuntChance)) {
            continue;
        }
        for (TREASURE_BOX &box : map.boxes) {
            if (box.used != 0 && box.kind == TREASURE_BOX_SMALL) {
                box.item_no = hunt.item;
                if (EventLogOn()) {
                    LogEvent("quest_item", std::format("\"quest\":\"{}\",\"item\":{}", hunt.quest, hunt.item));
                }
                return;
            }
        }
    }
}

} // namespace expanded
