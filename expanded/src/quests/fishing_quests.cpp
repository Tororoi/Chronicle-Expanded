#include "expanded/quests/fishing_quests.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <vector>

#include "expanded/core/random.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/town/fish_kinds.hpp"
#include "expanded/town/fishing.hpp"
#include "localize/localize.hpp"

namespace expanded {

using dc::quest::Context;
using dc::quest::Scene;
using dc::quest::Text;

namespace {

constexpr int kSamIndex = 2;

constexpr std::array kGivers = {
    FishingGiver{.quest = "fishing_pike",
                 .giver = {.map_no = kNorune, .npc_id = 5, .name = {.key = "expanded.quest.giver.pike", .english = "Pike"}},
                 .waters = FishPlace::NORUNE,
                 .size_least = 80,
                 .size_most = 140,
                 .size_width = 5,
                 .points_least = 35,
                 .points_most = 65,
                 .size_bonus = 0 },
    FishingGiver{.quest = "fishing_pao",
                 .giver = {.map_no = kMatataki, .npc_id = 7, .name = {.key = "expanded.quest.giver.pao", .english = "Pao"}},
                 .waters = FishPlace::MATATAKI,
                 .size_least = 80,
                 .size_most = 140,
                 .size_width = 5,
                 .points_least = 43,
                 .points_most = 73,
                 .size_bonus = 10},
    FishingGiver{.quest = "fishing_sam",
                 .giver = {.map_no = kQueens, .npc_id = 11, .name = {.key = "expanded.quest.giver.sam", .english = "Sam"}},
                 .waters = FishPlace::EAST_HARBOR,
                 .size_least = 90,
                 .size_most = 160,
                 .size_width = 10,
                 .points_least = 51,
                 .points_most = 81,
                 .size_bonus = 20},
    FishingGiver{.quest = "fishing_devia",
                 .giver = {.map_no = kMuskaLacka, .npc_id = 5, .name = {.key = "expanded.quest.giver.devia", .english = "Devia"}},
                 .waters = FishPlace::OASIS,
                 .size_least = 100,
                 .size_most = 180,
                 .size_width = 5,
                 .points_least = 60,
                 .points_most = 90,
                 .size_bonus = 30},
};

// Each giver's first word of the quests, in kGivers' order.
constexpr Text kIntros[] = {
    {.key = "expanded.quest.fishing.intro.pike",
     .english = "Oh, I bet you can already guess what I have in store for you.{page}"
                "That's right, fishing quests!{page}"
                "There are two kinds, and whenever you talk to me, I can give you one of them.{page}"
                "I hope you like fishing!"                                      },
    {.key = "expanded.quest.fishing.intro.pao",
     .english = "Show me your hunting skills!{page}"
                "Wait, you don't hunt anything?{page}"
                "Well, I guess fishing could count as hunting. I can give you some fishing quests.{page}"
                "Keep in mind that there are two places to fish here!"          },
    {.key = "expanded.quest.fishing.intro.sam",
     .english = "Fighting monsters in the dungeons must be exhausting.{page}"
                "Why don't you relax with some fishing quests?{page}"
                "Oh, by the way, I also have a special reward!"                 },
    {.key = "expanded.quest.fishing.intro.devia",
     .english = "I heard some tourists like to fish at the oasis, since you can apparently find rare fish in there.{page}"
                "And speaking of fishing... would you like some fishing quests?"},
};

constexpr Text kCountTask = {.key = "expanded.quest.fishing.count.task",
                             .english = "Your quest is to catch %1 %2 %3.{page}Good luck!"};
constexpr Text kSizeTask = {.key = "expanded.quest.fishing.size.task",
                            .english = "Your quest is to catch any fish %1 to %2 cm long %3.{page}Good luck!"};
constexpr Text kCountStill = {.key = "expanded.quest.fishing.count.still",
                              .english = "You're still on the quest to catch %2 %3. Just %1 left!"};
constexpr Text kSizeStill = {.key = "expanded.quest.fishing.size.still",
                             .english = "You're still on the quest to catch any fish %1 to %2 cm long %3. Good luck!"};
constexpr Text kReward = {.key = "expanded.quest.fishing.reward",
                          .english = "Nicely done!{page}Here's your reward: %1 fishing points!"};
constexpr Text kSamMore = {.key = "expanded.quest.fishing.sam.more",
                           .english = "And if you complete %1 more of my quests, I'll give you a special reward!"};
constexpr Text kSamSpecialText = {.key = "expanded.quest.fishing.sam.special",
                                  .english = "Awesome, you completed three quests for me!{page}"
                                             "As a special reward, you can now see the fish at East Harbor! Cool, right?{page}"
                                             "And here's the regular reward: %1 fishing points!"};

constexpr int kNone = -1;

struct Place {
    FishPlace          place;
    std::array<int, 2> spots; // kNone where there is no second
    Text               name;
};

constexpr Place kPlaces[] = {
    {FishPlace::NORUNE,      {kNoruneArea, kNone},              {.key = "expanded.quest.place.norune", .english = "at the Norune pond"}            },
    {FishPlace::PEANUT_POND, {kPeanutPond, kNone},              {.key = "expanded.quest.place.peanut_pond", .english = "at Peanut Pond"}           },
    {FishPlace::WATERFALL,   {kMatatakiWaterfall, kNone},       {.key = "expanded.quest.place.waterfall", .english = "at the Matataki waterfall"}  },
    {FishPlace::MATATAKI,    {kPeanutPond, kMatatakiWaterfall}, {.key = "expanded.quest.place.matataki", .english = "anywhere in Matataki Village"}},
    {FishPlace::EAST_HARBOR, {kEastHarbor, kNone},              {.key = "expanded.quest.place.east_harbor", .english = "at East Harbor"}           },
    {FishPlace::OASIS,       {kMuskaLackaOasis, kNone},         {.key = "expanded.quest.place.oasis", .english = "at the oasis"}                   },
};

struct FishName {
    int  kind;
    Text name;
};

constexpr FishName kFishNames[] = {
    {fish::kNiler,         {.key = "expanded.quest.fish.niler", .english = "Nilers"}                 },
    {fish::kGummy,         {.key = "expanded.quest.fish.gummy", .english = "Gummies"}                },
    {fish::kNonky,         {.key = "expanded.quest.fish.nonky", .english = "Nonkies"}                },
    {fish::kGobbler,       {.key = "expanded.quest.fish.gobbler", .english = "Gobblers"}             },
    {fish::kBakuBaku,      {.key = "expanded.quest.fish.baku_baku", .english = "Baku Bakus"}         },
    {fish::kTarton,        {.key = "expanded.quest.fish.tarton", .english = "Tartons"}               },
    {fish::kUmadakara,     {.key = "expanded.quest.fish.umadakara", .english = "Umadakaras"}         },
    {fish::kMardanGarayan, {.key = "expanded.quest.fish.mardan_garayan", .english = "Mardan Garayan"}},
    {fish::kBaronGarayan,  {.key = "expanded.quest.fish.baron_garayan", .english = "Baron Garayan"}  },
    {fish::kBobo,          {.key = "expanded.quest.fish.bobo", .english = "Bobos"}                   },
    {fish::kKaiji,         {.key = "expanded.quest.fish.kaji", .english = "Kajis"}                   },
    {fish::kBon,           {.key = "expanded.quest.fish.bon", .english = "Bons"}                     },
    {fish::kHamahama,      {.key = "expanded.quest.fish.hamahama", .english = "Hamahamas"}           },
    {fish::kNegie,         {.key = "expanded.quest.fish.negie", .english = "Negies"}                 },
    {fish::kDen,           {.key = "expanded.quest.fish.den", .english = "Dens"}                     },
    {fish::kHeela,         {.key = "expanded.quest.fish.heela", .english = "Heelas"}                 },
};

// The kinds a count quest picks from, waters by waters (the game's pools, with Expanded's changes: PoolFish).
struct Pool {
    FishPlace          place;
    std::array<int, 5> fish; // kNone past the last
};

constexpr Pool kPools[] = {
    {FishPlace::NORUNE,      {fish::kNiler, fish::kGummy, fish::kNonky, fish::kGobbler, kNone}                       },
    {FishPlace::PEANUT_POND, {fish::kBakuBaku, fish::kGobbler, fish::kTarton, fish::kUmadakara, kNone}               },
    {FishPlace::WATERFALL,   {fish::kBakuBaku, fish::kNonky, fish::kNiler, fish::kMardanGarayan, fish::kBaronGarayan}},
    {FishPlace::EAST_HARBOR, {fish::kBobo, fish::kKaiji, fish::kGobbler, fish::kBon, fish::kHamahama}                },
    {FishPlace::OASIS,       {fish::kNegie, fish::kDen, fish::kHeela, fish::kMardanGarayan, fish::kBaronGarayan}     },
};

bool Garayan(int kind) {
    return std::ranges::contains(fish::kGarayans, kind);
}

const Place &PlaceOf(FishPlace place) {
    for (const Place &known : kPlaces) {
        if (known.place == place) {
            return known;
        }
    }
    return kPlaces[0];
}

const FishingGiver &GiverOf(const std::string &quest) {
    for (const FishingGiver &giver : kGivers) {
        if (giver.quest == quest) {
            return giver;
        }
    }
    return kGivers[0];
}

int IndexOf(const FishingGiver &giver) {
    return static_cast<int>(&giver - kGivers.data());
}

int Roll(int least, int most) {
    return least + ModRoll(most - least + 1);
}

// A count quest's kind and where it is caught: a Garayan is rolled again once, as the mod has it.
void RollCount(const FishingGiver &giver, int &kind, FishPlace &place) {
    place = giver.waters;
    if (place == FishPlace::MATATAKI) {
        place = ModRoll(2) == 0 ? FishPlace::PEANUT_POND : FishPlace::WATERFALL;
    }
    for (const Pool &pool : kPools) {
        if (pool.place != place) {
            continue;
        }
        std::vector<int> fish;
        std::ranges::copy_if(pool.fish, std::back_inserter(fish), [](int kind) { return kind != kNone; });
        kind = fish[ModRoll(static_cast<int>(fish.size()))];
        if (Garayan(kind)) {
            kind = fish[ModRoll(static_cast<int>(fish.size()))];
        }
    }
    // The Baku Baku swims in both of Matataki's waters.
    if (kind == fish::kBakuBaku) {
        place = FishPlace::MATATAKI;
    }
}

std::string Task(const Context &ctx, Text count, Text size) {
    const std::string place = QuestFishPlace(static_cast<FishPlace>(ctx.Var("place")));
    if (ctx.Var("size") != 0) {
        return LocalizeFormat(QuestText(size), {std::to_string(ctx.Var("least")), std::to_string(ctx.Var("most")), place});
    }
    return LocalizeFormat(QuestText(count), {std::to_string(ctx.Var("left")), QuestFishName(ctx.Var("fish")), place});
}

// Sam's word of the special reward, while it is still to come.
std::string SamMore(const Context &ctx) {
    if (IndexOf(GiverOf(ctx.QuestId())) != kSamIndex || ctx.Var("done") >= kSamSpecial) {
        return {};
    }
    return "{page}" + LocalizeFormat(QuestText(kSamMore), {std::to_string(kSamSpecial - ctx.Var("done"))});
}

void Assign(Scene &scene, std::string said) {
    const FishingGiver &giver = GiverOf(scene.QuestId());
    if (ModRoll(2) == 0) {
        int       kind = fish::kNiler;
        FishPlace place = giver.waters;
        RollCount(giver, kind, place);
        const int count = Garayan(kind) ? 1 : kFishCount;
        scene.SetVar("size", 0);
        scene.SetVar("fish", kind);
        scene.SetVar("left", count);
        scene.SetVar("total", count);
        scene.SetVar("place", static_cast<int>(place));
    } else {
        const int least = Roll(giver.size_least, giver.size_most);
        scene.SetVar("size", 1);
        scene.SetVar("least", least);
        scene.SetVar("most", least + giver.size_width);
        scene.SetVar("place", static_cast<int>(giver.waters));
    }
    scene.SetStage("active");
    if (!said.empty()) {
        said += "{page}";
    }
    said += Task(scene, kCountTask, kSizeTask) + SamMore(scene);
    scene.Session().PushRawMessage(said);
}

void Offer(Scene &scene) {
    const int index = IndexOf(GiverOf(scene.QuestId()));
    Assign(scene, scene.AtStage("unheard") ? QuestText(kIntros[index]) : std::string());
}

int Points(const Context &ctx) {
    const FishingGiver &giver = GiverOf(ctx.QuestId());
    if (ctx.Var("size") != 0) {
        const int most = ctx.Var("most");
        return Roll(most - kSizeReward, most - 1) + giver.size_bonus;
    }
    int points = Roll(giver.points_least, giver.points_most) * ctx.Var("total");
    if (giver.waters == FishPlace::MATATAKI && Garayan(ctx.Var("fish"))) {
        points *= 2;
    }
    return points;
}

void About(Scene &scene) {
    if (scene.AtStage("active")) {
        scene.Session().PushRawMessage(Task(scene, kCountStill, kSizeStill) + SamMore(scene));
        return;
    }
    const int points = Points(scene);
    scene.Game().EarnFishingPoints(points);
    scene.SetStage("idle");
    const bool special = IndexOf(GiverOf(scene.QuestId())) == kSamIndex && scene.Var("done") >= kSamSpecial &&
                         scene.Var("shown") == 0;
    if (special) {
        scene.SetVar("shown", 1);
        scene.Session().PushRawMessage(LocalizeFormat(QuestText(kSamSpecialText), {std::to_string(points)}));
        return;
    }
    scene.Session().PushRawMessage(LocalizeFormat(QuestText(kReward), {std::to_string(points)}) + SamMore(scene));
}

dc::quest::Definition Quest(const FishingGiver &giver) {
    dc::quest::Builder quest(giver.quest);
    quest.Stages({"unheard", "idle", "active", "complete"});
    return Offers(quest, giver.giver, {"unheard", "idle"}, Offer, {"active", "complete"}, About).Build();
}

bool CaughtThere(FishPlace place, int spot) {
    return spot != kNone && std::ranges::contains(PlaceOf(place).spots, spot);
}

} // namespace

std::span<const FishingGiver> FishingGivers() {
    return kGivers;
}

std::vector<Text> FishingQuestTexts() {
    std::vector<Text> texts = {kCountTask, kSizeTask, kCountStill, kSizeStill, kReward, kSamMore, kSamSpecialText};
    for (const FishingGiver &giver : kGivers) {
        texts.push_back(giver.giver.name);
    }
    texts.insert(texts.end(), std::begin(kIntros), std::end(kIntros));
    for (const Place &place : kPlaces) {
        texts.push_back(place.name);
    }
    for (const FishName &name : kFishNames) {
        texts.push_back(name.name);
    }
    return texts;
}

void RegisterFishingQuests() {
    for (const FishingGiver &giver : kGivers) {
        dc::quest::Register(Quest(giver));
    }
}

void FishingQuestCatch(int fish_kind, int size, int spot) {
    for (const FishingGiver &giver : kGivers) {
        dc::quest::Act(giver.quest, [&](Context &ctx) {
            if (!ctx.AtStage("active") || !CaughtThere(static_cast<FishPlace>(ctx.Var("place")), spot)) {
                return;
            }
            if (ctx.Var("size") != 0) {
                if (size < ctx.Var("least") || size > ctx.Var("most")) {
                    return;
                }
                ctx.SetVar("left", 0);
            } else if (fish_kind == ctx.Var("fish")) {
                ctx.SetVar("left", std::max(ctx.Var("left") - 1, 0));
            } else {
                return;
            }
            if (ctx.Var("left") == 0) {
                ctx.SetStage("complete");
                ctx.SetVar("done", ctx.Var("done") + 1);
            }
            LogQuest(ctx);
        });
    }
}

bool EastHarborFishShown() {
    const dc::quest::Progress *progress = dc::quest::FindProgress(kGivers[kSamIndex].quest);
    if (progress == nullptr) {
        return false;
    }
    const auto shown = progress->vars.find("shown");
    return shown != progress->vars.end() && shown->second != 0;
}

bool FishingQuestOf(const FishingGiver &giver, FishingQuest &quest) {
    const dc::quest::Progress *progress = dc::quest::FindProgress(giver.quest);
    if (progress == nullptr || (progress->stage != "active" && progress->stage != "complete")) {
        return false;
    }
    const auto var = [&](const char *name) {
        const auto it = progress->vars.find(name);
        return it == progress->vars.end() ? 0 : it->second;
    };
    quest.size = var("size") != 0;
    quest.fish = var("fish");
    quest.left = var("left");
    quest.total = var("total");
    quest.least = var("least");
    quest.most = var("most");
    quest.place = static_cast<FishPlace>(var("place"));
    quest.complete = progress->stage == "complete";
    return true;
}

std::string QuestFishName(int fish_kind) {
    for (const FishName &name : kFishNames) {
        if (name.kind == fish_kind) {
            return QuestText(name.name);
        }
    }
    return {};
}

std::string QuestFishPlace(FishPlace place) {
    return QuestText(PlaceOf(place).name);
}

} // namespace expanded
