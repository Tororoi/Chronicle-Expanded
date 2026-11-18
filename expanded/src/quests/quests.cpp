#include "expanded/quests/quests.hpp"

#include "expanded/quests/monster_slayer.hpp"

namespace expanded {

void RegisterQuests() {
    RegisterMonsterSlayer();
}

std::vector<dc::quest::Text> QuestTexts() {
    std::vector<dc::quest::Text> texts = {kOfferLabel, kAboutLabel};
    for (const auto &list : {MonsterSlayerTexts()}) {
        texts.insert(texts.end(), list.begin(), list.end());
    }
    return texts;
}

} // namespace expanded
