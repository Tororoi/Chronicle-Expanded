#include "expanded/quests/tracker.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "clsmes.hpp"
#include "expanded/quests/fishing_quests.hpp"
#include "expanded/quests/item_hunts.hpp"
#include "expanded/quests/monster_slayer.hpp"
#include "expanded/quests/yellow_drops.hpp"
#include "gamepad.hpp"
#include "gameutil.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "menu_draw.hpp"
#include "menuetc.hpp"
#include "savedata.hpp"
#include "snd.hpp"
#include "text/gametext.hpp"
#include "ui/framework.hpp"

namespace expanded {

using dc::quest::Text;
namespace ui = dc::ui;

namespace {

constexpr Text kCategory = {.key = "expanded.quest.tracker.category", .english = "Quests"};
constexpr Text kTitle = {.key = "expanded.quest.tracker.title", .english = "JOURNAL: QUESTS"};
constexpr Text kUnderWay = {.key = "expanded.quest.tracker.under_way", .english = "UNDER WAY"};
constexpr Text kNone = {.key = "expanded.quest.tracker.none",
                        .english = "No quests are under way. A villager with work for you has a choice for it when you "
                                   "talk to them."};
constexpr Text kControls = {.key = "expanded.quest.tracker.controls", .english = "{up} / {down} Select   {circle} Return"};
constexpr Text kComplete = {.key = "expanded.quest.tracker.complete", .english = "Done! Go back to %1."};
constexpr Text kProgress = {.key = "expanded.quest.tracker.progress", .english = "Progress"};
constexpr Text kSlayer = {.key = "expanded.quest.tracker.slayer", .english = "Defeat %1 %2 %3."};
constexpr Text kFishCountTask = {.key = "expanded.quest.tracker.fish_count", .english = "Catch %1 %2 %3."};
constexpr Text kFishSizeTask = {.key = "expanded.quest.tracker.fish_size", .english = "Catch any fish %1 to %2 cm long %3."};
constexpr Text kHunt = {.key = "expanded.quest.tracker.hunt",
                        .english = "Find the rare item lost %1, on its back floors."};
constexpr Text kPuzzle = {.key = "expanded.quest.tracker.puzzle",
                          .english = "Show %1 the four items Blue Terra's people needed, in order, in your bag's first "
                                     "four slots."};
constexpr Text kChallenge = {.key = "expanded.quest.tracker.challenge",
                             .english = "Clear the Moon Sea's floor 7 back floor as %1, using only the Dagger."};

constexpr int kPaneTop = 132;
constexpr int kListTop = 156;
constexpr int kRowSpacing = 25;
constexpr int kRowsShown = 6;
constexpr int kLeftX = 92;
constexpr int kDividerX = 258;
constexpr int kRightX = 274;
constexpr int kRightWidth = 268;
constexpr int kNameLength = 11; // the most a name holds, as ClsMes reads one

int           selected = 0;
int           first_row = 0;
ui::Cursor    cursor;
ui::Brackets  brackets;
ui::ScrollBar scroll;

// The town a giver stands in, by the chest tracker's names for them.
std::string TownOf(const Giver &giver) {
    const int             town = giver.map_no == kYellowDrops ? 4 : giver.map_no;
    constexpr const char *kEnglish[] = {"Norune Village", "Matataki Village", "Queens", "Muska Lacka", "Yellow Drops"};
    return LocalizeText("tracker.town." + std::to_string(town), kEnglish[std::clamp(town, 0, 4)]);
}

// Toan's name as the player named him: the name entry's codes, each the game's text code it shows as.
std::string ToanName() {
    const s16 *name = SaveData != nullptr ? SaveData->GetCharaName(0) : nullptr;
    if (name == nullptr || name[0] == 0) {
        return "Toan";
    }
    std::vector<s16> codes;
    for (int i = 0; i < kNameLength && name[i] != 0; ++i) {
        codes.push_back(static_cast<s16>(NameRegistCodeJtoE(static_cast<u16>(name[i]) - 1)));
    }
    codes.push_back(MES_CODE_END);
    return GameTextDecode(codes.data());
}

TrackedQuest Of(const Giver &giver, std::string task) {
    TrackedQuest quest;
    quest.giver = QuestText(giver.name);
    quest.town = TownOf(giver);
    quest.task = std::move(task);
    return quest;
}

void Clamp(int count) {
    selected = std::clamp(selected, 0, std::max(count - 1, 0));
    first_row = std::clamp(first_row, std::max(selected - kRowsShown + 1, 0), selected);
}

} // namespace

std::vector<TrackedQuest> TrackedQuests() {
    std::vector<TrackedQuest> quests;
    for (const SlayerGiver &giver : SlayerGivers()) {
        SlayerQuest slayer;
        if (!MonsterSlayerQuest(giver, slayer)) {
            continue;
        }
        TrackedQuest quest = Of(giver.giver, LocalizeFormat(QuestText(kSlayer), {std::to_string(slayer.total),
                                                                                 SlayerMonsterName(slayer.target.monster),
                                                                                 QuestDungeonPlace(slayer.target.dungeon)}));
        quest.total = slayer.total;
        quest.done = slayer.total - slayer.left;
        quest.complete = slayer.complete;
        quest.item = ITEM_POWERUP_POWDER;
        quests.push_back(std::move(quest));
    }
    for (const FishingGiver &giver : FishingGivers()) {
        FishingQuest fishing;
        if (!FishingQuestOf(giver, fishing)) {
            continue;
        }
        const std::string place = QuestFishPlace(fishing.place);
        TrackedQuest      quest = Of(
            giver.giver, fishing.size ? LocalizeFormat(QuestText(kFishSizeTask),
                                                            {std::to_string(fishing.least), std::to_string(fishing.most), place})
                                           : LocalizeFormat(QuestText(kFishCountTask),
                                                            {std::to_string(fishing.total), QuestFishName(fishing.fish), place}));
        quest.total = fishing.size ? 1 : fishing.total;
        quest.done = fishing.size ? (fishing.complete ? 1 : 0) : fishing.total - fishing.left;
        quest.complete = fishing.complete;
        quests.push_back(std::move(quest));
    }
    for (const ItemHunt &hunt : ItemHunts()) {
        if (!ItemHuntOn(hunt)) {
            continue;
        }
        TrackedQuest quest = Of(hunt.giver, LocalizeFormat(QuestText(kHunt), {QuestDungeonPlace(hunt.dungeon)}));
        quests.push_back(std::move(quest));
    }
    if (PuzzleOn()) {
        TrackedQuest quest = Of(Ledan(), LocalizeFormat(QuestText(kPuzzle), {QuestText(Ledan().name)}));
        quest.item = ITEM_MAGICAL_CRYSTAL;
        quests.push_back(std::move(quest));
    }
    if (ChallengeOn() || ChallengeCleared()) {
        TrackedQuest quest = Of(Samba(), LocalizeFormat(QuestText(kChallenge), {ToanName()}));
        quest.complete = ChallengeCleared();
        quest.done = quest.complete ? 1 : 0;
        quest.item = ITEM_MAP;
        quests.push_back(std::move(quest));
    }
    return quests;
}

std::string TrackedQuestStatus(const TrackedQuest &quest) {
    return quest.complete ? LocalizeFormat(QuestText(kComplete), {quest.giver}) : std::string();
}

std::string QuestTrackerCategory() {
    return QuestText(kCategory);
}

std::vector<Text> QuestTrackerTexts() {
    return {kCategory, kTitle, kUnderWay, kNone, kControls, kComplete, kProgress,
            kSlayer, kFishCountTask, kFishSizeTask, kHunt, kPuzzle, kChallenge};
}

void QuestTrackerOpen() {
    selected = 0;
    first_row = 0;
    cursor.SnapTo(48.0f, static_cast<float>(kListTop - 2));
}

bool QuestTrackerKey() {
    if (GamePad.Down(PAD_CIRCLE)) {
        ComMenuSePlay(MENU_SOUND_REFUSE);
        return true;
    }
    const int count = static_cast<int>(TrackedQuests().size());
    if (count == 0) {
        return false;
    }
    const int before = selected;
    if (GamePad.Down(PAD_UP)) {
        selected = selected > 0 ? selected - 1 : count - 1;
    }
    if (GamePad.Down(PAD_DOWN)) {
        selected = selected < count - 1 ? selected + 1 : 0;
    }
    Clamp(count);
    if (selected != before) {
        ComMenuSePlay(MENU_SOUND_CURSOR);
    }
    scroll.Update(selected != before);
    return false;
}

void QuestTrackerPointer(float x, float y) {
    const int count = static_cast<int>(TrackedQuests().size());
    if (x < kLeftX - 8 || x >= kDividerX || y < kListTop - 4 || y >= kListTop - 4 + kRowsShown * kRowSpacing) {
        return;
    }
    const int row = first_row + static_cast<int>((y - (kListTop - 4)) / kRowSpacing);
    if (row < count && row != selected) {
        selected = row;
        ComMenuSePlay(MENU_SOUND_CURSOR);
    }
}

void QuestTrackerDraw() {
    constexpr float kHeaderScale = 0.92f;
    constexpr float kBodyScale = 0.84f;

    ui::TrackerWindow::DrawPaper(64, 91, 512, 262, 0x80);
    ui::TrackerWindow::DrawStoneBanner(ui::Rect(150, 74, 340, 28), 0x80, 0x80, QuestText(kTitle));
    MenuTextureReload(0x1A);

    const std::vector<TrackedQuest> quests = TrackedQuests();
    if (quests.empty()) {
        ui::Label(QuestText(kNone), ui::ColorWhite, ui::Alignment::Left, true, kBodyScale).DrawWrapped(100, 180, 440, 24, 0x80);
    } else {
        Clamp(static_cast<int>(quests.size()));
        ui::Divider::DrawVertical(kDividerX, kPaneTop, 338, 0x80);
        ui::Label(QuestText(kUnderWay), ui::ColorYellow, ui::Alignment::Left, true, kHeaderScale).Draw(kLeftX, kPaneTop, 0x80);

        const int last = std::min(first_row + kRowsShown, static_cast<int>(quests.size()));
        for (int row = first_row; row < last; ++row) {
            const int  y = kListTop + (row - first_row) * kRowSpacing;
            const bool chosen = row == selected;
            const u32  color = quests[row].complete ? ui::ColorGold : (chosen ? ui::ColorYellow : ui::ColorWhite);
            ui::Label(quests[row].giver, color, ui::Alignment::Left, true, kBodyScale).Draw(kLeftX, y, chosen ? 0x80 : 0x58);
        }
        if (static_cast<int>(quests.size()) > kRowsShown) {
            scroll.Draw(kDividerX - 12, kListTop - 4, kRowsShown * kRowSpacing, static_cast<int>(quests.size()), kRowsShown,
                        first_row, 0x80);
        }

        const TrackedQuest &quest = quests[selected];
        int                 title_x = kRightX;
        if (quest.item >= 0 && ui::ItemIcon::Draw(quest.item, kRightX + 2, kPaneTop - 1, 20, 0x80)) {
            title_x += 30;
        }
        MenuTextureReload(0x1A);
        ui::Label(quest.giver, ui::ColorYellow, ui::Alignment::Left, true, kHeaderScale).Draw(title_x, kPaneTop, 0x80);
        ui::Label(quest.town, ui::ColorGold, ui::Alignment::Left, true, kBodyScale).Draw(kRightX, kPaneTop + 24, 0x80);
        ui::Label(quest.task, ui::ColorWhite, ui::Alignment::Left, true, kBodyScale)
            .DrawWrapped(kRightX, kPaneTop + 52, kRightWidth, 22, 0x80);

        if (quest.complete) {
            ui::Label(TrackedQuestStatus(quest), ui::ColorGold, ui::Alignment::Left, true, kBodyScale)
                .Draw(kRightX, 287, 0x80);
        } else {
            ui::Label(QuestText(kProgress), ui::ColorYellow, ui::Alignment::Left, true, kHeaderScale).Draw(kRightX, 287, 0x80);
            ui::DrawCounter(460, 287, quest.done, quest.total, kBodyScale);
        }
        ui::ProgressBarStyle style{};
        style.text_scale = 0.76f;
        const float ratio = quest.total > 0 ? static_cast<float>(quest.done) / static_cast<float>(quest.total) : 0.0f;
        ui::ProgressBar::Draw(ui::Rect(kRightX, 314, kRightWidth, 22), ratio, {}, style, 0x80);
        MenuTextureReload(0x1A);

        const int y = kListTop + (selected - first_row) * kRowSpacing;
        brackets.Draw(ui::Rect(kLeftX - 4, y - 1, 144, 22), 0x80);
        cursor.SetTarget(48.0f, static_cast<float>(y - 2));
        cursor.Draw(0x80);
    }

    ui::Label(QuestText(kControls), ui::ColorWhite, ui::Alignment::Center).Draw(320, 418, 0x80);
}

} // namespace expanded
