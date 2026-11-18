#include "quest/quests.hpp"

#ifdef CHRONICLE_EXPANDED
#include "expanded/quests/quests.hpp"
#endif

namespace dc::quest {

const NpcMatch &NoruneMayor() {
    static const NpcMatch mayor{.map_no = 0, .npc_id = 13, .mes_file_key = {}};
    return mayor;
}

void RegisterAllQuests() {
    Register(MayorSupplies());
    Register(CarlsSlingshot());
#ifdef CHRONICLE_EXPANDED
    expanded::RegisterQuests();
#endif
}

std::vector<std::pair<std::string, std::string>> QuestStrings() {
    std::vector<std::pair<std::string, std::string>> out;
    for (std::span<const Text> texts : {MayorSuppliesTexts(), CarlsSlingshotTexts()}) {
        for (const Text &text : texts) {
            out.emplace_back(text.key, text.english);
        }
    }
    return out;
}

} // namespace dc::quest
