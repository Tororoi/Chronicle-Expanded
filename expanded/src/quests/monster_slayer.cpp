#include "expanded/quests/monster_slayer.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "expanded/core/random.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"

namespace expanded {

using dc::quest::Context;
using dc::quest::Scene;
using dc::quest::Text;

namespace {

constexpr int kRewardItem = ITEM_POWERUP_POWDER;

constexpr std::array kGivers = {
    SlayerGiver{.quest = "slayer_macho",
                .giver = {.map_no = kNorune, .npc_id = 3, .name = {.key = "expanded.quest.giver.macho", .english = "Macho"}}          },
    SlayerGiver{.quest = "slayer_gob",
                .giver = {.map_no = kMatataki, .npc_id = 5, .name = {.key = "expanded.quest.giver.gob", .english = "Gob"}}            },
    SlayerGiver{.quest = "slayer_jake",
                .giver = {.map_no = kQueens, .npc_id = 8, .name = {.key = "expanded.quest.giver.jake", .english = "Jake"}}            },
    SlayerGiver{.quest = "slayer_bonka",
                .giver = {.map_no = kMuskaLacka, .npc_id = 0, .name = {.key = "expanded.quest.giver.bonka", .english = "Chief Bonka"}}},
};

// Each giver's first word of the quests, in kGivers' order.
constexpr Text kIntros[] = {
    {.key = "expanded.quest.slayer.intro.macho",
     .english = "Prove your true strength to me!{page}"
                "I can give you Monster Slayer quests: defeat a number of one kind of monster in a dungeon you've "
                "been to."                                                   },
    {.key = "expanded.quest.slayer.intro.gob",
     .english = "I might be a cook, but I'm also a hunter, and I respect those who can fight.{page}"
                "That being said, I can give you some Monster Slayer quests."},
    {.key = "expanded.quest.slayer.intro.jake",
     .english = "A message from Mr. King: to win the election, we need to show the town we're strong.{page}"
                "How about you defeat some evil monsters and let us take the credit?{page}"
                "I can give you Monster Slayer quests."                      },
    {.key = "expanded.quest.slayer.intro.bonka",
     .english = "To survive the fearsome monsters on your travels, you can always use some training.{page}"
                "I can give you Monster Slayer quests."                      },
};

constexpr Text kTask = {.key = "expanded.quest.slayer.task",
                        .english = "Your quest is to defeat at least %1 %2 %3.{page}Good luck!"};
constexpr Text kStill = {.key = "expanded.quest.slayer.still",
                         .english = "You're still on the quest to defeat %2 %3. Just %1 left!"};
constexpr Text kReward = {.key = "expanded.quest.slayer.reward",
                          .english = "Well done, you completed it!{page}Here's your reward: a Powerup Powder!"};
constexpr Text kNoRoom = {.key = "expanded.quest.slayer.no_room",
                          .english = "Well done, you completed it!{page}"
                                     "Come back for your reward when you have room in your bag."};
constexpr Text kDone = {.key = "expanded.quest.slayer.done", .english = "%1's quest is complete. Well done!"};

struct Named {
    Monster monster;
    Text    name;
};

constexpr Named kNames[] = {
    {Monster::MASTER_JACKET,   {.key = "expanded.quest.monster.master_jacket", .english = "Master Jackets"}     },
    {Monster::DASHER,          {.key = "expanded.quest.monster.dasher", .english = "Dashers"}                   },
    {Monster::DRAGON,          {.key = "expanded.quest.monster.dragon", .english = "Dragons"}                   },
    {Monster::FLI_FLI,         {.key = "expanded.quest.monster.fli_fli", .english = "Fli Flis"}                 },
    {Monster::EARTH_DIGGER,    {.key = "expanded.quest.monster.earth_digger", .english = "Earth Diggers"}       },
    {Monster::WEREWOLF,        {.key = "expanded.quest.monster.werewolf", .english = "Werewolves"}              },
    {Monster::GUNNY,           {.key = "expanded.quest.monster.gunny", .english = "Gunnies"}                    },
    {Monster::GYON,            {.key = "expanded.quest.monster.gyon", .english = "Gyons"}                       },
    {Monster::PIRATES_CHARIOT, {.key = "expanded.quest.monster.pirates_chariot", .english = "Pirate's Chariots"}},
    {Monster::GOLEM,           {.key = "expanded.quest.monster.golem", .english = "Golems"}                     },
    {Monster::DUNE,            {.key = "expanded.quest.monster.dune", .english = "Dunes"}                       },
    {Monster::BLUE_DRAGON,     {.key = "expanded.quest.monster.blue_dragon", .english = "Blue Dragons"}         },
    {Monster::MOON_DIGGER,     {.key = "expanded.quest.monster.moon_digger", .english = "Moon Diggers"}         },
    {Monster::SPACE_GYON,      {.key = "expanded.quest.monster.space_gyon", .english = "Space Gyons"}           },
    {Monster::CRESCENT_BARON,  {.key = "expanded.quest.monster.crescent_baron", .english = "Crescent Barons"}   },
    {Monster::RASH_DASHER,     {.key = "expanded.quest.monster.rash_dasher", .english = "Rash Dashers"}         },
    {Monster::JOKER,           {.key = "expanded.quest.monster.joker", .english = "Jokers"}                     },
    {Monster::ALEXANDER,       {.key = "expanded.quest.monster.alexander", .english = "Alexanders"}             },
};

constexpr Text kMimics = {.key = "expanded.quest.monster.mimic", .english = "Mimics"};

constexpr Text kDungeons[kSlayerDungeons] = {
    {.key = "expanded.quest.dungeon.0", .english = "in the Divine Beast Cave"},
    {.key = "expanded.quest.dungeon.1", .english = "in the Wise Owl Forest"  },
    {.key = "expanded.quest.dungeon.2", .english = "in the Shipwreck"        },
    {.key = "expanded.quest.dungeon.3", .english = "in the Sun & Moon Temple"},
    {.key = "expanded.quest.dungeon.4", .english = "in the Moon Sea"         },
    {.key = "expanded.quest.dungeon.5", .english = "in the Gallery of Time"  },
};

// Whether another giver's quest under way wants the target.
bool Taken(const std::string &self, int dungeon, Monster monster) {
    for (const SlayerGiver &other : kGivers) {
        const dc::quest::Progress *progress = dc::quest::FindProgress(other.quest);
        if (other.quest == self || progress == nullptr || (progress->stage != "active" && progress->stage != "complete")) {
            continue;
        }
        const auto var = [&](const char *name) {
            const auto it = progress->vars.find(name);
            return it == progress->vars.end() ? -1 : it->second;
        };
        if (var("dungeon") == dungeon && var("monster") == No(monster)) {
            return true;
        }
    }
    return false;
}

SlayerTarget Roll(const Context &ctx) {
    std::vector<int> reached;
    for (int dungeon = 0; dungeon < kSlayerDungeons; ++dungeon) {
        if (ctx.Game().DeepestFloor(dungeon) >= 0) {
            reached.push_back(dungeon);
        }
    }
    if (reached.empty()) {
        reached.push_back(0);
    }
    SlayerTarget target{};
    for (int tries = 0; tries <= kSlayerRerolls; ++tries) {
        target.dungeon = reached[ModRoll(static_cast<int>(reached.size()))];
        target.monster = kSlayerTargets[target.dungeon][ModRoll(4)];
        if (!Taken(ctx.QuestId(), target.dungeon, target.monster)) {
            break;
        }
    }
    return target;
}

std::string Said(Text text, int count, Monster monster, int dungeon) {
    return LocalizeFormat(QuestText(text), {std::to_string(count), SlayerMonsterName(monster), QuestDungeonPlace(dungeon)});
}

const SlayerGiver &GiverOf(const std::string &quest) {
    for (const SlayerGiver &giver : kGivers) {
        if (giver.quest == quest) {
            return giver;
        }
    }
    return kGivers[0];
}

void Assign(Scene &scene, std::string said) {
    const SlayerGiver &giver = GiverOf(scene.QuestId());
    SlayerTarget       target = Roll(scene);
    if (giver.quest == kGivers[0].quest && scene.Var("given") == 0) {
        target = kMachoFirst;
    }
    const int count = kSlayerLeast + ModRoll(kSlayerMost - kSlayerLeast + 1);
    scene.SetVar("dungeon", target.dungeon);
    scene.SetVar("monster", No(target.monster));
    scene.SetVar("left", count);
    scene.SetVar("total", count);
    scene.SetVar("given", scene.Var("given") + 1);
    scene.SetStage("active");
    if (!said.empty()) {
        said += "{page}";
    }
    said += Said(kTask, count, target.monster, target.dungeon);
    scene.Session().PushRawMessage(said);
}

void Offer(Scene &scene) {
    const auto index = &GiverOf(scene.QuestId()) - kGivers.data();
    Assign(scene, scene.AtStage("unheard") ? QuestText(kIntros[index]) : std::string());
}

void About(Scene &scene) {
    if (scene.AtStage("active")) {
        scene.Session().PushRawMessage(
            Said(kStill, scene.Var("left"), static_cast<Monster>(scene.Var("monster")), scene.Var("dungeon")));
        return;
    }
    if (!scene.Game().GiveItem(kRewardItem)) {
        scene.Say(kNoRoom);
        return;
    }
    scene.SetStage("idle");
    if (scene.Game().ShowItemReceived(kRewardItem)) {
        scene.SayThenClose(kReward);
    } else {
        scene.Say(kReward);
    }
}

dc::quest::Definition Quest(const SlayerGiver &giver) {
    dc::quest::Builder quest(giver.quest);
    quest.Stages({"unheard", "idle", "active", "complete"});
    return Offers(quest, giver.giver, {"unheard", "idle"}, Offer, {"active", "complete"}, About).Build();
}

} // namespace

std::span<const SlayerGiver> SlayerGivers() {
    return kGivers;
}

std::vector<Text> MonsterSlayerTexts() {
    std::vector<Text> texts = {kTask, kStill, kReward, kNoRoom, kDone, kMimics};
    for (const SlayerGiver &giver : kGivers) {
        texts.push_back(giver.giver.name);
    }
    texts.insert(texts.end(), std::begin(kIntros), std::end(kIntros));
    for (const Named &named : kNames) {
        texts.push_back(named.name);
    }
    texts.insert(texts.end(), std::begin(kDungeons), std::end(kDungeons));
    return texts;
}

void RegisterMonsterSlayer() {
    for (const SlayerGiver &giver : kGivers) {
        dc::quest::Register(Quest(giver));
    }
}

void MonsterSlayerKill(int dungeon, Monster monster) {
    const int kind = No(BaseOf(monster));
    for (const SlayerGiver &giver : kGivers) {
        dc::quest::Act(giver.quest, [&](Context &ctx) {
            if (!ctx.AtStage("active") || ctx.Var("dungeon") != dungeon || ctx.Var("monster") != kind) {
                return;
            }
            const int left = std::max(ctx.Var("left") - 1, 0);
            ctx.SetVar("left", left);
            if (left == 0) {
                ctx.SetStage("complete");
                QuestNotice(LocalizeFormat(QuestText(kDone), {QuestText(giver.giver.name)}));
            }
            LogQuest(ctx);
        });
    }
}

bool MonsterSlayerQuest(const SlayerGiver &giver, SlayerQuest &quest) {
    const dc::quest::Progress *progress = dc::quest::FindProgress(giver.quest);
    if (progress == nullptr || (progress->stage != "active" && progress->stage != "complete")) {
        return false;
    }
    const auto var = [&](const char *name) {
        const auto it = progress->vars.find(name);
        return it == progress->vars.end() ? 0 : it->second;
    };
    quest.target = {.dungeon = var("dungeon"), .monster = static_cast<Monster>(var("monster"))};
    quest.left = var("left");
    quest.total = var("total");
    quest.complete = progress->stage == "complete";
    return true;
}

std::string SlayerMonsterName(Monster monster) {
    for (const Named &named : kNames) {
        if (named.monster == monster) {
            return QuestText(named.name);
        }
    }
    return QuestText(kMimics);
}

std::string QuestDungeonPlace(int dungeon) {
    return QuestText(kDungeons[std::clamp(dungeon, 0, kSlayerDungeons - 1)]);
}

} // namespace expanded
