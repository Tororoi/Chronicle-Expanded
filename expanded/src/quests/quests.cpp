#include "expanded/quests/quests.hpp"

#include "expanded/quests/fishing_quests.hpp"
#include "expanded/quests/item_hunts.hpp"
#include "expanded/quests/monster_slayer.hpp"

namespace expanded {

void RegisterQuests() {
    RegisterMonsterSlayer();
    RegisterFishingQuests();
    RegisterItemHunts();
}

std::vector<dc::quest::Text> QuestTexts() {
    std::vector<dc::quest::Text> texts = {kOfferLabel, kAboutLabel};
    for (const auto &list : {MonsterSlayerTexts(), FishingQuestTexts(), ItemHuntTexts()}) {
        texts.insert(texts.end(), list.begin(), list.end());
    }
    return texts;
}

} // namespace expanded
