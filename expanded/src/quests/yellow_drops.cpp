#include "expanded/quests/yellow_drops.hpp"

#include <string>

#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "dungeonparts.hpp"
#include "expanded/enemies/monster_queries.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "monstorunit.hpp"
#include "userstatus.hpp"

namespace expanded {

using dc::quest::Context;
using dc::quest::Scene;
using dc::quest::Text;

namespace {

constexpr int kParty = 6;
constexpr int kMonsters = 16;

constexpr Giver kLedan = {
    .map_no = kYellowDrops, .npc_id = 0, .name = {.key = "expanded.quest.giver.ledan", .english = "Ledan"}
};
constexpr Giver kSamba = {
    .map_no = kYellowDrops, .npc_id = 1, .name = {.key = "expanded.quest.giver.samba", .english = "Samba"}
};

constexpr const char *kPuzzle = "yd_crystal";
constexpr const char *kChallenge = "yd_map";

constexpr Text kPuzzleIntro = {.key = "expanded.quest.yd.puzzle.intro",
                               .english = "I have an extremely valuable crystal.{page}"
                                          "I know you're strong, but if you want my crystal, you'll need to prove "
                                          "you're smart as well!{page}"
                                          "I have a puzzle for you."};
constexpr Text kPuzzleHint = {.key = "expanded.quest.yd.puzzle.hint",
                              .english = "Show me four items in the right order: put them in the first four slots of "
                                         "your bag.{page}"
                                         "What are the items? I'm not telling you straight out! However...{page}"
                                         "On Blue Terra, your homeworld, you might have met some people who needed "
                                         "something.{page}"
                                         "Perhaps they're the clue to the right answer?"};
constexpr Text kPuzzleSolved = {.key = "expanded.quest.yd.puzzle.solved",
                                .english = "That's correct, well done! You solved the puzzle!{page}"
                                           "As a reward, have this ultra-rare Magical Crystal.{page}"
                                           "While it's in your bag, it works like the Magical Crystal you find in the "
                                           "dungeons, but it's always on!{page}"
                                           "And the Magical Crystals you find in the dungeons become extra loot. Worth "
                                           "it, right?"};
constexpr Text kPuzzleNoRoom = {.key = "expanded.quest.yd.puzzle.no_room",
                                .english = "That's correct, well done!{page}"
                                           "Make some room in your bag for my crystal, and talk to me again."};

constexpr Text kChallengeTask = {.key = "expanded.quest.yd.challenge.task",
                                 .english = "I have a challenge to test your true strength.{page}"
                                            "If you can clear the back floor of the Moon Sea's floor 7 as {name1}, "
                                            "using only the Dagger, you can have my reward.{page}"
                                            "Its healing springs are dry, so be sure to stock up."};
constexpr Text kChallengeStill = {.key = "expanded.quest.yd.challenge.still",
                                  .english = "If you can clear the back floor of the Moon Sea's floor 7 as {name1}, "
                                             "using only the Dagger, you can have my reward.{page}"
                                             "Its healing springs are dry, so be sure to stock up."};
constexpr Text kChallengeReward = {.key = "expanded.quest.yd.challenge.reward",
                                   .english = "You cleared the challenge? Extraordinary!{page}"
                                              "Here, you can have this special Map.{page}"
                                              "While it's in your bag, it works like the Map you find in the dungeons, "
                                              "but it's always on!{page}"
                                              "And the Maps you find in the dungeons become extra loot. Pretty cool, "
                                              "huh?"};
constexpr Text kChallengeNoRoom = {.key = "expanded.quest.yd.challenge.no_room",
                                   .english = "You cleared the challenge? Extraordinary!{page}"
                                              "Make some room in your bag for my reward, and talk to me again."};

constexpr Text kBegun = {.key = "expanded.quest.yd.challenge.begun",
                         .english = "Samba's challenge has begun!\nDefeat every monster with only the Dagger."};
constexpr Text kNotBegun = {.key = "expanded.quest.yd.challenge.not_begun",
                            .english = "Samba's challenge did not begin.\nCome back as {name1} with the Dagger."};
constexpr Text kEnded = {.key = "expanded.quest.yd.challenge.ended",
                         .english = "Samba's challenge has ended.\nCome back to this floor to try again."};
constexpr Text kCleared = {.key = "expanded.quest.yd.challenge.cleared",
                           .english = "Samba's challenge is complete. Well done!"};

// The challenge on its floor: how long the player has been there, and whether it is on and has met a monster.
struct Attempt {
    int  frames = 0;
    bool on = false;
    bool met = false;
    bool thrown = false;
};

Attempt attempt;

bool AtStage(const char *quest, const char *stage) {
    const dc::quest::Progress *progress = dc::quest::FindProgress(quest);
    return progress != nullptr && progress->stage == stage;
}

bool Solved(const Context &ctx) {
    for (size_t slot = 0; slot < kPuzzleItems.size(); ++slot) {
        if (ctx.Game().PackItem(static_cast<int>(slot)) != kPuzzleItems[slot]) {
            return false;
        }
    }
    return true;
}

void PuzzleOffer(Scene &scene) {
    scene.Session().PushRawMessage(QuestText(kPuzzleIntro) + "{page}" + QuestText(kPuzzleHint));
    scene.SetStage("asked");
}

void PuzzleAbout(Scene &scene) {
    if (!Solved(scene)) {
        scene.Say(kPuzzleHint);
        return;
    }
    if (!scene.Game().GiveItem(ITEM_MAGICAL_CRYSTAL)) {
        scene.Say(kPuzzleNoRoom);
        return;
    }
    scene.SetStage("done");
    scene.Say(kPuzzleSolved);
}

void ChallengeOffer(Scene &scene) {
    scene.Say(kChallengeTask);
    scene.SetStage("taken");
}

void ChallengeAbout(Scene &scene) {
    if (scene.AtStage("taken")) {
        scene.Say(kChallengeStill);
        return;
    }
    if (!scene.Game().GiveItem(ITEM_MAP)) {
        scene.Say(kChallengeNoRoom);
        return;
    }
    scene.SetStage("done");
    scene.Say(kChallengeReward);
}

dc::quest::Condition FullParty() {
    return [](const Context &ctx) { return ctx.Game().PartySize() >= kParty; };
}

bool OnTheFloor() {
    return UserStatus != nullptr && selectMapNo == kChallengeDungeon && UserStatus->cur_floor == kChallengeFloor &&
           BtUraDongeon != 0;
}

bool ToanWithTheDagger() {
    if (UserStatus == nullptr || UserStatus->cur_chara != CHARA_TOAN) {
        return false;
    }
    const WEAPON_HAVE *weapon = EquippedWeapon(*UserStatus, CHARA_TOAN);
    return weapon != nullptr && weapon->item_no == ITEM_WEAPON_DAGGER;
}

int MonstersLiving() {
    int living = 0;
    for (int slot = 0; slot < kMonsters; ++slot) {
        const MONSTOR *monster = FloorMonster(slot);
        living += monster != nullptr && MonsterLiving(*monster) ? 1 : 0;
    }
    return living;
}

bool Carried(int item) {
    const dc::quest::IGame *game = dc::quest::Game();
    return game != nullptr && game->ItemCount(item) > 0;
}

} // namespace

std::vector<Text> YellowDropsTexts() {
    return {kLedan.name, kSamba.name, kPuzzleIntro, kPuzzleHint, kPuzzleSolved, kPuzzleNoRoom,
            kChallengeTask, kChallengeStill, kChallengeReward, kChallengeNoRoom, kBegun, kNotBegun,
            kEnded, kCleared};
}

void RegisterYellowDropsQuests() {
    dc::quest::Builder puzzle(kPuzzle);
    puzzle.Stages({"hidden", "offered", "asked", "done"}).Advance("hidden", FullParty(), "offered");
    dc::quest::Register(Offers(puzzle, kLedan, {"offered"}, PuzzleOffer, {"asked"}, PuzzleAbout).Build());

    dc::quest::Builder challenge(kChallenge);
    challenge.Stages({"hidden", "offered", "taken", "cleared", "done"}).Advance("hidden", FullParty(), "offered");
    dc::quest::Register(
        Offers(challenge, kSamba, {"offered"}, ChallengeOffer, {"taken", "cleared"}, ChallengeAbout).Build());
}

void ItemThrown() {
    attempt.thrown = true;
}

bool SpringsDry() {
    return OnTheFloor() && AtStage(kChallenge, "taken");
}

void MoonSeaChallengeStep() {
    if (!OnTheFloor() || !AtStage(kChallenge, "taken")) {
        attempt = {};
        return;
    }
    if (++attempt.frames == kChallengeSettle) {
        attempt.on = ToanWithTheDagger();
        attempt.thrown = false;
        QuestNotice(QuestText(attempt.on ? kBegun : kNotBegun));
        return;
    }
    if (!attempt.on) {
        return;
    }
    if (!ToanWithTheDagger() || attempt.thrown) {
        attempt.on = false;
        QuestNotice(QuestText(kEnded));
        return;
    }
    const int living = MonstersLiving();
    attempt.met = attempt.met || living > 0;
    if (attempt.met && living == 0) {
        attempt.on = false;
        dc::quest::Act(kChallenge, [](Context &ctx) {
            ctx.SetStage("cleared");
            LogQuest(ctx);
        });
        QuestNotice(QuestText(kCleared));
    }
}

void CarriedMapChests(CDungeonMap &map, int dungeon, int floor, bool back_floor) {
    if (back_floor) {
        return;
    }
    const bool map_carried = Carried(ITEM_MAP);
    const bool crystal_carried = Carried(ITEM_MAGICAL_CRYSTAL);
    for (TREASURE_BOX &box : map.boxes) {
        if (box.used == 0 || box.kind != TREASURE_BOX_SMALL) {
            continue;
        }
        if ((box.item_no == ITEM_MAP && map_carried) || (box.item_no == ITEM_MAGICAL_CRYSTAL && crystal_carried)) {
            const int item = PresetSmallItemNo_Get(dungeon, floor, 0, 1);
            if (item != -1 && item != ITEM_MAP && item != ITEM_MAGICAL_CRYSTAL) {
                box.item_no = item;
            }
        }
    }
}

void CarriedMapsAct() {
    if (Carried(ITEM_MAP)) {
        BtEquipMap = 1;
    }
    if (Carried(ITEM_MAGICAL_CRYSTAL)) {
        BtEquipMasuisyou = 1;
    }
}

bool PuzzleOn() {
    return AtStage(kPuzzle, "asked");
}

bool ChallengeOn() {
    return AtStage(kChallenge, "taken");
}

bool ChallengeCleared() {
    return AtStage(kChallenge, "cleared");
}

} // namespace expanded
